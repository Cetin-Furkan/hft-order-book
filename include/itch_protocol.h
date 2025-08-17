#ifndef ITCH_PROTOCOL_H
#define ITCH_PROTOCOL_H

#include <stdint.h> // For fixed-width integer types like uint8_t, uint64_t

// The __attribute__((packed)) directive is critical. It tells the compiler
// NOT to insert any padding bytes for alignment. This ensures that the
// memory layout of our struct is a perfect 1-to-1 match with the
// binary message format specified in the NASDAQ ITCH 5.0 documentation.

// --- SIMULATED HEADER ---
// Real ITCH messages are wrapped in a transport layer protocol.
// We will simulate this by adding a small header to each of our messages
// that contains a sequence number.
typedef struct __attribute__((packed)) {
    uint64_t sequence_number;
    uint8_t  message_type;
} MessageHeader;


// Message Type: 'A'
typedef struct __attribute__((packed)) {
    uint64_t sequence_number;
    uint8_t  message_type;       // 'A' for Add Order
    uint16_t stock_locate;       // Locator for the stock symbol
    uint16_t tracking_number;    // Internal tracking number
    uint64_t timestamp;          // Nanoseconds since midnight
    uint64_t order_ref_num;      // Unique order reference number
    char     buy_sell_indicator; // 'B' for Buy, 'S' for Sell
    uint32_t shares;             // Number of shares for the order
    char     stock[8];           // Stock symbol, right-padded with spaces
    uint32_t price;              // Price of the order (in 1/10000ths of a dollar)
} AddOrderMessage;

// Message Type: 'E'
typedef struct __attribute__((packed)) {
    uint64_t sequence_number;
    uint8_t  message_type;       // 'E' for Order Executed
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_ref_num;
    uint32_t executed_shares;
    uint64_t match_number;
} OrderExecutedMessage;

// Message Type: 'X'
typedef struct __attribute__((packed)) {
    uint64_t sequence_number;
    uint8_t  message_type;       // 'X' for Order Cancel
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_ref_num;
    uint32_t canceled_shares;
} OrderCancelMessage;


#endif // ITCH_PROTOCOL_H
