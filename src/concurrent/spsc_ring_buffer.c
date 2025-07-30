#include "spsc_ring_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

SPSC_RingBuffer* spsc_ring_buffer_init(size_t capacity, size_t item_size) {
    // Ensure capacity is a power of 2 for efficient modulo operations using bitwise AND.
    assert((capacity > 0) && ((capacity & (capacity - 1)) == 0));

    size_t total_size = sizeof(SPSC_RingBuffer) + (capacity * item_size);
    SPSC_RingBuffer* rb = malloc(total_size);
    if (!rb) {
        return NULL;
    }

    rb->capacity = capacity;
    rb->item_size = item_size;
    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);

    return rb;
}

void spsc_ring_buffer_destroy(SPSC_RingBuffer* rb) {
    if (rb) {
        free(rb);
    }
}

bool spsc_ring_buffer_push(SPSC_RingBuffer* rb, const void* item) {
    const size_t current_tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    const size_t next_tail = current_tail + 1;

    // Use memory_order_acquire to ensure we see the latest value of 'head' written by the consumer.
    if (next_tail - atomic_load_explicit(&rb->head, memory_order_acquire) > rb->capacity) {
        return false; // Buffer is full
    }

    // Copy the item into the buffer.
    const size_t index = current_tail & (rb->capacity - 1); // Fast modulo
    memcpy(rb->buffer + (index * rb->item_size), item, rb->item_size);

    // Use memory_order_release to ensure the write to the buffer is visible to the consumer
    // before the tail pointer is updated.
    atomic_store_explicit(&rb->tail, next_tail, memory_order_release);

    return true;
}

bool spsc_ring_buffer_pop(SPSC_RingBuffer* rb, void* item) {
    const size_t current_head = atomic_load_explicit(&rb->head, memory_order_relaxed);

    // Use memory_order_acquire to ensure we see the latest value of 'tail' written by the producer.
    if (current_head == atomic_load_explicit(&rb->tail, memory_order_acquire)) {
        return false; // Buffer is empty
    }

    // Copy the item out of the buffer.
    const size_t index = current_head & (rb->capacity - 1); // Fast modulo
    memcpy(item, rb->buffer + (index * rb->item_size), rb->item_size);

    // Use memory_order_release to ensure the read from the buffer is complete
    // before the head pointer is updated, making the slot available for the producer.
    atomic_store_explicit(&rb->head, current_head + 1, memory_order_release);

    return true;
}
