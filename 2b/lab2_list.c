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

int num_of_threads = 1, num_of_iterations = 1, num_of_lists = 1; // default 1
int num_of_operations; // num_of_threads * num_of_iterations * 3
int num_of_elements;
int insert_yield = 0, delete_yield = 0, lookup_yield = 0;
int opt_yield;

// To create testname, length of yieldopts or syncops < 5 (longest name is "none")
// "list" and "-" are printed separately in record_data function
char yieldopts[5];
char syncops[5];

long long runtime = 0, avgtime = 0, totalwaittime = 0;

// sync options
int mutex_sync = 0, spinlock_sync = 0;
// mutex and lock for spin-lock
pthread_mutex_t* mutexes;
int* locks;

SortedList_t* list;
SortedListElement_t* elements;

// Refernced TA Tianxiang Li's slides
static unsigned long get_nanosec_from_timespec(struct timespec *spec){
    unsigned long ret= spec->tv_sec; // seconds
    return ret * 1000000000 + spec->tv_nsec; // nanoseconds
}

int hash_function(const char* key){
    return ((int) *key) % num_of_lists;
}

void *run_threads(void* arg){
    int i;
    // arg is the thread id, so the thread only handles own elements
    // each thread inserts num_of_iterations elements
    int current = *((int*) arg) * num_of_iterations;
    SortedListElement_t* to_be_deleted;
    struct timespec start, end;
    long long waittime = 0;
    // Insert into sublists of single, shared list
    for (i = current; i < current + num_of_iterations; i++){
        int hash = hash_function(elements[i].key);
        if (mutex_sync){
            clock_gettime(CLOCK_MONOTONIC, &start);
            pthread_mutex_lock(&mutexes[hash]);
            clock_gettime(CLOCK_MONOTONIC, &end);
            SortedList_insert(&list[hash], &elements[i]);
            pthread_mutex_unlock(&mutexes[hash]);
            waittime += (get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&start));
        }
        else if (spinlock_sync){
            clock_gettime(CLOCK_MONOTONIC, &start);
            while (__sync_lock_test_and_set(&locks[hash], 1));
            clock_gettime(CLOCK_MONOTONIC, &end);
            SortedList_insert(&list[hash], &elements[i]);
            __sync_lock_release(&locks[hash]);
            waittime += (get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&start));
        }
        else{
            SortedList_insert(&list[hash], &elements[i]);
        }
    }
    // Get list length (and check for corruption)
    // If length of list is less than the num of iterations or -1, there is corruption
    int length = 0, sublength = 0;
    for (i = 0; i < num_of_lists; i++){
        if (mutex_sync){
            clock_gettime(CLOCK_MONOTONIC, &start);
            pthread_mutex_lock(&mutexes[i]);
            clock_gettime(CLOCK_MONOTONIC, &end);
            sublength = SortedList_length(&list[i]);
            if (sublength == -1){
                fprintf(stderr, "Corruption: length of list is -1; strerror reports: %s\n", strerror(errno));
                exit(2);
            }
            length += sublength;
            pthread_mutex_unlock(&mutexes[i]);
            waittime += (get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&start));
        }
        else if (spinlock_sync){
            clock_gettime(CLOCK_MONOTONIC, &start);
            while (__sync_lock_test_and_set(&locks[i], 1));
            clock_gettime(CLOCK_MONOTONIC, &end);
            sublength = SortedList_length(&list[i]);
            if (sublength == -1){
                fprintf(stderr, "Corruption: length of list is -1; strerror reports: %s\n", strerror(errno));
                exit(2);
            }
            length += sublength;
            __sync_lock_release(&locks[i]);
            waittime += (get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&start));
        }
        else {
            sublength = SortedList_length(&list[i]);
            if (sublength == -1){
                fprintf(stderr, "Corruption: length of list is -1; strerror reports: %s\n", strerror(errno));
                exit(2);
            }
            length += sublength;
        }
    }
    if (length < num_of_iterations){
        fprintf(stderr, "Corruption: length of list is less than number of iterations; strerror reports: %s\n", strerror(errno));
        exit(2);
    }

    // Lookup and delete each key that was previously inserted
    // If corruption is found (e.g., a key that was inserted cannot be found), exit(2)
    for (i = current; i < current + num_of_iterations; i++){
        int hash = hash_function(elements[i].key);
        if (mutex_sync){
            clock_gettime(CLOCK_MONOTONIC, &start);
            pthread_mutex_lock(&mutexes[hash]);
            clock_gettime(CLOCK_MONOTONIC, &end);
            to_be_deleted = SortedList_lookup(&list[hash], elements[i].key);
            if (to_be_deleted == NULL){
                fprintf(stderr, "Corruption: cannot lookup previously inserted key; strerror reports: %s\n", strerror(errno));
                exit(2);
            }
            // SortedList_delete returns 1 on corruption
            if (SortedList_delete(to_be_deleted)){
                fprintf(stderr, "Corruption: cannot delete previously found key; strerror reports: %s\n", strerror(errno));
                exit(2);     
            }
            pthread_mutex_unlock(&mutexes[hash]);
            waittime += (get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&start));
        }
        else if (spinlock_sync){
            clock_gettime(CLOCK_MONOTONIC, &start);
            while (__sync_lock_test_and_set(&locks[hash], 1));
            clock_gettime(CLOCK_MONOTONIC, &end);
            to_be_deleted = SortedList_lookup(&list[hash], elements[i].key);
            if (to_be_deleted == NULL){
                fprintf(stderr, "Corruption: cannot lookup previously inserted key; strerror reports: %s\n", strerror(errno));
                exit(2);
            }
            // SortedList_delete returns 1 on corruption
            if (SortedList_delete(to_be_deleted)){
                fprintf(stderr, "Corruption: cannot delete previously found key; strerror reports: %s\n", strerror(errno));
                exit(2);     
            }
            __sync_lock_release(&locks[hash]);
            waittime += (get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&start));
        }
        else{
            to_be_deleted = SortedList_lookup(&list[hash], elements[i].key);
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
    return (void*) waittime;
}

