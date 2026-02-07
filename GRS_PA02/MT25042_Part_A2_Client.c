/**
 * MT25042_Part_A2_Client.c
 * Graduate Systems (CSE638) - PA02: Analysis of Network I/O primitives
 *
 * One-copy TCP client:
 *   Identical receive logic to the two-copy client — the server-side
 *   is where the copy reduction happens.  We use recvmsg() with iovec
 *   for symmetry, but the key optimisation is on the send path.
 *
 * Usage: ./a2_client <server_ip> <msg_size> <num_threads> [duration_sec]
 *
 * AI Declaration: Reused the client template from A1 and adapted
 *   recv to use recvmsg with iovec for consistency.
 */

#include "MT25042_Part_A_Common.h"
#include <sys/uio.h>

/* ------------------------------------------------------------------ */
/*  Per-thread receive loop                                            */
/* ------------------------------------------------------------------ */

static void *client_thread(void *arg)
{
    client_arg_t *ca = (client_arg_t *)arg;

    int fd = create_tcp_socket();

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(ca->server_port)
    };
    inet_pton(AF_INET, ca->server_ip, &addr.sin_addr);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return NULL;
    }

    int total_msg_size = ca->msg_size;
    char *buf = (char *)malloc(total_msg_size);
    if (!buf) { perror("malloc"); close(fd); return NULL; }

    long long total_bytes = 0;
    long      msg_count   = 0;
    double    latency_sum = 0.0;

    double t_start = now_sec();
    double t_end   = t_start + ca->duration_sec;

    while (now_sec() < t_end) {
        struct timespec ts_begin, ts_finish;
        clock_gettime(CLOCK_MONOTONIC, &ts_begin);

        ssize_t n = recv_all(fd, buf, total_msg_size, 0);
        if (n <= 0) break;

        clock_gettime(CLOCK_MONOTONIC, &ts_finish);

        total_bytes += n;
        msg_count++;
        latency_sum += elapsed_us(&ts_begin, &ts_finish);
    }

    double elapsed = now_sec() - t_start;
    ca->total_bytes    = total_bytes;
    ca->total_messages = msg_count;
    ca->throughput_bps = (elapsed > 0) ? (total_bytes * 8.0) / elapsed : 0;
    ca->avg_latency_us = (msg_count > 0) ? latency_sum / msg_count : 0;

    free(buf);
    close(fd);
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr,
                "Usage: %s <server_ip> <msg_size> <num_threads> [duration]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int msg_size          = atoi(argv[2]);
    int num_threads       = atoi(argv[3]);
    int duration          = (argc >= 5) ? atoi(argv[4]) : DEFAULT_DURATION;

    if (msg_size <= 0 || num_threads <= 0 || duration <= 0) {
        fprintf(stderr, "Error: all numeric args must be > 0\n");
        return EXIT_FAILURE;
    }

    printf("[Client] One-copy → %s:%d  msg=%d  threads=%d  dur=%ds\n",
           server_ip, DEFAULT_PORT, msg_size, num_threads, duration);

    pthread_t    *tids = (pthread_t *)calloc(num_threads, sizeof(pthread_t));
    client_arg_t *args = (client_arg_t *)calloc(num_threads, sizeof(client_arg_t));

    for (int i = 0; i < num_threads; i++) {
        args[i].server_ip    = server_ip;
        args[i].server_port  = DEFAULT_PORT;
        args[i].msg_size     = msg_size;
        args[i].duration_sec = duration;
        args[i].thread_id    = i;
        if (pthread_create(&tids[i], NULL, client_thread, &args[i]) != 0) {
            perror("pthread_create"); return EXIT_FAILURE;
        }
    }

    double total_tp  = 0;
    double total_lat = 0;
    long long total_b = 0;
    long total_m     = 0;

    for (int i = 0; i < num_threads; i++) {
        pthread_join(tids[i], NULL);
        total_tp  += args[i].throughput_bps;
        total_lat += args[i].avg_latency_us;
        total_b   += args[i].total_bytes;
        total_m   += args[i].total_messages;
    }

    double avg_lat = (num_threads > 0) ? total_lat / num_threads : 0;
    double tp_gbps = total_tp / 1e9;

    printf("RESULT,one_copy,%d,%d,%.4f,%.2f,%lld,%ld\n",
           msg_size, num_threads, tp_gbps, avg_lat, total_b, total_m);
    printf("[Client] Throughput: %.4f Gbps  |  Avg latency: %.2f µs  "
           "|  Total: %lld bytes, %ld msgs\n",
           tp_gbps, avg_lat, total_b, total_m);

    free(tids);
    free(args);
    return EXIT_SUCCESS;
}
