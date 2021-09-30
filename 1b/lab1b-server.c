// NAME: Brendan Rossmango
// EMAIL: brendan0913@ucla.edu
// ID: 505370692

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include "zlib.h"

// Child process
pid_t child;

// Pipes
int from_shell[2];
int to_shell[2];

// Poll descriptors
struct pollfd pollfds[2];

// Compression and decompression streams
z_stream to_client_stream;
z_stream to_server_stream;

int is_polling_stopped, is_pipe_to_shell_closed, is_pipe_from_shell_closed;

// Refernced Tianxiang Li's discussion slides 
int server_connect(unsigned int port_number){
    int sockfd, newfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    unsigned int sin_size;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Error creating socket for server; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "Error binding server; strerror reports: %s\n", strerror(errno));
        exit(1);
    }

    if (listen(sockfd, 5) < 0){
        fprintf(stderr, "Error listening to socket file descriptor with 5 maximum connections; strerror reports: %s\n", strerror(errno));
        exit(1);
    }

    sin_size = sizeof(struct sockaddr_in);
    if ((newfd = accept(sockfd, (struct sockaddr*) &client_addr, &sin_size)) < 0){
        fprintf(stderr, "Error accepting the client connection; strerror reports: %s\n", strerror(errno));
        exit(1);
    }

    if (close(sockfd) < 0){
        fprintf(stderr, "Error closing listening socket; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    return newfd;
}

void signal_handler(int signum){
    if (signum == SIGPIPE){
        is_polling_stopped = 1;
    }
}

// Wrapper function, handles errors for calls to close(2)
void check_close(int fd){
    if (close(fd) < 0){
        fprintf(stderr, "Error closing file descriptor %d; strerror reports: %s\n", fd, strerror(errno));
        exit(1);
    }
}

// Wrapper function, handles errors for calls to dup2(2)
void check_dup2(int oldfd, int newfd){
    if (dup2(oldfd, newfd) < 0){
        fprintf(stderr, "Error duplicating old fd %d to new fd %d; strerror reports: %s\n", oldfd, newfd, strerror(errno));
        exit(1);
    }
}

// Wrapper function, handles errors for calls to write(2)
// The s argument is used to customize the message for "^D", "<cr><lf>", etc.
void check_write(int fd, void* buf, size_t count, const char* s){
    if (write(fd, buf, count) < 0){
        if (fd == STDOUT_FILENO){
            fprintf(stderr, "Error writing %s to stdout; strerror reports: %s\n", s, strerror(errno));
            exit(1);
        }
        else{
            fprintf(stderr, "Error writing %s to shell, file descriptor %d; strerror reports: %s\n", s, fd, strerror(errno));
            exit(1);
        }
    }
}

int main(int argc, char** argv){
    static struct option long_options[] = {
		{"port", required_argument, NULL, 'p'},
        {"log", required_argument, NULL, 'l'},
        {"compress", no_argument, NULL, 'c'},
		{0, 0, 0, 0}
	};
    int socket_fd;
    int no_port_flag = 1;
    unsigned int port_number = 0;
    int compress_flag = 0;
    int c;
    while(1){
        c = getopt_long(argc, argv, "", long_options, NULL);
        if (c == -1) break;
        switch(c){
            case 'p':
                port_number = atoi(optarg);
                no_port_flag = 0;
                break;
            case 'c':
                compress_flag = 1;
                break;
            default:
                fprintf(stderr, "Incorrect usage: correct usage is ./lab1b-server --port=portnumber [--compress]\n");
                exit(1);
        }
    }
    if (no_port_flag){
        fprintf(stderr, "Error: the --port option is mandatory\n");
        exit(1);
    }
    if (compress_flag){
        to_client_stream.opaque = Z_NULL;
        to_client_stream.zalloc = Z_NULL;
        to_client_stream.zfree = Z_NULL;
        if (deflateInit(&to_client_stream, Z_DEFAULT_COMPRESSION) != Z_OK){
            fprintf(stderr, "Error compressing z_stream to client; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
        to_server_stream.opaque = Z_NULL;
        to_server_stream.zalloc = Z_NULL;
        to_server_stream.zfree = Z_NULL;
        if (inflateInit(&to_server_stream) != Z_OK){
            fprintf(stderr, "Error decompressing z_stream to server; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
    }

    // connect to port
    socket_fd = server_connect(port_number);

    // register SIGPIPE handler
    if (signal(SIGPIPE, signal_handler) == SIG_ERR){
        fprintf(stderr, "Error signaling SIGPIPE signal handler; strerror reports: %s\n", strerror(errno));
        exit(1);
    }

    // create pipes from shell, to shell
    if (pipe(from_shell) < 0){
        fprintf(stderr, "Error creating pipe from shell; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    if (pipe(to_shell) < 0){
        fprintf(stderr, "Error creating pipe to shell; strerror reports: %s\n", strerror(errno));
        exit(1);
    }

    child = fork();
    if (child < 0){
        fprintf(stderr, "Error forking process; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    else if (child == 0){
        check_close(to_shell[1]); // close write end of pipe to shell
        check_close(from_shell[0]); // close read end of pipe from shell

        check_dup2(to_shell[0], STDIN_FILENO); // input of shell
        check_dup2(from_shell[1], STDOUT_FILENO); // stdout of shell
        check_dup2(from_shell[1], STDERR_FILENO); // stderr of shell

        // after duplicating the pipes, can now close pipes
        check_close(to_shell[0]);
        check_close(from_shell[1]);

        if (execl("/bin/bash", "/bin/bash", NULL) < 0){
            fprintf(stderr, "Error executing shell; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
    }
    // parent process
    check_close(to_shell[0]); // close read to shell
    check_close(from_shell[1]); // close write from shell
    pollfds[0].fd = socket_fd;
    pollfds[0].events = POLLIN + POLLERR + POLLHUP;
    pollfds[1].fd = from_shell[0];
    pollfds[1].events = POLLIN + POLLERR + POLLHUP;

    is_polling_stopped = 0;
    is_pipe_from_shell_closed = 0;
    is_pipe_to_shell_closed = 0;
    while(!is_polling_stopped){
        // poll socket_fd and from_shell[0] (read end of pipe from shell)
        if (poll(pollfds, 2, 0) < 0){
            fprintf(stderr, "Error polling stdin and socket file descriptor; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
        if (pollfds[0].revents & POLLIN){ // socket_fd
            // read from socket_fd, send to write end of pipe to shell, to_shell[1]
            char buffer[1024];
            int nbytes = read(socket_fd, buffer, sizeof(buffer));
            if (nbytes < 0){
                fprintf(stderr, "Error reading from socket; strerror reports: %s\n", strerror(errno));
                exit(1);
            }
            if (compress_flag){
                char decompressed_in[1024];
                memcpy(decompressed_in, buffer, nbytes);
                to_server_stream.avail_in = nbytes;
                to_server_stream.next_in = (Bytef *) decompressed_in;
                to_server_stream.avail_out = sizeof(decompressed_in);
                to_server_stream.next_out = (Bytef *) buffer;
                do{
                    if (inflate(&to_server_stream, Z_SYNC_FLUSH) != Z_OK){
                        fprintf(stderr, "Error decompressing bytes from client to shell; strerror reports: %s\n", strerror(errno));
                        exit(1);               
                    }
                } while (to_server_stream.avail_in > 0);
                nbytes = sizeof(decompressed_in) - to_server_stream.avail_out;
                // fprintf(stderr, "Server decompress bytes good");
            }
            int i;
            for (i = 0; i < nbytes; i++){
                char temp[2]; // temp handles <cr><lf>
                char ch = buffer[i];
                if (ch == '\r' || ch == '\n'){
                    temp[0] = '\n';
                    // write <lf> to shell
                    check_write(to_shell[1], temp, 1, "<lf>");
                }
                else if (ch == 0x04){
                    if (!is_pipe_to_shell_closed){
                        check_close(to_shell[1]);
                        is_pipe_to_shell_closed = 1;
                    }
                }
                else if (ch == 0x03){
                    if (kill(child, SIGINT) < 0){
                        fprintf(stderr, "Error killing child process with SIGINT on ^C; strerror reports: %s\n", strerror(errno));
                        exit(1);
                    }              
                }
                else {
                    check_write(to_shell[1], &ch, 1, "from socket");            
                }
            }
        }
        if (pollfds[1].revents & POLLIN){ // read end of pipe from shell, from_shell[0]
            // read from from_shell[0], send to socket_fd
            char buffer[1024];
            int nbytes = read(from_shell[0], buffer, sizeof(buffer));
            if (nbytes < 0){
                fprintf(stderr, "Error reading from shell; strerror reports: %s\n", strerror(errno));
                exit(1);
            }
            if (compress_flag){
                char compressed_out[1024];
                memcpy(compressed_out, buffer, nbytes);
                to_client_stream.avail_in = nbytes;
                to_client_stream.next_in = (Bytef *) compressed_out;
                to_client_stream.avail_out = sizeof(compressed_out);
                to_client_stream.next_out = (Bytef *) buffer;
                do{
                    if (deflate(&to_client_stream, Z_SYNC_FLUSH) != Z_OK){
                        fprintf(stderr, "Error compressing bytes from shell to client; strerror reports: %s\n", strerror(errno));
                        exit(1);               
                    }
                } while (to_client_stream.avail_in > 0);
                nbytes = sizeof(compressed_out) - to_client_stream.avail_out;
                // fprintf(stderr, "Shell to client good");
            }
            check_write(socket_fd, buffer, nbytes, "from shell");
        }          
        if (pollfds[0].revents & (POLLERR | POLLHUP)){
            // EOF or read error from socket
            if (!is_pipe_to_shell_closed){
                check_close(to_shell[1]);
                is_pipe_to_shell_closed = 1;
            }
            is_polling_stopped = 1;
        }
        if (pollfds[1].revents & (POLLERR | POLLHUP)){
            // proceed to exit process
            is_polling_stopped = 1;
        }
    }

    if (!is_pipe_to_shell_closed){
        check_close(to_shell[1]);
        is_pipe_to_shell_closed = 1;
    }
    check_close(from_shell[0]);

    if (close(socket_fd) < 0){
        fprintf(stderr, "Error closing socket; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    if (compress_flag){
        deflateEnd(&to_client_stream);
        inflateEnd(&to_server_stream);
    }

    // Collect shell's exit status and report to stderr
    int status;
    if (waitpid(child, &status, 0) < 0){
        fprintf(stderr, "Error in parent process waiting for child process; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    int exit_signal = status & 0x007f;
    int exit_status = WEXITSTATUS(status);
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", exit_signal, exit_status);
    exit(0);
}