void record_data(){
    // 3 operations: insert, delete, lookup
    num_of_operations = num_of_threads * num_of_iterations * 3;
    avgtime = runtime / num_of_operations;
    totalwaittime /= num_of_operations;
    fprintf(stdout, "list-%s-%s,%d,%d,%d,%d,%lld,%lld,%lld\n", yieldopts, syncops, num_of_threads, num_of_iterations,
                                            num_of_lists, num_of_operations, runtime, avgtime, totalwaittime);
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
        {"lists", required_argument, NULL, 'l'},
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
            case 'l':
                num_of_lists = atoi(optarg);
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

    num_of_elements = num_of_threads * num_of_iterations;

    // Initialize empty circular list of sublists
    list = malloc(sizeof(SortedList_t) * num_of_lists);
    if (list == NULL){
        fprintf(stderr, "Error mallociing list of sublists; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    int i;
    for (i = 0; i < num_of_lists; i++){
        list[i].next = &list[i];
        list[i].prev = &list[i];
        list[i].key = NULL;
    }
    // Create mutexes and spin-locks for each sublist
    if (mutex_sync){
        mutexes = malloc(sizeof(pthread_mutex_t) * num_of_lists);
        if (mutexes == NULL){
            fprintf(stderr, "Error mallocing mutexes for sublists; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
        for (i = 0; i < num_of_lists; i++){
            if (pthread_mutex_init(&mutexes[i], NULL) != 0){
                fprintf(stderr, "Error initializing mutexes for sublists; strerror reports: %s\n", strerror(errno));
                exit(1);
            }
        }
        strcat(syncops, "m");
    }
    else if (spinlock_sync){
        locks = malloc(sizeof(int) * num_of_lists);
        if (locks == NULL){
            fprintf(stderr, "Error mallocing spin-locks for sublists; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
        for (i = 0; i < num_of_lists; i++){
            locks[i] = 0;
        }
        strcat(syncops, "s");
    }
    else
        strcat(syncops, "none");

    // Create and initialize number of elements
    elements = malloc(sizeof(SortedListElement_t) * num_of_elements);
    if (elements == NULL){
        fprintf(stderr, "Error mallocing list elements; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    // For random keys, use current time as seed for randomness
    srand(time(NULL)); 
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
    // Wait for all threads to complete and join, add each thread's waittime to thread total
    void** waittime = malloc(sizeof(void**));
    for (i = 0; i < num_of_threads; i++){
        if (pthread_join(threads[i], waittime) != 0){
            fprintf(stderr, "Error joining threads; strerror reports: %s\n", strerror(errno));
            exit(1);
        }
        totalwaittime += (long long) *waittime;
    }
    // Note ending time of run
    if (clock_gettime(CLOCK_MONOTONIC, &end_time) != 0){
        fprintf(stderr, "Error getting end time of run; strerror reports: %s\n", strerror(errno));
        exit(1);
    }
    // Check that length of list is 0
    // If length is not 0, then there is corruption
    int length = 0;
    for (i = 0; i < num_of_lists; i++){
        length += SortedList_length(&list[i]);
    }
    if (length != 0){
        fprintf(stderr, "Corruption: length of list is not 0 at end of run\n");
        exit(2);
    }
    free(list);
    free(elements);
    if (mutex_sync){
        for (i = 0; i < num_of_lists; i++){
            if (pthread_mutex_destroy(&mutexes[i]) != 0){
                fprintf(stderr, "Error destroying mutex; strerror reports: %s\n", strerror(errno));
                exit(1);
            }
        }
        free(mutexes);
    }
    else if (spinlock_sync)
        free(locks);
    free(waittime);
    runtime = get_nanosec_from_timespec(&end_time) - get_nanosec_from_timespec(&start_time);
    record_data();
    exit(0);
}