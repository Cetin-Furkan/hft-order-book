#define _GNU_SOURCE // Must be defined to get CPU_SET, etc.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h> // For usleep
#include <sched.h>  // For CPU affinity

#include "network.h"
#include "itch_protocol.h"
#include "orderbook.h"
#include "spsc_queue.h"
#include "sequencer.h"

// --- Shared State ---
typedef struct {
    SPSCQueue* net_to_seq_queue;
    SPSCQueue* seq_to_proc_queue;
    Sequencer* sequencer;
    OrderBook* book;
    int sockfd;
} SharedState;

// --- Helper Functions & Message Handlers ---
void print_stock_symbol(const char* symbol) {
    char temp[9];
    memcpy(temp, symbol, 8);
    temp[8] = '\0';
    printf("%s", temp);
}

void handle_add_order(OrderBook* book, void* buffer) {
    AddOrderMessage* msg = (AddOrderMessage*)buffer;
    // This is the key change: we now call the matching engine.
    orderbook_process_order(book, msg->order_ref_num, ntohl(msg->price), ntohl(msg->shares), (msg->buy_sell_indicator == 'B'));
}

void handle_order_executed(OrderBook* book, void* buffer) {
    // In a real system, an 'E' message from the exchange would likely
    // be a trade confirmation against one of our own orders.
    // For our simulation, we'll treat it as a simple execution.
    OrderExecutedMessage* msg = (OrderExecutedMessage*)buffer;
    orderbook_execute(book, msg->order_ref_num, ntohl(msg->executed_shares));
}

void handle_order_cancel(OrderBook* book, void* buffer) {
    OrderCancelMessage* msg = (OrderCancelMessage*)buffer;
    orderbook_cancel(book, msg->order_ref_num);
}

// --- Thread Functions ---
void* network_thread_func(void* arg) {
    SharedState* state = (SharedState*)arg;
    printf("Network thread started on CPU core 1.\n");
    fflush(stdout);
    while (1) {
        QueueItem item;
        item.size = recvfrom(state->sockfd, item.data, BUF_SIZE, 0, NULL, NULL);
        if (item.size > 0) {
            while (!spsc_queue_enqueue(state->net_to_seq_queue, &item)) {}
        }
    }
    return NULL;
}

void* sequencer_thread_func(void* arg) {
    SharedState* state = (SharedState*)arg;
    printf("Sequencer thread started on CPU core 2.\n");
    fflush(stdout);
    while (1) {
        // This is the robust pattern: try to do work, and only sleep
        // if there was no work to do.
        if (!sequencer_run_once(state->sequencer)) {
            usleep(10);
        }
    }
    return NULL;
}

void* processing_thread_func(void* arg) {
    SharedState* state = (SharedState*)arg;
    printf("Processing thread started on CPU core 3.\n");
    fflush(stdout);
    QueueItem item;
    while (1) {
        if (spsc_queue_dequeue(state->seq_to_proc_queue, &item)) {
            MessageHeader* header = (MessageHeader*)item.data;
            switch (header->message_type) {
                case 'A':
                    if (item.size >= sizeof(AddOrderMessage)) handle_add_order(state->book, item.data);
                    break;
                case 'E':
                    if (item.size >= sizeof(OrderExecutedMessage)) handle_order_executed(state->book, item.data);
                    break;
                case 'X':
                    if (item.size >= sizeof(OrderCancelMessage)) handle_order_cancel(state->book, item.data);
                    break;
            }
        } else {
            usleep(10);
        }
    }
    return NULL;
}

// --- Main Orchestrator ---
int main(void) {
    printf("Starting Hermes...\n");
    fflush(stdout);

    SharedState* state = malloc(sizeof(SharedState));
    state->book = malloc(sizeof(OrderBook));
    state->net_to_seq_queue = malloc(sizeof(SPSCQueue));
    state->seq_to_proc_queue = malloc(sizeof(SPSCQueue));
    state->sequencer = malloc(sizeof(Sequencer));

    orderbook_init(state->book);
    spsc_queue_init(state->net_to_seq_queue);
    spsc_queue_init(state->seq_to_proc_queue);
    sequencer_init(state->sequencer, state->net_to_seq_queue, state->seq_to_proc_queue);

    state->sockfd = setup_multicast_socket("233.1.1.1", 5000);

    pthread_t network_thread, sequencer_thread, processing_thread;
    pthread_attr_t attr;
    cpu_set_t cpus;
    pthread_attr_init(&attr);

    CPU_ZERO(&cpus);
    CPU_SET(1, &cpus);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
    pthread_create(&network_thread, &attr, network_thread_func, state);

    CPU_ZERO(&cpus);
    CPU_SET(2, &cpus);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
    pthread_create(&sequencer_thread, &attr, sequencer_thread_func, state);

    CPU_ZERO(&cpus);
    CPU_SET(3, &cpus);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
    pthread_create(&processing_thread, &attr, processing_thread_func, state);

    pthread_join(network_thread, NULL);
    pthread_join(sequencer_thread, NULL);
    pthread_join(processing_thread, NULL);

    return 0;
}
