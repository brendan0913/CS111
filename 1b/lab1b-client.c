// NAME: Brendan Rossmango
// EMAIL: brendan0913@ucla.edu
// ID: 505370692

#include <termios.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include "zlib.h"

// Original terminal mode attributes
struct termios original_terminal_mode;

// Poll descriptors
struct pollfd pollfds[2];

// socket
int socket_fd;

// Compression and decompression streams
z_stream to_client_stream;
z_stream to_server_stream;

/* 
 * Referenced https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html for
 * restore and set terminal modes
 */
// Restores terminal mode immediately on exit
void restore_terminal_mode(void){
    if (tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal_mode) < 0){
        fprintf(stderr, "Error restoring original terminal mode; strerror reports: %s\r\n", strerror(errno));
        exit(1);
    }
}

// Sets terminal to noncanonical, no echo mode
void set_terminal_mode(void){
    struct termios tattr;

    // Make sure stdin is a terminal
    if (!isatty(STDIN_FILENO)){
        fprintf(stderr, "Error: Standard input is not a terminal; strerror reports: %s\r\n", strerror(errno));
        exit(1);
    }

    // Save the terminal attributes so we can restore on exit
    if (tcgetattr(STDIN_FILENO, &original_terminal_mode) < 0){
        fprintf(stderr, "Error getting original terminal mode; strerror reports: %s\r\n", strerror(errno));
        exit(1);
    }
    atexit(restore_terminal_mode);

    // Set the terminal modes
    if (tcgetattr(STDIN_FILENO, &tattr) < 0){
        fprintf(stderr, "Error getting temporary terminal mode; strerror reports: %s\r\n", strerror(errno));
        exit(1);
    }
    tattr.c_iflag = ISTRIP;
    tattr.c_oflag = 0;
    tattr.c_lflag = 0;
    tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tattr) < 0){
        fprintf(stderr, "Error setting noncanonical, no echo terminal mode; strerror reports: %s\r\n", strerror(errno));
        exit(1);
    }
}

// Refernced Tianxiang Li's discussion slides 
int client_connect(char* hostname, unsigned int port){
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Error socketing client; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    server = gethostbyname(hostname);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero)); // padding zeroes

    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        fprintf(stderr, "Error connecting client; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    return sockfd;
}

