#include "spsc_queue.h"
#include <string.h> // For memcpy

void spsc_queue_init(SPSCQueue* q) {
    atomic_store(&q->head, 0);
    atomic_store(&q->tail, 0);
}

bool spsc_queue_enqueue(SPSCQueue* q, const QueueItem* item) {
    const size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    const size_t next_tail = (tail + 1) & (QUEUE_CAPACITY - 1); // Bitwise AND for fast modulo

    // Check if the queue is full. We load head with relaxed order because we are the
    // only writer to tail, so we only need to see if we'd lap the reader.
    if (next_tail == atomic_load_explicit(&q->head, memory_order_relaxed)) {
        return false; // Queue is full
    }

    // Copy the data into the buffer slot.
    // This is a normal memory write.
    memcpy(&q->buffer[tail], item, sizeof(QueueItem));

    // "Publish" the write by updating the tail pointer using a RELEASE memory fence.
    // This ensures the memcpy above is visible to the consumer before the tail update.
    atomic_store_explicit(&q->tail, next_tail, memory_order_release);

    return true;
}

bool spsc_queue_dequeue(SPSCQueue* q, QueueItem* item) {
    const size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);

    // Check if the queue is empty. We must use an ACQUIRE fence on the tail load
    // to synchronize with the producer's RELEASE store.
    if (head == atomic_load_explicit(&q->tail, memory_order_acquire)) {
        return false; // Queue is empty
    }

    // Copy the data out of the buffer slot.
    // The acquire fence above guarantees we will see the data the producer wrote.
    memcpy(item, &q->buffer[head], sizeof(QueueItem));

    // Update the head pointer. We use a RELEASE fence here to ensure that any
    // subsequent operations by the consumer are not reordered before this store.
    atomic_store_explicit(&q->head, (head + 1) & (QUEUE_CAPACITY - 1), memory_order_release);

    return true;
}
