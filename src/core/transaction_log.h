#ifndef TRANSACTION_LOG_H
#define TRANSACTION_LOG_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include "../concurrent/spsc_ring_buffer.h"

// Enum for the different types of log entries we can have.
typedef enum {
    LOG_ENTRY_LISTING,
    LOG_ENTRY_TRADE
} LogEntryType;

// Represents a log entry for a new order listing.
typedef struct {
    uint64_t order_id;
    uint8_t side;
    double listing_fee; // Store fees as double for precision
} ListingLog;

// Represents a log entry for a trade execution.
typedef struct {
    uint64_t aggressive_order_id; // The ID of the order that initiated the trade
    uint64_t resting_order_id;    // The ID of the order that was on the book
    uint64_t price;
    uint32_t quantity;
    double transaction_fee;
} TradeLog;

// FIX: Refactored to a proper "tagged union".
// The 'type' field now exists outside the union, correctly identifying
// which member of the 'data' union is active. This resolves the compiler warnings.
typedef struct {
    LogEntryType type;
    union {
        ListingLog listing;
        TradeLog trade;
    } data;
} LogEntry;

// State for the dedicated logging thread.
typedef struct {
    FILE* log_file;
    SPSC_RingBuffer* ring_buffer;
    atomic_bool* running;
} LoggerThreadState;

/**
 * @brief The main function for the logging thread.
 * It pulls log entries from a ring buffer and writes them to a file.
 * @param arg A pointer to the LoggerThreadState struct.
 * @return NULL.
 */
void* logger_thread_main(void* arg);

#endif // TRANSACTION_LOG_H
