#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "spsc_queue.h"
#include <stdint.h>
#include <stdbool.h> // For bool

// --- Configuration ---
// The maximum number of out-of-order messages we can buffer.
#define GAP_BUFFER_SIZE 1024

// The main struct for our sequencing engine.
typedef struct {
    // The next sequence number we expect to process.
    uint64_t next_sequence_number;

    // Pointers to the two queues it connects.
    SPSCQueue* input_queue;  // From the Network thread
    SPSCQueue* output_queue; // To the Processing thread

    // A simple buffer to hold messages that arrive out of order.
    QueueItem gap_buffer[GAP_BUFFER_SIZE];
    bool is_slot_occupied[GAP_BUFFER_SIZE];

} Sequencer;

// --- Public API Functions ---

/**
 * @brief Initializes the sequencer state.
 * @param seq A pointer to the Sequencer struct.
 * @param input_q A pointer to the input queue.
 * @param output_q A pointer to the output queue.
 */
void sequencer_init(Sequencer* seq, SPSCQueue* input_q, SPSCQueue* output_q);

/**
 * @brief Performs one cycle of the sequencer's logic.
 * It tries to process one message from the input queue and any
 * applicable messages from the gap buffer.
 * @param seq A pointer to the Sequencer struct.
 * @return True if any message was processed, false otherwise.
 */
bool sequencer_run_once(Sequencer* seq);

#endif // SEQUENCER_H
