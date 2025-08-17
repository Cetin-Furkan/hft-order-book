#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include <stdatomic.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h> // For size_t

// --- Configuration ---
#define QUEUE_CAPACITY 8192 // Must be a power of two
#define BUF_SIZE 2048       // Must match the buffer size in main.c

// The item that will be stored in the queue.
// It contains the raw data and the number of bytes received.
typedef struct {
    char data[BUF_SIZE];
    size_t size;
} QueueItem;

// The lock-free, cache-padded SPSC queue structure.
typedef struct {
    // Consumer-only section
    alignas(64) atomic_size_t head;

    // Producer-only section
    alignas(64) atomic_size_t tail;

    // The buffer that holds the queue items
    alignas(64) QueueItem buffer[QUEUE_CAPACITY];
} SPSCQueue;


// --- Public API Functions ---

/**
 * @brief Initializes the SPSC queue.
 * @param q A pointer to the SPSCQueue struct.
 */
void spsc_queue_init(SPSCQueue* q);

/**
 * @brief Enqueues an item (Producer side).
 * @param q A pointer to the SPSCQueue.
 * @param item The item to enqueue.
 * @return True if the enqueue was successful, false if the queue was full.
 */
bool spsc_queue_enqueue(SPSCQueue* q, const QueueItem* item);

/**
 * @brief Dequeues an item (Consumer side).
 * @param q A pointer to the SPSCQueue.
 * @param item A pointer to a QueueItem to be filled with the dequeued data.
 * @return True if an item was dequeued, false if the queue was empty.
 */
bool spsc_queue_dequeue(SPSCQueue* q, QueueItem* item);

#endif // SPSC_QUEUE_H
