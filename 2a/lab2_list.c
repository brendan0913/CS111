// NAME: Brendan Rossmango
// EMAIL: brendan0913@ucla.edu
// ID: 505370692

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "SortedList.h"

int num_of_threads = 1, num_of_iterations = 1; // default 1
int num_of_operations; // num_of_threads * num_of_iterations * 3
int num_of_elements;
int insert_yield = 0, delete_yield = 0, lookup_yield = 0;
int opt_yield;

// To create testname, length of yieldopts or syncops < 5 (longest name is "none")
// "list" and "-" are printed separately in record_data function
char yieldopts[5];
char syncops[5];

long long runtime, avgtime;

// sync options
int mutex_sync = 0, spinlock_sync = 0;
// mutex and lock for spin-lock
pthread_mutex_t mutex;
int lock = 0;

SortedList_t* list;
SortedListElement_t* elements;

// Refernced TA Tianxiang Li's slides
static unsigned long get_nanosec_from_timespec(struct timespec *spec){
    unsigned long ret= spec->tv_sec; // seconds
    return ret * 1000000000 + spec->tv_nsec; // nanoseconds
}

void *run_threads(void* arg){
    int i;
    // arg is the thread id, so the thread only handles own elements
    // each thread inserts num_of_iterations elements
    int current = *((int*) arg) * num_of_iterations;
    SortedListElement_t* to_be_deleted;
    // Insert into single, shared list
    for (i = current; i < current + num_of_iterations; i++){
        if (mutex_sync){
            pthread_mutex_lock(&mutex);
            SortedList_insert(list, &elements[i]);
            pthread_mutex_unlock(&mutex);
        }
        else if (spinlock_sync){
            while (__sync_lock_test_and_set(&lock, 1));
            SortedList_insert(list, &elements[i]);
            __sync_lock_release(&lock);
        }
        else{
            SortedList_insert(list, &elements[i]);
        }
    }
    // Get list length (and check for corruption)
    // If length of list is less than the num of iterations or -1, there is corruption
    if (mutex_sync){
        pthread_mutex_lock(&mutex);
        if (SortedList_length(list) < num_of_iterations){
            fprintf(stderr, "Corruption: length of list is less than number of iterations or -1; strerror reports: %s\n", strerror(errno));
            exit(2);
        }
        pthread_mutex_unlock(&mutex);
    }
    else if (spinlock_sync){
        while (__sync_lock_test_and_set(&lock, 1));
        if (SortedList_length(list) < num_of_iterations){
            fprintf(stderr, "Corruption: length of list is less than number of iterations or -1; strerror reports: %s\n", strerror(errno));
            exit(2);
        }        
        __sync_lock_release(&lock);
    }
    else {
        if (SortedList_length(list) < num_of_iterations){
            fprintf(stderr, "Corruption: length of list is less than number of iterations or -1; strerror reports: %s\n", strerror(errno));
            exit(2);
        }
    }
    // Lookup and delete each key that was previously inserted
    // If corruption is found (e.g., a key that was inserted cannot be found), exit(2)
    for (i = current; i < current + num_of_iterations; i++){
        if (mutex_sync){
            pthread_mutex_lock(&mutex);
            to_be_deleted = SortedList_lookup(list, elements[i].key);
            if (to_be_deleted == NULL){
                fprintf(stderr, "Corruption: cannot lookup previously inserted key; strerror reports: %s\n", strerror(errno));
                exit(2);
            }
            // SortedList_delete returns 1 on corruption
            if (SortedList_delete(to_be_deleted)){
                fprintf(stderr, "Corruption: cannot delete previously found key; strerror reports: %s\n", strerror(errno));
                exit(2);     
            }
            pthread_mutex_unlock(&mutex);
        }
        else if (spinlock_sync){
            while (__sync_lock_test_and_set(&lock, 1));
            to_be_deleted = SortedList_lookup(list, elements[i].key);
            if (to_be_deleted == NULL){
                fprintf(stderr, "Corruption: cannot lookup previously inserted key; strerror reports: %s\n", strerror(errno));
                exit(2);
            }
            // SortedList_delete returns 1 on corruption
            if (SortedList_delete(to_be_deleted)){
                fprintf(stderr, "Corruption: cannot delete previously found key; strerror reports: %s\n", strerror(errno));
                exit(2);     
            }
            __sync_lock_release(&lock);
        }
        else{
            to_be_deleted = SortedList_lookup(list, elements[i].key);
            if (to_be_deleted == NULL){
                fprintf(stderr, "Corruption: cannot lookup previously inserted key; strerror reports: %s\n", strerror(errno));
                exit(2);
            }
            // SortedList_delete returns 1 on corruption
            if (SortedList_delete(to_be_deleted)){
                fprintf(stderr, "Corruption: cannot delete previously found key; strerror reports: %s\n", strerror(errno));
                exit(2);     
            }
        }
    }
    return NULL;
}

void record_data(){
    // 3 operations: insert, delete, lookup
    num_of_operations = num_of_threads * num_of_iterations * 3;
    avgtime = runtime / num_of_operations;
    // Number of lists is 1
    fprintf(stdout, "list-%s-%s,%d,%d,1,%d,%lld,%lld\n", yieldopts, syncops, num_of_threads, num_of_iterations,
                                            num_of_operations, runtime, avgtime);
}

