/**
 * MT25042_Part_A_Program_B.c
 * Graduate Systems (CSE638) - PA01: Processes and Threads
 * 
 * Program B: Creates 2 threads using pthread
 * Each thread executes a worker function (cpu, mem, or io)
 * 
 * Usage: ./program_b <worker_type> [num_threads]
 *        worker_type: cpu, mem, or io
 *        num_threads: number of threads (default: 2)
 * 
 * AI Declaration: This code structure was generated with AI assistance.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "MT25042_Part_B_workers.h"

#define DEFAULT_NUM_THREADS 2

/* Thread argument structure */
typedef struct {
    int thread_id;
    char worker_type[10];
} thread_args_t;

/**
 * Thread function that executes the appropriate worker
 * @param arg Pointer to thread_args_t structure
 * @return NULL
 */
void *thread_function(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    
    printf("  [Thread %d, TID: %lu] Starting '%s' worker\n", 
           args->thread_id, (unsigned long)pthread_self(), args->worker_type);

    /* Execute the appropriate worker function based on type */
    if (strcmp(args->worker_type, "cpu") == 0) {
        worker_cpu();
    } 
    else if (strcmp(args->worker_type, "mem") == 0) {
        worker_mem();
    } 
    else if (strcmp(args->worker_type, "io") == 0) {
        worker_io();
    }

    printf("  [Thread %d, TID: %lu] Finished '%s' worker\n", 
           args->thread_id, (unsigned long)pthread_self(), args->worker_type);

    return NULL;
}

int main(int argc, char *argv[]) {
    /* Validate command line arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <worker_type> [num_threads]\n", argv[0]);
        fprintf(stderr, "  worker_type: cpu, mem, or io\n");
        fprintf(stderr, "  num_threads: number of threads (default: 2)\n");
        return EXIT_FAILURE;
    }

    /* Parse worker type */
    char *worker_type = argv[1];

    /* Parse number of threads (optional, default is 2) */
    int num_threads = DEFAULT_NUM_THREADS;
    if (argc >= 3) {
        num_threads = atoi(argv[2]);
        if (num_threads < 1) {
            fprintf(stderr, "Error: num_threads must be at least 1\n");
            return EXIT_FAILURE;
        }
    }

    /* Validate worker type */
    if (strcmp(worker_type, "cpu") != 0 && 
        strcmp(worker_type, "mem") != 0 && 
        strcmp(worker_type, "io") != 0) {
        fprintf(stderr, "Error: Invalid worker type '%s'\n", worker_type);
        fprintf(stderr, "Valid types: cpu, mem, io\n");
        return EXIT_FAILURE;
    }

    printf("[Main Thread] Creating %d threads for '%s' worker\n", 
           num_threads, worker_type);

    /* Allocate arrays for thread handles and arguments */
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    thread_args_t *args = malloc(num_threads * sizeof(thread_args_t));

    if (threads == NULL || args == NULL) {
        perror("malloc failed");
        return EXIT_FAILURE;
    }

    /* Create threads */
    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i + 1;
        strncpy(args[i].worker_type, worker_type, sizeof(args[i].worker_type) - 1);
        args[i].worker_type[sizeof(args[i].worker_type) - 1] = '\0';

        int ret = pthread_create(&threads[i], NULL, thread_function, &args[i]);
        if (ret != 0) {
            fprintf(stderr, "Error: pthread_create failed for thread %d\n", i + 1);
            return EXIT_FAILURE;
        }
    }

    /* Wait for all threads to complete */
    printf("[Main Thread] Waiting for %d threads to complete...\n", num_threads);
    
    for (int i = 0; i < num_threads; i++) {
        int ret = pthread_join(threads[i], NULL);
        if (ret != 0) {
            fprintf(stderr, "Error: pthread_join failed for thread %d\n", i + 1);
        } else {
            printf("[Main Thread] Thread %d joined successfully\n", i + 1);
        }
    }

    /* Free allocated memory */
    free(threads);
    free(args);

    printf("[Main Thread] All threads completed.\n");
    return EXIT_SUCCESS;
}
