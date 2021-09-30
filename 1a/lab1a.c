// NAME: Brendan Rossmango
// EMAIL: brendan0913@ucla.edu
// ID: 505370692

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

struct termios original_terminal_mode;

// --shell or no --shell 
int shell_flag;

// Child process
pid_t child;

// Pipes and flags to know if we already closed the necessary pipes
int from_terminal_to_shell[2];
int to_terminal_from_shell[2];
int is_pipe_to_shell_closed;
int is_pipe_from_shell_closed;

int is_polling_stopped;
struct pollfd pollfds[2];

/* 
 * Referenced https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html for
 * restore and set terminal modes
 */
void restore_terminal_mode(void){
    if (tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal_mode) < 0){
        fprintf(stderr, "Error restoring original terminal mode; strerror reports: %s\r\n", strerror(errno));
        exit(1);
    }
}

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

// Wrapper function, handles errors for calls to close(2)
void check_close(int fd){
    if (close(fd) < 0){
        fprintf(stderr, "Error closing file descriptor %d; strerror reports: %s\r\n", fd, strerror(errno));
        exit(1);
    }
}

// Wrapper function, handles errors for calls to dup2(2)
void check_dup2(int oldfd, int newfd){
    if (dup2(oldfd, newfd) < 0){
        fprintf(stderr, "Error duplicating old fd %d to new fd %d; sterror reports: %s\r\n", oldfd, newfd, strerror(errno));
        exit(1);
    }
}

// Signal handler for SIGPIPE
void signal_handler(int signal){
    if (signal == SIGPIPE){
        is_polling_stopped = 1;
    }
}

