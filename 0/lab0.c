// NAME: Brendan Rossmango                                                                                               
// EMAIL: brendan0913@ucla.edu
// ID: 505370692 

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

/* 
   For input/output redirection,
   referenced http://web.cs.ucla.edu/~harryxu/courses/111/winter21/ProjectGuide/fd_juggling.html
 */

// If successful read, redirects input
void redirect_input(char* input){
    int ifd = open(input, O_RDONLY);
    if (ifd >= 0) {
        close(0);
        dup(ifd);
        close(ifd);
    }
    else {
        fprintf(stderr, "--input: %s: unable to open file '%s' \n", strerror(errno), input);
        exit(2);
    }
}

// If successful create, redirects output 
void redirect_output(char* output){
    int ofd = creat(output, 0666);
    if (ofd >= 0) {
        close(1);
        dup(ofd);
        close(ofd);
    }
    else {
        fprintf(stderr,"--output: %s: unable to create file '%s' \n", strerror(errno), output);
        exit(3);
    }
}

// Forces segfault by storing through the null ptr
void force_segfault(){
    char* ptr = NULL;
    *ptr = 'p';
}

// Prints that the segmentation fault is caught if SIGSEGV is signaled
void signal_handler(int signal){
    if (signal == SIGSEGV){
        fprintf(stderr, "%s: Segmentation fault caught\n", strerror(errno));
        exit(4);
    }
}

// Reads from fd 0 and writes to fd 1
void copy_stdin_to_stdout(){
    char buf;
    size_t nbytes = read(0, &buf, 1);
    while(nbytes > 0){
        write(1, &buf, 1);
        nbytes = read(0, &buf, 1);
    }
}

int
main (int argc, char **argv)
{
    // Variables to store results in after processing the arguments
    char* input = NULL;
    char* output = NULL;
    int catch = 0;
    int segfault = 0;
    int c;
    // Referenced https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html for how to use getopt_long
    while (1){
        static struct option long_options[] =
        {
            // The required argument for input and output is the file name
            {"input", required_argument, 0, 'i'},
            {"output", required_argument, 0, 'o'},
            // segfault and catch are optional
            {"segfault", no_argument, 0, 's'},
            {"catch", no_argument, 0, 'c'},
            {0, 0, 0, 0}
        };
        c = getopt_long(argc, argv, "i:o:sc", long_options, NULL);
        if (c == -1) break;
        switch (c)
        {
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 's':
                segfault = -1;
                break;
            case 'c':
                catch = -1;
                break;
            default:
                printf("Incorrect usage: accepted arguments are [--input=filename1] [--output=filename2] [--segfault] [--catch] \n");
                exit(1);
        }
    }
    // Do any file redirection, then register the signal handler, then cause the segfault
    if (input) redirect_input(input);
    if (output) redirect_output(output);
    if (catch == -1) signal(SIGSEGV, signal_handler);
    if (segfault == -1) force_segfault();
    // If no segfault is caused, then copy input to output and exit successfully with code 0
    copy_stdin_to_stdout();
    exit(0);
}