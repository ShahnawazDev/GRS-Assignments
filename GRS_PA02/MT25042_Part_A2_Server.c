/**
 * MT25042_Part_A2_Server.c
 * Graduate Systems (CSE638) - PA02: Analysis of Network I/O primitives
 *
 * One-copy TCP server:
 *   Uses sendmsg() with scatter-gather (iovec) to point directly at the
 *   8 heap-allocated message fields.  The kernel gathers them into the
 *   socket buffer in a single pass — eliminating the user-space
 *   serialization copy present in the two-copy baseline.
 *
 *   Remaining copy: user-space buffers → kernel socket buffer (1 copy).
 *
 * Usage: ./a2_server <msg_size> <max_clients>
 *
 * AI Declaration: Asked ChatGPT "How does sendmsg with iovec eliminate
 *   a copy compared to plain send?" and used the explanation to design
 *   this scatter-gather approach.
 */

#include "MT25042_Part_A_Common.h"
#include <sys/uio.h>   /* struct iovec, sendmsg */

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

    printf("[Server T%d] One-copy handler, fd=%d, msg_size=%d\n",
           tid, fd, msg_size);

    /* Build the message with 8 heap-allocated fields */
    message_t *msg = create_message(msg_size);
    if (!msg) { close(fd); return NULL; }

    /*
     * Set up iovec array pointing directly at the 8 heap fields.
     * No intermediate serialization buffer is needed — this removes
     * the first copy that existed in the two-copy version.
     */
    struct iovec iov[NUM_FIELDS];
    for (int i = 0; i < NUM_FIELDS; i++) {
        iov[i].iov_base = msg->fields[i];
        iov[i].iov_len  = msg->field_len;
    }

    struct msghdr mh;
    memset(&mh, 0, sizeof(mh));
    mh.msg_iov    = iov;
    mh.msg_iovlen = NUM_FIELDS;

    /* Send repeatedly until the client disconnects */
    while (1) {
        /*
         * SINGLE COPY: the kernel gathers data from the 8 iovec entries
         * directly into the socket buffer (user → kernel).
         */
        ssize_t n = sendmsg(fd, &mh, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            break;
        }
    }

    printf("[Server T%d] Client disconnected\n", tid);
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

    printf("[Server] One-copy (sendmsg/iovec) on port %d "
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
