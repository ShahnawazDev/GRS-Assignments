/**
 * MT25042_Part_B_workers.h
 * Graduate Systems (CSE638) - PA01: Processes and Threads
 * 
 * Header file for worker functions (cpu, mem, io)
 * 
 * AI Declaration: This code structure was generated with AI assistance.
 */

#ifndef WORKERS_H
#define WORKERS_H

/**
 * LOOP_COUNT: Last digit of roll number × 10^3
 * If last digit is 0, use 9 instead.
 * 
 * IMPORTANT: Update this value based on your roll number!
 * Example: If roll number is MT25123, last digit is 3, so LOOP_COUNT = 3000
 *          If roll number is MT25120, last digit is 0, so LOOP_COUNT = 9000
 */
#define LOOP_COUNT 2000  /* TODO: Update based on your roll number's last digit × 1000 */

/**
 * CPU-intensive worker function
 * Performs complex mathematical calculations (computing pi using Leibniz series)
 * Spends majority of time in CPU-bound computation
 */
void worker_cpu(void);

/**
 * Memory-intensive worker function
 * Allocates large arrays and performs memory-bound operations
 * Bottlenecked by memory bandwidth and cache efficiency
 */
void worker_mem(void);

/**
 * I/O-intensive worker function
 * Performs disk read/write operations
 * Spends most time waiting for I/O operations to complete
 */
void worker_io(void);

#endif /* WORKERS_H */
