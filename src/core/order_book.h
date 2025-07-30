#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <stdint.h>
#include <stdbool.h>
#include "../util/memory_arena.h"
#include "transaction_log.h" // Include the new logger definitions

// --- Constants ---
#define MAX_PRICE_LEVELS 1024
#define MAX_ORDERS_PER_LEVEL 2048
#define MAX_TOTAL_ORDERS 1000000

// --- Fee Constants ---
#define LISTING_FEE 0.01        // $0.01 per new order placed
#define TRANSACTION_FEE_BPS 100 // 1 basis point = 0.01%

// --- Core Data Structures ---

typedef struct PriceLevel PriceLevel;

typedef struct {
    uint64_t id;
    uint64_t price;
    uint32_t quantity;
    uint8_t  side;
    uint8_t  _padding[3];
    PriceLevel* level;
} Order;

struct PriceLevel {
    uint64_t price;
    uint32_t total_quantity;
    uint32_t order_count;
    uint32_t capacity;
    Order** orders;
};

typedef struct {
    PriceLevel bids[MAX_PRICE_LEVELS];
    PriceLevel asks[MAX_PRICE_LEVELS];
    uint32_t bid_level_count;
    uint32_t ask_level_count;
    Order* orders_by_id[MAX_TOTAL_ORDERS];
    Arena* arena;
    SPSC_RingBuffer* logger_queue; // Pointer to the logger's ring buffer
} OrderBook;


// --- Function Prototypes ---

int order_book_init(OrderBook* book, Arena* arena, SPSC_RingBuffer* logger_queue);
int order_book_add_order(OrderBook* book, uint64_t id, uint8_t side, uint64_t price, uint32_t quantity);
int order_book_cancel_order(OrderBook* book, uint64_t order_id);
void order_book_print(const OrderBook* book);

#endif // ORDER_BOOK_H
