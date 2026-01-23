/**
 * MT25042_Part_A_Program_A.c
 * Graduate Systems (CSE638) - PA01: Processes and Threads
 * 
 * Program A: Creates 2 child processes using fork()
 * Each process executes a worker function (cpu, mem, or io)
 * 
 * Usage: ./program_a <worker_type> [num_processes]
 *        worker_type: cpu, mem, or io
 *        num_processes: number of child processes (default: 2)
 * 
 * AI Declaration: This code structure was generated with AI assistance.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "MT25042_Part_B_workers.h"

#define DEFAULT_NUM_PROCESSES 2

int main(int argc, char *argv[]) {
    /* Validate command line arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <worker_type> [num_processes]\n", argv[0]);
        fprintf(stderr, "  worker_type: cpu, mem, or io\n");
        fprintf(stderr, "  num_processes: number of child processes (default: 2)\n");
        return EXIT_FAILURE;
    }

    /* Parse worker type */
    char *worker_type = argv[1];
    
    /* Parse number of processes (optional, default is 2) */
    int num_processes = DEFAULT_NUM_PROCESSES;
    if (argc >= 3) {
        num_processes = atoi(argv[2]);
        if (num_processes < 1) {
            fprintf(stderr, "Error: num_processes must be at least 1\n");
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

    printf("[Parent PID: %d] Creating %d child processes for '%s' worker\n", 
           getpid(), num_processes, worker_type);

    /* Create child processes using fork() */
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            /* Fork failed */
            perror("fork failed");
            return EXIT_FAILURE;
        } 
        else if (pid == 0) {
            /* Child process */
            printf("  [Child %d, PID: %d] Starting '%s' worker\n", 
                   i + 1, getpid(), worker_type);

            /* Execute the appropriate worker function */
            if (strcmp(worker_type, "cpu") == 0) {
                worker_cpu();
            } 
            else if (strcmp(worker_type, "mem") == 0) {
                worker_mem();
            } 
            else if (strcmp(worker_type, "io") == 0) {
                worker_io();
            }

            printf("  [Child %d, PID: %d] Finished '%s' worker\n", 
                   i + 1, getpid(), worker_type);
            
            /* Child exits after completing work */
            exit(EXIT_SUCCESS);
        }
        /* Parent continues to create more children */
    }

    /* Parent waits for all child processes to complete */
    printf("[Parent PID: %d] Waiting for %d children to complete...\n", 
           getpid(), num_processes);
    
    for (int i = 0; i < num_processes; i++) {
        int status;
        pid_t child_pid = wait(&status);
        
        if (child_pid > 0) {
            if (WIFEXITED(status)) {
                printf("[Parent] Child PID %d exited with status %d\n", 
                       child_pid, WEXITSTATUS(status));
            }
        }
    }

    printf("[Parent PID: %d] All children completed.\n", getpid());
    return EXIT_SUCCESS;
}