// Wrapper function, handles errors for calls to write(2)
// The s argument is used to customize the message for "^D", "<cr><lf>", etc.
void check_write(int fd, void* buf, size_t count, const char* s){
    if (write(fd, buf, count) < 0){
        if (fd == STDOUT_FILENO){
            fprintf(stderr, "Error writing %s to stdout; strerror reports: %s\r\n", s, strerror(errno));
            exit(1);
        }
        else{
            fprintf(stderr, "Error writing %s to shell, file %d; strerror reports: %s\r\n", s, fd, strerror(errno));
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
    int no_port_flag = 1;
    unsigned int port_number = 0;
    int log_flag = 0;
    char* log_filename = NULL;
    int log_file;
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
            case 'l':
                log_flag = 1;
                log_filename = optarg;
                break;
            case 'c':
                compress_flag = 1;
                break;
            default:
                fprintf(stderr, "Incorrect usage: correct usage is ./lab1b-client --port=portnumber [--log=filename] [--compress]\n");
                exit(1);
        }
    }
    if (no_port_flag){
        fprintf(stderr, "Error: the --port option is mandatory\n");
        exit(1);
    }
    if (log_flag){
        log_file = creat(log_filename, 0666);
        if (log_file < 0){
            fprintf(stderr, "Error creating/writing to log file; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
    }
    if (compress_flag){
        to_server_stream.opaque = Z_NULL;
        to_server_stream.zalloc = Z_NULL;
        to_server_stream.zfree = Z_NULL;
        if (deflateInit(&to_server_stream, Z_DEFAULT_COMPRESSION) != Z_OK){
            fprintf(stderr, "Error compressing z_stream to server; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
        to_client_stream.opaque = Z_NULL;
        to_client_stream.zalloc = Z_NULL;
        to_client_stream.zfree = Z_NULL;
        if (inflateInit(&to_client_stream) != Z_OK){
            fprintf(stderr, "Error decompressing z_stream to client; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
    }

    // connect to port
    socket_fd = client_connect("localhost", port_number);
    set_terminal_mode();

    pollfds[0].fd = STDIN_FILENO;
    pollfds[0].events = POLLIN + POLLERR + POLLHUP;
    pollfds[1].fd = socket_fd;
    pollfds[1].events = POLLIN + POLLERR + POLLHUP;

    while(1){
        // poll the socket_fd and stdin
        if (poll(pollfds, 2, 0) < 0){
            fprintf(stderr, "Error polling stdin and socket file descriptor; strerror reports: %s\r\n", strerror(errno));
            exit(1);
        }
        if (pollfds[0].revents & POLLIN){ // keyboard stdin
            // read from stdin, send to stdout and socket_fd
            char buffer[1024];
            int nbytes = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (nbytes < 0){
                fprintf(stderr, "Error reading input from keyboard; strerror reports: %s\r\n", strerror(errno));
                exit(1);
            }
            // echo to display
            int i;
            for (i = 0; i < nbytes; i++){
                char temp[2]; // temp handles <cr><lf> printing to screen
                char ch = buffer[i];
                if (ch == '\r' || ch == '\n'){
                    temp[0] = '\r';
                    temp[1] = '\n';
                    check_write(STDOUT_FILENO, temp, 2, "<cr><lf>");
                }
                else {
                    check_write(STDOUT_FILENO, &ch, 1, "input");            
                }
            }
            if (compress_flag){
                char compressed_out[1024];
                to_server_stream.avail_in = nbytes;
                to_server_stream.next_in = (Bytef *) buffer;
                to_server_stream.avail_out = sizeof(compressed_out);
                to_server_stream.next_out = (Bytef *) compressed_out;
                do{
                    if (deflate(&to_server_stream, Z_SYNC_FLUSH) != Z_OK){
                        fprintf(stderr, "Error compressing bytes from keyboard to server; strerror reports: %s\r\n", strerror(errno));
                        exit(1);                    
                    }
                } while(to_server_stream.avail_in > 0);
                // fprintf(stderr, "Keyboard to server good");
                if (write(socket_fd, compressed_out, sizeof(compressed_out) - to_server_stream.avail_out) < 0){
                    fprintf(stderr, "Error writing compressed bytes to server; strerror reports %s\r\n", strerror(errno));
                    exit(1);
                }
                if (log_flag){ // log outgoing bytes post-compression
                    if (dprintf(log_file, "SENT %d bytes: ", (int) (sizeof(compressed_out) - to_server_stream.avail_out)) < 0){
                        fprintf(stderr, "Error in dprintf writing compressed bytes to log file; strerror reports %s\r\n", strerror(errno));
                        exit(1);           
                    }
                    check_write(log_file, compressed_out, sizeof(compressed_out) - to_server_stream.avail_out, "log message");
                    check_write(log_file, "\n", 1, "log message");
               }
            }
            else{
                // send input to socket (includes ^D and ^C)
                if (write(socket_fd, buffer, nbytes) < 0){
                    fprintf(stderr, "Error sending input from keyboard to socket; strerror reports %s\r\n", strerror(errno));
                    exit(1);
                }
                if (log_flag){ // log outgoing bytes post-compression
                    if (dprintf(log_file, "SENT %d bytes: ", nbytes) < 0){
                        fprintf(stderr, "Error in dprintf writing to log file; strerror reports %s\r\n", strerror(errno));
                        exit(1);           
                    }
                    check_write(log_file, buffer, nbytes, "log message");
                    check_write(log_file, "\n", 1, "log message");
                }
            }
        }
        if (pollfds[1].revents & POLLIN){ // socket_fd
            // read from socket_fd, process special characters, send to stdout
            char buffer[1024];
            int nbytes = read(socket_fd, buffer, sizeof(buffer)); // reads stdin
            if (nbytes < 0){
                fprintf(stderr, "Error reading input from keyboard; strerror reports: %s\r\n", strerror(errno));
                exit(1);
            }
            else if (nbytes == 0){
                break;
            }
            if (log_flag){ // log incoming bytes pre-decompression
                if (dprintf(log_file, "RECEIVED %d bytes: ", nbytes) < 0){
                    fprintf(stderr, "Error in dprintf writing to log file; strerror reports %s\r\n", strerror(errno));
                    exit(1);           
                }
                check_write(log_file, buffer, nbytes, "log message");
                check_write(log_file, "\n", 1, "log message");
            }
            if (compress_flag){
                char decompressed_in[2048];
                to_client_stream.avail_in = nbytes;
                to_client_stream.next_in = (Bytef *) buffer;
                to_client_stream.avail_out = sizeof(decompressed_in);
                to_client_stream.next_out = (Bytef *) decompressed_in;
                do{
                    if (inflate(&to_client_stream, Z_SYNC_FLUSH) != Z_OK){
                        fprintf(stderr, "Error decompressing bytes from server to client; strerror reports: %s\r\n", strerror(errno));
                        exit(1);
                    }
                } while(to_client_stream.avail_in > 0);
                nbytes = sizeof(decompressed_in) - to_client_stream.avail_out;
                // fprintf(stderr, "Server to client good");
                int i;
                for (i = 0; i < nbytes; i++){
                    char temp[2]; // temp handles <cr><lf> printing to screen
                    char ch = decompressed_in[i];
                    if (ch == '\r' || ch == '\n'){
                        temp[0] = '\r';
                        temp[1] = '\n';
                        check_write(STDOUT_FILENO, temp, 2, "<cr><lf>");
                    }
                    else{
                        check_write(STDOUT_FILENO, &ch, 1, "input");            
                    }
                }
            }
            else{
                // echo to display
                int i;
                for (i = 0; i < nbytes; i++){
                    char temp[2]; // temp handles <cr><lf> printing to screen
                    char ch = buffer[i];
                    if (ch == '\r' || ch == '\n'){
                        temp[0] = '\r';
                        temp[1] = '\n';
                        check_write(STDOUT_FILENO, temp, 2, "<cr><lf>");
                    }
                    else{
                        check_write(STDOUT_FILENO, &ch, 1, "input");            
                    }
                }
            }
        }
        if (pollfds[0].revents & (POLLERR | POLLHUP)){
            fprintf(stderr, "Error polling the input from keyboard; strerror reports: %s\r\n", strerror(errno));
            exit(1);
        }
        if (pollfds[1].revents & (POLLERR | POLLHUP)){
            // proceed to exit process
            break;
        }
    }
    if (close(socket_fd) < 0){
        fprintf(stderr, "Error closing socket from client; strerror reports: %s\r\n", strerror(errno));
        exit(1);
    }
    if (compress_flag){
        deflateEnd(&to_server_stream);
        inflateEnd(&to_client_stream);
    }
    // restores terminal on exit
    exit(0);
}