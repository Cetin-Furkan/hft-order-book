#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <time.h>
#include <immintrin.h>
#include "util/memory_arena.h"
#include "util/hptimer.h"
#include "core/order_book.h"
#include "core/transaction_log.h"
#include "network.h"
#include "platform/cpu_dispatch.h"

#define GIGABYTES(N) ((size_t)N * 1024 * 1024 * 1024)
#define RING_BUFFER_CAPACITY 1024

// --- Thread Affinity Configuration ---
#define PROCESSING_THREAD_CORE 1
#define NETWORK_THREAD_CORE 2
#define LOGGER_THREAD_CORE 3

// Global running flag for the whole application
atomic_bool g_running = true;

void set_thread_affinity(pthread_t thread, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        fprintf(stderr, "Error: pthread_setaffinity_np for core %d failed\n", core_id);
    } else {
        printf("Successfully pinned thread %lu to CPU Core %d\n", (unsigned long)thread, core_id);
    }
}

void process_message(OrderBook* book, const ProtocolMessage* msg) {
    uint64_t start_cycles, end_cycles;
    switch (msg->msg_type) {
        case 'N':
            start_cycles = rdtsc();
            order_book_add_order(book, msg->new_order.order_id, msg->new_order.side, msg->new_order.price, msg->new_order.quantity);
            end_cycles = rdtsc();
            printf("Processed New Order %lu in %lu CPU cycles.\n", msg->new_order.order_id, end_cycles - start_cycles);
            order_book_print(book);
            break;
        case 'C':
            start_cycles = rdtsc();
            order_book_cancel_order(book, msg->cancel_order.order_id);
            end_cycles = rdtsc();
            printf("Processed Cancel Order %lu in %lu CPU cycles.\n", msg->cancel_order.order_id, end_cycles - start_cycles);
            order_book_print(book);
            break;
        case 'X':
            printf("Shutdown signal received. Terminating application.\n");
            atomic_store(&g_running, false);
            break;
        default:
            fprintf(stderr, "Unknown message type: %c\n", msg->msg_type);
            break;
    }
}

void system_warmup(OrderBook* book, Arena* arena) {
    printf("\n--- System Cache Warmup ---\n");
    volatile uint64_t dummy_read = 0;
    for (int i = 0; i < MAX_PRICE_LEVELS; ++i) {
        dummy_read += book->bids[i].price;
        dummy_read += book->asks[i].price;
    }
    order_book_add_order(book, 0, 'B', 1, 1);
    order_book_cancel_order(book, 0);
    arena_reset(arena);
    order_book_init(book, arena, book->logger_queue);
    printf("Cache warmup complete.\n");
}

int main(void) {
    cpu_features_init();
    printf("--- HFT Order Book Starting ---\n");

    // --- Initialize Subsystems ---
    Arena main_arena;
    if (arena_init(&main_arena, GIGABYTES(1)) != 0) { return EXIT_FAILURE; }

    SPSC_RingBuffer* net_rb = spsc_ring_buffer_init(RING_BUFFER_CAPACITY, sizeof(ProtocolMessage));
    if (!net_rb) {
        fprintf(stderr, "Failed to initialize network ring buffer.\n");
        arena_destroy(&main_arena);
        return EXIT_FAILURE;
    }
    
    SPSC_RingBuffer* log_rb = spsc_ring_buffer_init(RING_BUFFER_CAPACITY, sizeof(LogEntry));
    if (!log_rb) {
        fprintf(stderr, "Failed to initialize logger ring buffer.\n");
        spsc_ring_buffer_destroy(net_rb);
        arena_destroy(&main_arena);
        return EXIT_FAILURE;
    }

    OrderBook book;
    if (order_book_init(&book, &main_arena, log_rb) != 0) {
        arena_destroy(&main_arena);
        return EXIT_FAILURE;
    }

    FILE* log_file = fopen("trades.log", "a");
    if (!log_file) {
        perror("fopen trades.log");
        return EXIT_FAILURE;
    }

    // --- Setup and Start Threads ---
    NetworkThreadState net_state = { .ring_buffer = net_rb, .running = &g_running };
    pthread_t network_tid;
    if (pthread_create(&network_tid, NULL, network_thread_main, &net_state) != 0) {
        perror("pthread_create network");
        return EXIT_FAILURE;
    }

    LoggerThreadState log_state = { .log_file = log_file, .ring_buffer = log_rb, .running = &g_running };
    pthread_t logger_tid;
    if (pthread_create(&logger_tid, NULL, logger_thread_main, &log_state) != 0) {
        perror("pthread_create logger");
        return EXIT_FAILURE;
    }

    // --- Set Thread Affinities ---
    set_thread_affinity(network_tid, NETWORK_THREAD_CORE);
    set_thread_affinity(logger_tid, LOGGER_THREAD_CORE);
    set_thread_affinity(pthread_self(), PROCESSING_THREAD_CORE);

    system_warmup(&book, &main_arena);

    // --- Main Processing Loop (Consumer) ---
    printf("\nMain processing loop started. Waiting for messages...\n");
    ProtocolMessage msg;
    while (atomic_load(&g_running)) {
        if (spsc_ring_buffer_pop(net_rb, &msg)) {
            process_message(&book, &msg);
        } else {
            _mm_pause();
        }
    }

    // --- Shutdown ---
    printf("Waiting for threads to join...\n");
    pthread_join(network_tid, NULL);
    pthread_join(logger_tid, NULL);

    printf("Shutting down gracefully.\n");
    fclose(log_file);
    spsc_ring_buffer_destroy(net_rb);
    spsc_ring_buffer_destroy(log_rb);
    arena_destroy(&main_arena);

    return EXIT_SUCCESS;
}
