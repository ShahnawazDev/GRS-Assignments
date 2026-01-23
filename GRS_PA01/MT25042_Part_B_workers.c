/**
 * MT25042_Part_B_workers.c
 * Graduate Systems (CSE638) - PA01: Processes and Threads
 * 
 * Implementation of worker functions (cpu, mem, io)
 * 
 * Loop count is determined by: last digit of roll number × 10^3
 * If last digit is 0, use 9 instead.
 * 
 * AI Declaration: This code structure was generated with AI assistance.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "MT25042_Part_B_workers.h"

/* Memory size for memory-intensive operations (16 MB per array) */
#define MEM_ARRAY_SIZE (16 * 1024 * 1024)

/* File size for I/O operations (1 MB per write) */
#define IO_BLOCK_SIZE (1024 * 1024)

/* Temporary file prefix for I/O operations */
#define IO_TEMP_FILE_PREFIX "/tmp/pa01_io_worker_"

/**
 * CPU-intensive worker function
 * 
 * This function performs heavy CPU-bound calculations:
 * 1. Computes pi using the Leibniz series (slow convergence)
 * 2. Performs trigonometric calculations
 * 3. Executes matrix-like multiplications
 * 
 * The goal is to keep the CPU busy with actual computations,
 * not waiting on memory or I/O.
 */
void worker_cpu(void) {
    volatile double pi = 0.0;          /* volatile prevents optimization */
    volatile double temp = 0.0;
    volatile double result = 0.0;
    
    printf("    [CPU Worker] Starting CPU-intensive work (LOOP_COUNT=%d)\n", LOOP_COUNT);

    for (int i = 0; i < LOOP_COUNT; i++) {
        /* Leibniz series for pi: pi/4 = 1 - 1/3 + 1/5 - 1/7 + ... */
        for (int j = 0; j < 10000; j++) {
            int sign = (j % 2 == 0) ? 1 : -1;
            pi += sign * (4.0 / (2.0 * j + 1));
        }
        
        /* Additional CPU-intensive trigonometric calculations */
        for (int k = 0; k < 1000; k++) {
            temp = sin((double)k * 0.001) * cos((double)k * 0.001);
            temp += tan((double)k * 0.0001 + 0.0001);
            result += temp * temp;
        }
        
        /* Nested loop multiplication (matrix-like operation) */
        for (int m = 0; m < 100; m++) {
            for (int n = 0; n < 100; n++) {
                result += (double)(m * n) / (m + n + 1);
            }
        }
    }

    printf("    [CPU Worker] Completed. Pi approximation: %.10f, Result: %.2f\n", 
           pi, result);
}

/**
 * Memory-intensive worker function
 * 
 * This function performs memory-bound operations:
 * 1. Allocates large arrays (exceeding typical L3 cache)
 * 2. Performs random access patterns to cause cache misses
 * 3. Copies data between arrays (memory bandwidth limited)
 * 4. Sorts data to trigger memory movements
 * 
 * The goal is to stress the memory subsystem, not the CPU.
 */
void worker_mem(void) {
    printf("    [MEM Worker] Starting memory-intensive work (LOOP_COUNT=%d)\n", LOOP_COUNT);

    /* Allocate large arrays (16 MB each - larger than typical L3 cache) */
    unsigned char *array1 = malloc(MEM_ARRAY_SIZE);
    unsigned char *array2 = malloc(MEM_ARRAY_SIZE);
    
    if (array1 == NULL || array2 == NULL) {
        perror("    [MEM Worker] malloc failed");
        if (array1) free(array1);
        if (array2) free(array2);
        return;
    }

    /* Initialize arrays with pattern */
    for (size_t i = 0; i < MEM_ARRAY_SIZE; i++) {
        array1[i] = (unsigned char)(i & 0xFF);
    }

    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        /* Memory copy operation (tests memory bandwidth) */
        memcpy(array2, array1, MEM_ARRAY_SIZE);
        
        /* Random access pattern (causes cache misses) */
        unsigned int seed = iter;
        for (int j = 0; j < 100000; j++) {
            /* Linear congruential generator for pseudo-random index */
            seed = (seed * 1103515245 + 12345) & 0x7fffffff;
            size_t idx1 = seed % MEM_ARRAY_SIZE;
            
            seed = (seed * 1103515245 + 12345) & 0x7fffffff;
            size_t idx2 = seed % MEM_ARRAY_SIZE;
            
            /* Swap elements (random memory access) */
            unsigned char temp = array1[idx1];
            array1[idx1] = array2[idx2];
            array2[idx2] = temp;
        }

        /* Sequential access with modification (prefetcher can help here) */
        for (size_t i = 0; i < MEM_ARRAY_SIZE; i++) {
            array1[i] = (array1[i] + array2[MEM_ARRAY_SIZE - 1 - i]) & 0xFF;
        }
    }

    /* Cleanup */
    free(array1);
    free(array2);

    printf("    [MEM Worker] Completed memory operations.\n");
}

/**
 * I/O-intensive worker function
 * 
 * This function performs I/O-bound operations:
 * 1. Creates temporary files
 * 2. Writes large blocks of data to disk
 * 3. Reads data back from disk
 * 4. Uses fsync() to ensure data is written to disk (not cached)
 * 
 * The goal is to make the process spend most time waiting for I/O,
 * with the CPU sitting idle.
 */
void worker_io(void) {
    printf("    [IO Worker] Starting I/O-intensive work (LOOP_COUNT=%d)\n", LOOP_COUNT);

    /* Create unique temporary filename based on PID and thread ID */
    char filename[256];
    snprintf(filename, sizeof(filename), "%s%d_%lu.tmp", 
             IO_TEMP_FILE_PREFIX, getpid(), (unsigned long)pthread_self());

    /* Allocate buffer for I/O operations */
    char *buffer = malloc(IO_BLOCK_SIZE);
    if (buffer == NULL) {
        perror("    [IO Worker] malloc failed");
        return;
    }

    /* Initialize buffer with pattern */
    for (int i = 0; i < IO_BLOCK_SIZE; i++) {
        buffer[i] = (char)('A' + (i % 26));
    }

    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        /* Open file for writing */
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("    [IO Worker] open for write failed");
            free(buffer);
            return;
        }

        /* Write data to file */
        ssize_t bytes_written = write(fd, buffer, IO_BLOCK_SIZE);
        if (bytes_written < 0) {
            perror("    [IO Worker] write failed");
        }

        /* Force data to disk (bypass OS buffer cache) */
        fsync(fd);
        close(fd);

        /* Open file for reading */
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            perror("    [IO Worker] open for read failed");
            free(buffer);
            return;
        }

        /* Read data from file */
        ssize_t bytes_read = read(fd, buffer, IO_BLOCK_SIZE);
        if (bytes_read < 0) {
            perror("    [IO Worker] read failed");
        }

        close(fd);

        /* Every 100 iterations, print progress */
        if ((iter + 1) % 100 == 0) {
            printf("    [IO Worker] Progress: %d/%d iterations\n", iter + 1, LOOP_COUNT);
        }
    }

    /* Cleanup: remove temporary file */
    unlink(filename);
    free(buffer);

    printf("    [IO Worker] Completed I/O operations.\n");
}
