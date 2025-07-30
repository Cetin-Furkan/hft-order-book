// FIX: Define _POSIX_C_SOURCE to expose POSIX function declarations (like nanosleep)
#define _POSIX_C_SOURCE 200809L

#include "transaction_log.h"
#include <time.h>
#include <unistd.h>

// Helper to get a timestamp string
static void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

void* logger_thread_main(void* arg) {
    LoggerThreadState* state = (LoggerThreadState*)arg;
    LogEntry entry;
    char timestamp[100];

    printf("Logger thread started.\n");

    while (atomic_load(state->running)) {
        if (spsc_ring_buffer_pop(state->ring_buffer, &entry)) {
            get_timestamp(timestamp, sizeof(timestamp));
            // FIX: Access union members via the new 'data' field.
            switch (entry.type) {
                case LOG_ENTRY_LISTING:
                    fprintf(state->log_file, "[%s] LISTING: OrderID=%lu, Side=%c, Fee=$%.2f\n",
                            timestamp, entry.data.listing.order_id, entry.data.listing.side, entry.data.listing.listing_fee);
                    break;
                case LOG_ENTRY_TRADE:
                    fprintf(state->log_file, "[%s] TRADE: AggressorID=%lu matched RestingID=%lu for %u @ %lu. Fee=$%.4f\n",
                            timestamp, entry.data.trade.aggressive_order_id, entry.data.trade.resting_order_id,
                            entry.data.trade.quantity, entry.data.trade.price, entry.data.trade.transaction_fee);
                    break;
            }
            // In a real system, you might want to flush less often for performance.
            fflush(state->log_file);
        } else {
            // A sleep for 10 milliseconds (10,000,000 nanoseconds).
            struct timespec sleep_duration = { .tv_sec = 0, .tv_nsec = 10000000 };
            nanosleep(&sleep_duration, NULL);
        }
    }

    printf("Logger thread shutting down.\n");
    return NULL;
}