void signal_handler(int sig){
    if (sig == SIGSEGV){
        fprintf(stderr, "%s: Segmentation fault caught\n", strerror(errno));
        exit(2);
    }
}

int main(int argc, char** argv){
    if (signal(SIGSEGV, signal_handler) == SIG_ERR){
        fprintf(stderr, "Error signaling signal handler; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    static struct option long_options[] =
    {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"yield", required_argument, NULL, 'y'},
        {"sync", required_argument, NULL, 's'},
        {0, 0, 0, 0}
    };
    int c;
    opt_yield = 0;
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
                int len = strlen(optarg);
                if (len > 3){
                    fprintf(stderr, "Incorect usage: --yield=[idl]: maximum 3 usable options: options are (i)nsert, (d)elete, (l)ookup\n");
                    exit(1);
                }
                else {
                    int i;
                    for (i = 0; i < len; i++){
                        if (optarg[i] == 'i') insert_yield = 1;
                        else if (optarg[i] == 'd') delete_yield = 1;
                        else if (optarg[i] == 'l') lookup_yield = 1;
                        else {
                            fprintf(stderr, "Incorrect yield option for --yield=[idl]: options are (i)nsert, (d)elete, (l)ookup\n");
                            exit(1);
                        }
                    }
                }
                break;
            case 's':
                if (strlen(optarg) > 1){
                    fprintf(stderr, "Incorect usage: --sync=[m][s]: maximum 1 usable option: options are (m)utex or (s)pin-lock\n");
                    exit(1);
                }
                else{
                    if (optarg[0] == 'm') mutex_sync = 1;
                    else if (optarg[0] == 's') spinlock_sync = 1;
                    else{
                        fprintf(stderr, "Incorrect sync option '%s' for --sync=[m][s]: options are (m)utex or (s)pin-lock\n", optarg);
                        exit(1);
                    }
                }
                break;
            default:
                fprintf(stderr, "Incorrect usage: ./lab2_list [--threads=n_threads] [--iterations=n_iterations] [--yield=[idl]] [--sync=[m][s]]\n");
                exit(1);
        }
    }
    if (num_of_threads < 1 || num_of_iterations < 1){
        fprintf(stderr, "Incorrect usage: number of threads and iterations must be at least 1\n");
        exit(1);
    }
    if (insert_yield){
        strcat(yieldopts, "i");
        opt_yield |= INSERT_YIELD;
    }
    if (delete_yield){
        strcat(yieldopts, "d");
        opt_yield |= DELETE_YIELD;
    }
    if (lookup_yield){
        strcat(yieldopts, "l");
        opt_yield |= LOOKUP_YIELD;
    }
    if (!insert_yield && !delete_yield && !lookup_yield)
        strcat(yieldopts, "none");

    if (mutex_sync){
        if (pthread_mutex_init(&mutex, NULL) != 0){
            fprintf(stderr, "Error initializing mutex; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
        strcat(syncops, "m");
    }
    else if (spinlock_sync) strcat(syncops, "s");
    else strcat(syncops, "none");

    num_of_elements = num_of_threads * num_of_iterations;

    // Initialize empty circular list
    list = malloc(sizeof(SortedList_t));
    if (list == NULL){
        fprintf(stderr, "Error mallociing empty list; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    list->next = list;
    list->prev = list;
    list->key = NULL;

    // Create and initialize number of elements
    elements = malloc(sizeof(SortedListElement_t) * num_of_elements);
    if (elements == NULL){
        fprintf(stderr, "Error mallocing list elements; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    // For random keys, use current time as seed for randomness
    srand(time(NULL)); 
    int i;
    char *k;
    // "Generate a sequence of random integers between 0 and 25, and
    // convert the integers to English letter characters"
    // From Piazza post https://piazza.com/class/kirz3jfa5jv7l7?cid=403 
    // by TA Tengyu Liu
    for (i = 0; i < num_of_elements; i++){
        // Create random key for each element
        k = malloc(sizeof(char));
        if (k == NULL){
            fprintf(stderr, "Error mallocing a random key; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
        // Referenced https://stackoverflow.com/questions/19724346/generate-random-characters-in-c
        *k = (rand() % 26) + 'a';
        elements[i].key = k;
    }

    struct timespec start_time, end_time;
    // Note starting time for run
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0){
        fprintf(stderr, "Error getting start time of run; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    // Create threads with thread ids (so the correct current thread inserts, looks up,
    // and deletes its own elements)
    pthread_t threads[num_of_threads];
    int tid[num_of_threads];
    for (i = 0; i < num_of_threads; i++){
        tid[i] = i;
        if (pthread_create(&(threads[i]), NULL, run_threads, &(tid[i])) != 0){
            fprintf(stderr, "Error creating threads wtih thread ids; strerror reports: %s\n", strerror(errno));
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
    // Check that length of list is 0
    // If length is not 0, then there is corruption
    if (SortedList_length(list) != 0){
        fprintf(stderr, "Corruption: length of list is not 0 at end of run\n");
        exit(2);
    }
    if (mutex_sync){
        if (pthread_mutex_destroy(&mutex) != 0){
            fprintf(stderr, "Error destroying mutex; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
    }
    // Free malloc'd list and elements
    free(list);
    free(elements);
    runtime = get_nanosec_from_timespec(&end_time) - get_nanosec_from_timespec(&start_time);
    record_data();
    exit(0);
}