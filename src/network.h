#ifndef NETWORK_H
#define NETWORK_H

#include "core/order_book.h"
#include "concurrent/spsc_ring_buffer.h"
#include "../include/protocol.h" // Include protocol definitions
#include <stdint.h>
#include <stdbool.h>

#define MAX_EPOLL_EVENTS 64

// A union to hold any type of message from our protocol.
// This will be the item type for our ring buffer.
typedef union {
    uint8_t msg_type;
    NewOrderMessage new_order;
    CancelOrderMessage cancel_order;
    ShutdownMessage shutdown;
} ProtocolMessage;

// NEW: A struct to hold the state for a single client connection.
// Each connected client will have one of these.
typedef struct {
    int fd;
    char buffer[2048];
    size_t buffer_used;
} ConnectionState;

// Holds the state required for the networking thread.
typedef struct {
    int epoll_fd;
    int listen_fd;
    SPSC_RingBuffer* ring_buffer; // Pointer to the shared ring buffer
    atomic_bool* running;         // Pointer to the global running flag
} NetworkThreadState;

/**
 * @brief The main function for the network thread.
 * This function will be started in a new thread. It initializes networking,
 * accepts connections, and pushes incoming messages into the ring buffer.
 * @param arg A pointer to the NetworkThreadState struct.
 * @return NULL.
 */
void* network_thread_main(void* arg);

#endif // NETWORK_H
