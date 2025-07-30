#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// This pragma directive ensures that the compiler does not add any padding
// between the members of the struct. This is crucial for network protocols
// to ensure the data layout is identical on both the client and server.
#pragma pack(push, 1)

// Defines the structure for a new order message.
typedef struct {
    uint8_t  msg_type;   // Message Type: 'N' for New Order
    uint8_t  side;       // Side: 'B' for Buy, 'S' for Sell
    uint64_t order_id;   // A unique identifier for the order
    uint64_t price;      // Price, stored as an integer (e.g., 100.50 is stored as 10050)
    uint32_t quantity;   // The quantity for the order
} NewOrderMessage;

// Defines the structure for a cancel order message.
typedef struct {
    uint8_t  msg_type;   // Message Type: 'C' for Cancel
    uint64_t order_id;   // The ID of the order to be cancelled
} CancelOrderMessage;

// Defines the structure for a shutdown message.
typedef struct {
    uint8_t msg_type;    // Message Type: 'X' for Shutdown
} ShutdownMessage;

#pragma pack(pop)

#endif // PROTOCOL_H
