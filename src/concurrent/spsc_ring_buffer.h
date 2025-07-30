#ifndef SPSC_RING_BUFFER_H
#define SPSC_RING_BUFFER_H

#include <stdatomic.h>
#include <stddef.h>

// A generic ring buffer that can store items of any type.
// The actual data is stored in a flexible array member at the end.
typedef struct {
    size_t capacity;
    size_t item_size;
    // 'head' is only ever modified by the consumer thread.
    // 'tail' is only ever modified by the producer thread.
    // We use 'atomic_size_t' to ensure visibility between threads without locks.
    atomic_size_t head;
    atomic_size_t tail;
    // Flexible array member to hold the buffer data.
    char buffer[];
} SPSC_RingBuffer;

/**
 * @brief Creates and initializes a new ring buffer.
 * @param capacity The number of items the buffer can hold. Must be a power of 2.
 * @param item_size The size of each item in bytes.
 * @return A pointer to the newly created ring buffer, or NULL on failure.
 */
SPSC_RingBuffer* spsc_ring_buffer_init(size_t capacity, size_t item_size);

/**
 * @brief Destroys a ring buffer and frees its memory.
 * @param rb The ring buffer to destroy.
 */
void spsc_ring_buffer_destroy(SPSC_RingBuffer* rb);

/**
 * @brief Pushes an item into the ring buffer (Producer only).
 * This is a non-blocking, lock-free operation.
 * @param rb The ring buffer.
 * @param item A pointer to the item to be pushed.
 * @return true if the item was pushed successfully, false if the buffer is full.
 */
bool spsc_ring_buffer_push(SPSC_RingBuffer* rb, const void* item);

/**
 * @brief Pops an item from the ring buffer (Consumer only).
 * This is a non-blocking, lock-free operation.
 * @param rb The ring buffer.
 * @param item A pointer to a variable where the popped item will be stored.
 * @return true if an item was popped successfully, false if the buffer is empty.
 */
bool spsc_ring_buffer_pop(SPSC_RingBuffer* rb, void* item);

#endif // SPSC_RING_BUFFER_H
