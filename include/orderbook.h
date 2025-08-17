#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <stdint.h>
#include <stdbool.h>

// --- Configuration Constants ---
#define MAX_ORDERS_PER_LEVEL 512
#define MAX_PRICE_LEVELS 1024
#define MAX_ORDERS 1000000 // Max total orders in the system

// --- Core Data Structures ---

// Represents a single order in the book.
typedef struct {
    uint64_t id;
    uint32_t shares;
    // We need a back-pointer to the parent PriceLevel to make
    // cancellations efficient.
    struct PriceLevel* level;
} Order;

// Represents a single price level in the order book.
typedef struct PriceLevel {
    uint32_t price;
    uint32_t total_shares;
    uint32_t order_count;
    Order orders[MAX_ORDERS_PER_LEVEL];
} PriceLevel;

// The main order book structure.
typedef struct {
    // Direct lookup table for O(1) access to any order by its ID.
    Order* orders_by_id[MAX_ORDERS];

    // Arrays to hold the price levels.
    PriceLevel bids[MAX_PRICE_LEVELS];
    PriceLevel asks[MAX_PRICE_LEVELS];

    // Counters for the number of active levels.
    uint32_t bid_levels;
    uint32_t ask_levels;
} OrderBook;


// --- Public API Functions ---

void orderbook_init(OrderBook* book);

/**
 * @brief Processes a new incoming order, matching it against the book if possible.
 * Any remaining shares are added to the book as a new resting order.
 * @param book A pointer to the OrderBook.
 * @param id The unique reference number of the order.
 * @param price The price of the order.
 * @param shares The number of shares.
 * @param is_buy True if it's a buy order, false for a sell order.
 */
void orderbook_process_order(OrderBook* book, uint64_t id, uint32_t price, uint32_t shares, bool is_buy);

void orderbook_execute(OrderBook* book, uint64_t id, uint32_t shares);

void orderbook_cancel(OrderBook* book, uint64_t id);

#endif // ORDERBOOK_H