int main(int argc, char** argv){
    shell_flag = 0;
    static struct option long_options[] = {
		{"shell", no_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

    int c;
    while(1){
        c = getopt_long(argc, argv, "s", long_options, NULL);
        if (c == -1) break;
        switch(c){
            case 's':
                shell_flag = 1;
                break;
            default:
                fprintf(stderr, "Incorrect usage: correct usage is ./lab1a [--shell]\r\n");
                exit(1);
        }
    }
    set_terminal_mode();
    if(shell_flag == 1){
		if (signal(SIGPIPE, signal_handler) == SIG_ERR){
			fprintf(stderr, "Error signaling the signal handler for SIGPIPE; sterror reports: %s\r\n", strerror(errno));
			exit(1);
		} 
        if (pipe(from_terminal_to_shell) < 0){
            fprintf(stderr, "Error creating pipe from terminal to shell; sterror reports: %s\r\n", strerror(errno));
            exit(1);
        }
        if(pipe(to_terminal_from_shell) < 0){
            fprintf(stderr, "Error creating pipe from shell to terminal; sterror reports: %s\r\n", strerror(errno));
            exit(1);
        }

        child = fork();
        if (child < 0){
            fprintf(stderr, "Error creating child process with fork; sterror reports: %s\r\n", strerror(errno));
            exit(1);
        }
        else if (child == 0){ // child process is the shell
            /* shell's input is from the read end of pipe from terminal to shell,
             * and shell's output is the write end of the pipe from shell to terminal, so
             * we close the other unused pipes
             */
            check_close(from_terminal_to_shell[1]); // close write end of pipe to shell
            check_close(to_terminal_from_shell[0]); // close read end of pipe from shell

            check_dup2(from_terminal_to_shell[0], STDIN_FILENO); // input of shell
            check_dup2(to_terminal_from_shell[1], STDOUT_FILENO); // stdout of shell
            check_dup2(to_terminal_from_shell[1], STDERR_FILENO); // stderr of shell

            // After duplicating the pipes, can now close the pipes
            check_close(from_terminal_to_shell[0]);
            check_close(to_terminal_from_shell[1]);

            if (execl("/bin/bash", "/bin/bash", NULL) < 0){
                fprintf(stderr, "Error executing shell; strerror reports: %s\r\n", strerror(errno));
                exit(1);
            }
        }
        else if (child > 0){ // parent process is the terminal
            check_close(from_terminal_to_shell[0]); // close read from terminal
            check_close(to_terminal_from_shell[1]); // close write from shell
            pollfds[0].fd = STDIN_FILENO;
            pollfds[0].events = POLLIN + POLLHUP + POLLERR;
            pollfds[1].fd = to_terminal_from_shell[0];
            pollfds[1].events = POLLIN + POLLHUP + POLLERR;
        }
    }

    // Processes both situations (--shell or no --shell)
    char buffer[256];
    int nbytes = 0;
    is_polling_stopped = 0;
	is_pipe_from_shell_closed = 0;
	is_pipe_to_shell_closed = 0;
    while(!is_polling_stopped){
        if (poll(pollfds, 2, 0) < 0){ // timeout is 0
            fprintf(stderr, "Error polling the keyboard/shell input; sterror reports: %s\r\n", strerror(errno));
            exit(1);
        }
        if ((pollfds[1].revents & POLLIN) && shell_flag == 1){ // pipe that returns output to shell, only happens with --shell
            nbytes = read(pollfds[1].fd, buffer, 256); // reads from to_terminal_from_shell[0]
            if (nbytes < 0){
                fprintf(stderr, "Error reading input from shell; sterror reports: %s\r\n", strerror(errno));
                exit(1);
            }
            int i;
            for (i = 0; i < nbytes; i++){
                char temp[2]; // temp handles printing <cr><lf> to screen
                char ch = buffer[i];
                if (ch == 0x04){ // receive ^D from shell, we know there is no more output, so stop polling
                    is_polling_stopped = 1;
                    break;
                }
                else if (ch == '\n'){ // map <lf> to <cr><lf>
                    temp[0] = '\r';
                    temp[1] = '\n';
                    check_write(STDOUT_FILENO, temp, 2, "<cr><lf>"); // prints to STDOUT as <cr><lf>
                }
                else{
                    check_write(STDOUT_FILENO, &ch, 1, "input");
                }
            }
        }
        else if ((pollfds[0].revents & POLLIN) || (shell_flag == 0)){ // keyboard stdin (for both --shell and no --shell)
            nbytes = read(0, buffer, 256);
            if (nbytes < 0){
                fprintf(stderr, "Error reading input from keyboard (stdin); sterror reports: %s\r\n", strerror(errno));
                exit(1);
            }
            int i;
            for (i = 0; i < nbytes; i++){
                char temp[2]; // temp handles returning '^D' or <cr><lf>
                char ch = buffer[i];
                if (ch == 0x04){ // ^D, write ^D, exit, and restore on exit
                    temp[0] = '^';
                    temp[1] = 'D';
                    check_write(STDOUT_FILENO, temp, 2, "^D");
                    if (shell_flag == 1){
                        if (!is_pipe_to_shell_closed){
                            check_close(from_terminal_to_shell[1]);
                            is_pipe_to_shell_closed = 1;
                        }
                    }
                    else{
                        exit(0);
                    }
                }
                else if (ch == '\r' || ch == '\n'){ // map <cr> or <lf> to <cr><lf>
                    temp[0] = '\r';
                    temp[1] = '\n';
                    if (shell_flag == 1){
                        check_write(STDOUT_FILENO, temp, 2, "<cr><lf>"); // echoes to STDOUT as <cr><lf>
                        check_write(from_terminal_to_shell[1], temp + 1, 1, "<lf>"); // sent to shell as <lf>
                    }
                    else{
                        check_write(STDOUT_FILENO, temp, 2, "<cr><lf>"); // if no --shell argument, echoes to STDOUT as <cr><lf>
                    }
                }
                else if (ch == 0x03){ // '^C' handling
                    temp[0] = '^';
                    temp[1] = 'C';
                    check_write(STDOUT_FILENO, temp, 2, "^C");
                    if (shell_flag == 1){
                        if (kill(child, SIGINT) < 0){
                            fprintf(stderr, "Error killing child process with SIGINT on ^C; sterror reports: %s\r\n", strerror(errno));
                            exit(1);
                        }
                    }
                    else{ // with no --shell argument, ^C is a normal character
                        check_write(STDOUT_FILENO, &ch, 1, "input");
                    }
                }
                else{
                    check_write(STDOUT_FILENO, &ch, 1, "input");
                    if (shell_flag == 1){
                       check_write(from_terminal_to_shell[1], &ch, 1, "input");
                    }
                }
            }
        }
        if (pollfds[0].revents & (POLLERR | POLLHUP) && shell_flag){
            fprintf(stderr, "Error polling the input from keyboard; sterror reports: %s\r\n", strerror(errno));
            exit(1);
        }
        if (pollfds[1].revents & (POLLERR | POLLHUP) && shell_flag){ // ^D from shell is handled by poll.revents[1] & (POLLHUP | POLLERR)
            if (!is_pipe_from_shell_closed){
                check_close(to_terminal_from_shell[0]);
                is_pipe_from_shell_closed = 1;
            }
            break;
        }
    }
    // Close write end of pipe to shell and read end of pipe from shell
    if (!is_pipe_to_shell_closed){
        check_close(from_terminal_to_shell[1]);
    }
    if (!is_pipe_from_shell_closed){
        check_close(to_terminal_from_shell[0]);
    }

    // Collect shell's exit status and report to stderr
    int status;
    if (waitpid(child, &status, 0) < 0){
        fprintf(stderr, "Error in parent process waiting for child process; sterror reports: %s\r\n", strerror(errno));
        exit(1);
    }
    int exit_signal = status & 0x007f;
    int exit_status = WEXITSTATUS(status);
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", exit_signal, exit_status);
    exit(0);
}