/**
 * MT25042_Part_A3_Server.c
 * Graduate Systems (CSE638) - PA02: Analysis of Network I/O primitives
 *
 * Zero-copy TCP server:
 *   Uses sendmsg() with MSG_ZEROCOPY.  The kernel pins the user-space
 *   pages and performs DMA directly from them, avoiding any copy into
 *   kernel socket buffers.
 *
 *   After each send the server must drain completion notifications from
 *   the socket error queue (MSG_ERRQUEUE) so the kernel can unpin the
 *   pages.
 *
 * Usage: ./a3_server <msg_size> <max_clients>
 *
 * AI Declaration: Asked ChatGPT "How to use MSG_ZEROCOPY with sendmsg
 *   in Linux and handle the completion notification on MSG_ERRQUEUE?"
 *   The error-queue polling logic is based on that explanation.
 */

#define _GNU_SOURCE
#include "MT25042_Part_A_Common.h"
#include <sys/uio.h>
#include <linux/errqueue.h>

/* ------------------------------------------------------------------ */
/*  Drain zero-copy completion notifications                           */
/* ------------------------------------------------------------------ */

static void drain_completions(int fd)
{
    char cbuf[128];
    struct msghdr mh;
    memset(&mh, 0, sizeof(mh));
    mh.msg_control    = cbuf;
    mh.msg_controllen = sizeof(cbuf);

    /* Non-blocking drain – consume all pending notifications */
    while (1) {
        int ret = recvmsg(fd, &mh, MSG_ERRQUEUE | MSG_DONTWAIT);
        if (ret < 0) break;   /* EAGAIN / no more notifications */
    }
}

/* ------------------------------------------------------------------ */
/*  Per-client handler thread                                          */
/* ------------------------------------------------------------------ */

static void *handle_client(void *arg)
{
    thread_arg_t *ta  = (thread_arg_t *)arg;
    int fd            = ta->client_fd;
    int msg_size      = ta->msg_size;
    int tid           = ta->thread_id;
    free(ta);

    printf("[Server T%d] Zero-copy handler, fd=%d, msg_size=%d\n",
           tid, fd, msg_size);

    /* Enable MSG_ZEROCOPY on this socket */
    int one = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_ZEROCOPY, &one, sizeof(one)) < 0) {
        perror("setsockopt SO_ZEROCOPY");
        /* Fall back – some kernels / veth pairs may not support it */
        fprintf(stderr, "[Server T%d] WARNING: SO_ZEROCOPY not supported, "
                "falling back to normal sendmsg\n", tid);
    }

    /* Build message with 8 heap-allocated fields */
    message_t *msg = create_message(msg_size);
    if (!msg) { close(fd); return NULL; }

    /* iovec pointing directly at the heap fields (same as one-copy) */
    struct iovec iov[NUM_FIELDS];
    for (int i = 0; i < NUM_FIELDS; i++) {
        iov[i].iov_base = msg->fields[i];
        iov[i].iov_len  = msg->field_len;
    }

    struct msghdr mh;
    memset(&mh, 0, sizeof(mh));
    mh.msg_iov    = iov;
    mh.msg_iovlen = NUM_FIELDS;

    long send_count = 0;

    while (1) {
        /*
         * ZERO COPY: the kernel pins user-space pages and arranges
         * DMA directly from them — no copy into kernel socket buffers.
         */
        ssize_t n = sendmsg(fd, &mh, MSG_ZEROCOPY);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == ENOBUFS) {
                /* Back-pressure: drain completions and retry */
                drain_completions(fd);
                usleep(100);
                continue;
            }
            break;
        }
        if (n == 0) break;

        send_count++;
        /* Periodically drain completion notifications */
        if (send_count % 64 == 0)
            drain_completions(fd);
    }

    /* Final drain */
    drain_completions(fd);

    printf("[Server T%d] Client disconnected (sent %ld msgs)\n",
           tid, send_count);
    free_message(msg);
    close(fd);
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Main – listen, accept, spawn threads                               */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <msg_size> <max_clients>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int msg_size    = atoi(argv[1]);
    int max_clients = atoi(argv[2]);

    if (msg_size <= 0 || max_clients <= 0) {
        fprintf(stderr, "Error: msg_size and max_clients must be > 0\n");
        return EXIT_FAILURE;
    }

    int server_fd = create_tcp_socket();

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(DEFAULT_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }

    printf("[Server] Zero-copy (MSG_ZEROCOPY) on port %d "
           "(msg_size=%d, max_clients=%d)\n",
           DEFAULT_PORT, msg_size, max_clients);

    pthread_t *threads = (pthread_t *)calloc(max_clients, sizeof(pthread_t));
    int tcount = 0;

    while (tcount < max_clients) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int cfd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (cfd < 0) { perror("accept"); continue; }

        printf("[Server] Accepted client %d from %s:%d\n",
               tcount, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        thread_arg_t *ta = (thread_arg_t *)malloc(sizeof(thread_arg_t));
        ta->client_fd = cfd;
        ta->msg_size  = msg_size;
        ta->thread_id = tcount;

        if (pthread_create(&threads[tcount], NULL, handle_client, ta) != 0) {
            perror("pthread_create");
            free(ta);
            close(cfd);
            continue;
        }
        tcount++;
    }

    for (int i = 0; i < tcount; i++)
        pthread_join(threads[i], NULL);

    free(threads);
    close(server_fd);
    printf("[Server] Shutdown complete\n");
    return EXIT_SUCCESS;
}
