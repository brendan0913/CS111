// NAME: Brendan Rossmango
// EMAIL: brendan0913@ucla.edu
// ID: 505370692

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

// Initialize shared long long counter to 0
long long counter = 0;

int num_of_threads = 1, num_of_iterations = 1; // default 1
int num_of_operations; // num_of_threads * num_of_operations * 2
int opt_yield = 0;

// To create testname, length < 11 (longest name is "yield-none")
// "add-" is printed in record_data function
char test_name[11];

long long runtime, avgtime;

// sync options
int mutex_sync = 0, spinlock_sync = 0, compare_and_swap_sync = 0;
// mutex and lock for spin-lock
pthread_mutex_t mutex;
int lock = 0;

// no sync option
void add(long long *pointer, long long value){
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
}

// mutex sync option
void add_m(long long *pointer, long long value){
    pthread_mutex_lock(&mutex);
    add(pointer, value);
    pthread_mutex_unlock(&mutex);
}

// spin-lock sync option
void add_s(long long *pointer, long long value){
    while (__sync_lock_test_and_set(&lock, 1));
    add(pointer, value);
    __sync_lock_release(&lock);
}

// compare-and-swap sync option
void add_c(long long value){
    long long oldval, newval;
    do{
        oldval = counter;
        newval = oldval + value;
        if (opt_yield)
            sched_yield();
    } while (__sync_val_compare_and_swap(&counter, oldval, newval) != oldval);
}

void *run_threads(){
    int i;
    // Increment counter
    for (i = 0; i < num_of_iterations; i++){
        if (mutex_sync){
            add_m(&counter, 1);
        }
        else if (spinlock_sync){
            add_s(&counter, 1);
        }
        else if (compare_and_swap_sync){
            add_c(1);
        }
        else{
            add(&counter, 1);
        }
    }
    // Decrement counter
    for (i = 0; i < num_of_iterations; i++){
        if (mutex_sync){
            add_m(&counter, -1);
        }
        else if (spinlock_sync){
            add_s(&counter, -1);
        }
        else if (compare_and_swap_sync){
            add_c(-1);
        }
        else{
            add(&counter, -1);
        }    
    }
    return NULL;
}

// Refernced TA Tianxiang Li's slides
static unsigned long get_nanosec_from_timespec(struct timespec *spec){
    unsigned long ret= spec->tv_sec; // seconds
    return ret * 1000000000 + spec->tv_nsec; // nanoseconds
}

void record_data(void){
    num_of_operations = num_of_threads * num_of_iterations * 2;
    avgtime = runtime / num_of_operations;
    fprintf(stdout, "add-%s,%d,%d,%d,%lld,%lld,%lld\n", test_name, num_of_threads, num_of_iterations,
                                            num_of_operations, runtime, avgtime,
                                            counter);
}

int main(int argc, char** argv){
    static struct option long_options[] =
    {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"yield", no_argument, NULL, 'y'},
        {"sync", required_argument, NULL, 's'},
        {0, 0, 0, 0}
    };
    int c;
    while(1){
        c = getopt_long(argc, argv, "", long_options, NULL);
        if (c == -1) break;
        switch (c){
            case 't':
                num_of_threads = atoi(optarg);
                break;
            case 'i':
                num_of_iterations = atoi(optarg);
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                if (strlen(optarg) > 1){
                    fprintf(stderr, "Incorect usage: --sync=[m][s][c]: maximum 1 usable option: options are (m)utex, (s)pin-lock, or (c)ompare-and-swap\n");
                    exit(1);
                }
                else{
                    if (optarg[0] == 'm') mutex_sync = 1;
                    else if (optarg[0] == 's') spinlock_sync = 1;
                    else if (optarg[0] == 'c') compare_and_swap_sync = 1;
                    else{
                        fprintf(stderr, "Incorrect sync option '%s' for --sync=[m][s][c]: options are (m)utex, (s)pin-lock, or (c)ompare-and-swap\n", optarg);
                        exit(1);
                    }
                }
                break;
            default:
                fprintf(stderr, "Incorrect usage: ./lab2_add [--threads=n_threads] [--iterations=n_iterations] [--yield] [--sync=[msc]]\n");
                exit(1);
        }
    }
    if (num_of_threads < 1 || num_of_iterations < 1){
        fprintf(stderr, "Incorrect usage: number of threads and iterations must be at least 1\n");
        exit(1);
    }
    if (opt_yield){
        strcat(test_name, "yield-");
    }
    if (mutex_sync){
        if (pthread_mutex_init(&mutex, NULL) != 0){
            fprintf(stderr, "Error initializing mutex; strerror reports: %s\n", strerror(errno));
            exit(1);
        }    
        strcat(test_name, "m");
    }
    else if (spinlock_sync) strcat(test_name, "s");
    else if (compare_and_swap_sync) strcat(test_name, "c");
    else strcat(test_name, "none");

    struct timespec start_time, end_time;
    int i;
    // Note starting time for run
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0){
        fprintf(stderr, "Error getting start time of run; strerror reports: %s\n", strerror(errno));
        exit(1);
    }    
    // Create and start threads
    pthread_t threads[num_of_threads];
    for (i = 0; i < num_of_threads; i++){
        if (pthread_create(&(threads[i]), NULL, run_threads, NULL) != 0){
            fprintf(stderr, "Error creating threads; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
    }
    // Wait for all threads to complete and join
    for (i = 0; i < num_of_threads; i++){
        if (pthread_join(threads[i], NULL) != 0){
            fprintf(stderr, "Error joining threads; strerror reports: %s\n", strerror(errno));
            exit(1);
        }    
    }
    // Note ending time of run
    if (clock_gettime(CLOCK_MONOTONIC, &end_time) != 0){
        fprintf(stderr, "Error getting end time of run; strerror reports: %s\n", strerror(errno));
        exit(1);
    }    
    if (mutex_sync){
        if (pthread_mutex_destroy(&mutex) != 0){
            fprintf(stderr, "Error destroying mutex; strerror reports: %s\n", strerror(errno));
            exit(1);
        }    
    }
    runtime = get_nanosec_from_timespec(&end_time) - get_nanosec_from_timespec(&start_time);
    record_data();
    exit(0);
}