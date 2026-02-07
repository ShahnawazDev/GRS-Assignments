/**
 * MT25042_Part_A_Common.h
 * Graduate Systems (CSE638) - PA02: Analysis of Network I/O primitives
 *
 * Common definitions shared across all socket implementations:
 *   - Message structure with 8 dynamically allocated string fields
 *   - Serialization / deserialization helpers
 *   - Timing utilities for throughput & latency measurement
 *   - Network configuration constants
 *
 * AI Declaration: Used ChatGPT to clarify the sendmsg() iovec layout
 *   and MSG_ZEROCOPY completion notification flow.
 */

#ifndef MT25042_PART_A_COMMON_H
#define MT25042_PART_A_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

/* ------------------------------------------------------------------ */
/*  Constants                                                          */
/* ------------------------------------------------------------------ */

#define DEFAULT_PORT       9876
#define DEFAULT_DURATION   10          /* seconds each client sends   */
#define NUM_FIELDS         8           /* string fields per message   */
#define BACKLOG            64          /* listen() backlog            */

/* ------------------------------------------------------------------ */
/*  Message structure – 8 heap-allocated string fields                 */
/* ------------------------------------------------------------------ */

typedef struct {
    char *fields[NUM_FIELDS];          /* each malloc'd separately    */
    int   field_len;                   /* bytes per field             */
} message_t;

/* ------------------------------------------------------------------ */
/*  Thread argument passed to each per-client handler                  */
/* ------------------------------------------------------------------ */

typedef struct {
    int  client_fd;
    int  msg_size;                     /* total message size in bytes */
    int  thread_id;
} thread_arg_t;

/* ------------------------------------------------------------------ */
/*  Client-side argument                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *server_ip;
    int         server_port;
    int         msg_size;
    int         duration_sec;
    int         thread_id;
    /* results written back by the thread */
    double      throughput_bps;
    double      avg_latency_us;
    long long   total_bytes;
    long        total_messages;
} client_arg_t;

/* ------------------------------------------------------------------ */
/*  Timing helpers                                                     */
/* ------------------------------------------------------------------ */

static inline double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static inline double elapsed_us(struct timespec *start, struct timespec *end)
{
    return (end->tv_sec - start->tv_sec) * 1e6 +
           (end->tv_nsec - start->tv_nsec) / 1e3;
}

/* ------------------------------------------------------------------ */
/*  Message allocation & initialisation                                */
/* ------------------------------------------------------------------ */

/**
 * create_message – allocates a message_t whose total payload is
 *                  approximately `total_size` bytes (split evenly
 *                  across NUM_FIELDS fields).
 */
static inline message_t *create_message(int total_size)
{
    message_t *msg = (message_t *)malloc(sizeof(message_t));
    if (!msg) { perror("malloc message_t"); return NULL; }

    int per_field = total_size / NUM_FIELDS;
    if (per_field < 1) per_field = 1;
    msg->field_len = per_field;

    for (int i = 0; i < NUM_FIELDS; i++) {
        msg->fields[i] = (char *)malloc(per_field);
        if (!msg->fields[i]) { perror("malloc field"); return NULL; }
        /* fill with a repeating pattern so the payload is not all zeros */
        memset(msg->fields[i], 'A' + (i % 26), per_field);
    }
    return msg;
}

static inline void free_message(message_t *msg)
{
    if (!msg) return;
    for (int i = 0; i < NUM_FIELDS; i++)
        free(msg->fields[i]);
    free(msg);
}

/* ------------------------------------------------------------------ */
/*  Serialization: pack all 8 fields into one contiguous buffer        */
/*  (used by the two-copy baseline that needs a flat buffer for send)  */
/* ------------------------------------------------------------------ */

static inline char *serialize_message(const message_t *msg, int *out_len)
{
    int total = msg->field_len * NUM_FIELDS;
    char *buf = (char *)malloc(total);
    if (!buf) { perror("malloc serialize"); return NULL; }

    for (int i = 0; i < NUM_FIELDS; i++)
        memcpy(buf + i * msg->field_len, msg->fields[i], msg->field_len);

    *out_len = total;
    return buf;
}

/* ------------------------------------------------------------------ */
/*  Reliable send / recv wrappers                                      */
/* ------------------------------------------------------------------ */

static inline ssize_t send_all(int fd, const void *buf, size_t len, int flags)
{
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, (const char *)buf + sent, len - sent, flags);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return n;   /* error or peer closed */
        }
        sent += (size_t)n;
    }
    return (ssize_t)sent;
}

static inline ssize_t recv_all(int fd, void *buf, size_t len, int flags)
{
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(fd, (char *)buf + got, len - got, flags);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return n;
        }
        got += (size_t)n;
    }
    return (ssize_t)got;
}

/* ------------------------------------------------------------------ */
/*  Socket creation helper                                             */
/* ------------------------------------------------------------------ */

static inline int create_tcp_socket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return fd;
}

#endif /* MT25042_PART_A_COMMON_H */
