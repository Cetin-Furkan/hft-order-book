#include "sequencer.h"
#include "itch_protocol.h"
#include "byte_order.h" // Include our new header
#include <stdio.h>
#include <string.h>

void sequencer_init(Sequencer* seq, SPSCQueue* input_q, SPSCQueue* output_q) {
    seq->next_sequence_number = 1;
    seq->input_queue = input_q;
    seq->output_queue = output_q;
    memset(seq->is_slot_occupied, 0, sizeof(seq->is_slot_occupied));
    printf("Sequencer initialized.\n");
}

static void process_and_check_gap(Sequencer* seq, QueueItem* item) {
    while (!spsc_queue_enqueue(seq->output_queue, item)) {}
    seq->next_sequence_number++;

    while (true) {
        uint32_t gap_index = seq->next_sequence_number % GAP_BUFFER_SIZE;
        if (seq->is_slot_occupied[gap_index]) {
            printf("Processing message %lu from gap buffer.\n", seq->next_sequence_number);
            fflush(stdout);

            while (!spsc_queue_enqueue(seq->output_queue, &seq->gap_buffer[gap_index])) {}

            seq->is_slot_occupied[gap_index] = false;
            seq->next_sequence_number++;
        } else {
            break;
        }
    }
}

bool sequencer_run_once(Sequencer* seq) {
    QueueItem item;
    if (spsc_queue_dequeue(seq->input_queue, &item)) {
        MessageHeader* header = (MessageHeader*)item.data;
        
        // CRITICAL FIX: Convert the sequence number from network to host byte order.
        uint64_t seq_num = ntohll(header->sequence_number);

        if (seq_num == seq->next_sequence_number) {
            process_and_check_gap(seq, &item);
        } else if (seq_num > seq->next_sequence_number) {
            printf("Received out-of-order message %lu, expected %lu. Buffering.\n",
                   seq_num, seq->next_sequence_number);
            fflush(stdout);
            uint32_t gap_index = seq_num % GAP_BUFFER_SIZE;
            if (!seq->is_slot_occupied[gap_index]) {
                memcpy(&seq->gap_buffer[gap_index], &item, sizeof(QueueItem));
                seq->is_slot_occupied[gap_index] = true;
            }
        }
        return true;
    }
    return false;
}
