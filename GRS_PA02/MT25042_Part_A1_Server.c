/**
 * MT25042_Part_A1_Server.c
 * Graduate Systems (CSE638) - PA02: Analysis of Network I/O primitives
 *
 * Two-copy TCP server (baseline):
 *   Copy 1 – serialize 8 heap fields into a contiguous user-space buffer
 *   Copy 2 – send() copies from user buffer into the kernel socket buffer
 *
 * Usage: ./a1_server <msg_size> <max_clients>
 *
 * AI Declaration: Asked ChatGPT "How to write a multithreaded TCP server
 *   in C that uses one thread per client with send/recv?" and adapted
 *   the structure to match the assignment's message format.
 */

#include "MT25042_Part_A_Common.h"

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

    printf("[Server T%d] Handling client on fd %d, msg_size=%d\n",
           tid, fd, msg_size);

    /* Build the message with 8 heap-allocated string fields */
    message_t *msg = create_message(msg_size);
    if (!msg) { close(fd); return NULL; }

    /*
     * COPY 1: serialize the 8 separate heap buffers into one
     * contiguous buffer (user-space → user-space copy).
     */
    int buf_len = 0;
    char *buf = serialize_message(msg, &buf_len);
    if (!buf) { free_message(msg); close(fd); return NULL; }

    /* Send until the client disconnects or an error occurs */
    while (1) {
        /*
         * COPY 2: send() copies from user buffer → kernel socket buffer
         * (user-space → kernel-space copy).
         */
        ssize_t n = send_all(fd, buf, buf_len, 0);
        if (n <= 0) break;
    }

    printf("[Server T%d] Client disconnected\n", tid);
    free(buf);
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

    printf("[Server] Two-copy baseline listening on port %d "
           "(msg_size=%d, max_clients=%d)\n",
           DEFAULT_PORT, msg_size, max_clients);

    pthread_t *threads = (pthread_t *)calloc(max_clients, sizeof(pthread_t));
    int tcount = 0;

    while (tcount < max_clients) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int cfd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (cfd < 0) {
            perror("accept");
            continue;
        }

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

    /* Wait for all handler threads to finish */
    for (int i = 0; i < tcount; i++)
        pthread_join(threads[i], NULL);

    free(threads);
    close(server_fd);
    printf("[Server] Shutdown complete\n");
    return EXIT_SUCCESS;
}
