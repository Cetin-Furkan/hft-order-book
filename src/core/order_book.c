#include "order_book.h"
#include <stdio.h>
#include <string.h>
#include <stdalign.h>

// --- Static Helper Function Prototypes ---
static void remove_order(PriceLevel* level, uint32_t order_index);
static void remove_price_level(OrderBook* book, uint8_t side, uint32_t level_index);

// --- Public Function Implementations ---

int order_book_init(OrderBook* book, Arena* arena, SPSC_RingBuffer* logger_queue) {
    if (book == NULL || arena == NULL || logger_queue == NULL) {
        return -1;
    }
    memset(book, 0, sizeof(OrderBook));
    book->arena = arena;
    book->logger_queue = logger_queue;
    return 0;
}

static PriceLevel* find_or_create_level(OrderBook* book, uint64_t price, uint8_t side) {
    PriceLevel* levels = (side == 'B') ? book->bids : book->asks;
    uint32_t* level_count = (side == 'B') ? &book->bid_level_count : &book->ask_level_count;

    for (uint32_t i = 0; i < *level_count; ++i) {
        if (levels[i].price == price) {
            return &levels[i];
        }
    }

    if (*level_count >= MAX_PRICE_LEVELS) {
        fprintf(stderr, "Error: Max price levels reached for side %c\n", side);
        return NULL;
    }

    PriceLevel* new_level = &levels[(*level_count)++];
    new_level->price = price;
    new_level->total_quantity = 0;
    new_level->order_count = 0;
    new_level->capacity = MAX_ORDERS_PER_LEVEL;
    
    new_level->orders = arena_alloc(book->arena, new_level->capacity * sizeof(Order*), alignof(Order*));
    if (new_level->orders == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for new price level's order list.\n");
        (*level_count)--;
        return NULL;
    }
    return new_level;
}

int order_book_add_order(OrderBook* book, uint64_t id, uint8_t side, uint64_t price, uint32_t quantity) {
    uint32_t quantity_to_trade = quantity;

    if (side == 'B') {
        while (quantity_to_trade > 0 && book->ask_level_count > 0 && price >= book->asks[0].price) {
            PriceLevel* best_ask_level = &book->asks[0];
            for (uint32_t i = 0; i < best_ask_level->order_count && quantity_to_trade > 0; ) {
                Order* resting_order = best_ask_level->orders[i];
                uint32_t trade_qty = (quantity_to_trade < resting_order->quantity) ? quantity_to_trade : resting_order->quantity;

                // --- Fee Calculation & Logging ---
                double trade_value = (double)(trade_qty * resting_order->price) / 100.0; // Assuming price is in cents
                double fee = trade_value * (TRANSACTION_FEE_BPS / 10000.0);
                // FIX: Use correct tagged union initialization.
                LogEntry trade_log = {
                    .type = LOG_ENTRY_TRADE,
                    .data.trade = {
                        .aggressive_order_id = id,
                        .resting_order_id = resting_order->id,
                        .price = resting_order->price,
                        .quantity = trade_qty,
                        .transaction_fee = fee
                    }
                };
                spsc_ring_buffer_push(book->logger_queue, &trade_log);
                // ---------------------------------

                quantity_to_trade -= trade_qty;
                resting_order->quantity -= trade_qty;
                best_ask_level->total_quantity -= trade_qty;

                if (resting_order->quantity == 0) {
                    book->orders_by_id[resting_order->id] = NULL;
                    remove_order(best_ask_level, i);
                } else {
                    i++;
                }
            }
            if (best_ask_level->order_count == 0) {
                remove_price_level(book, 'S', 0);
            }
        }
    } else { // side == 'S'
        while (quantity_to_trade > 0 && book->bid_level_count > 0 && price <= book->bids[0].price) {
            PriceLevel* best_bid_level = &book->bids[0];
            for (uint32_t i = 0; i < best_bid_level->order_count && quantity_to_trade > 0; ) {
                Order* resting_order = best_bid_level->orders[i];
                uint32_t trade_qty = (quantity_to_trade < resting_order->quantity) ? quantity_to_trade : resting_order->quantity;
                
                // --- Fee Calculation & Logging ---
                double trade_value = (double)(trade_qty * resting_order->price) / 100.0;
                double fee = trade_value * (TRANSACTION_FEE_BPS / 10000.0);
                // FIX: Use correct tagged union initialization.
                LogEntry trade_log = {
                    .type = LOG_ENTRY_TRADE,
                    .data.trade = {
                        .aggressive_order_id = id,
                        .resting_order_id = resting_order->id,
                        .price = resting_order->price,
                        .quantity = trade_qty,
                        .transaction_fee = fee
                    }
                };
                spsc_ring_buffer_push(book->logger_queue, &trade_log);
                // ---------------------------------
                
                quantity_to_trade -= trade_qty;
                resting_order->quantity -= trade_qty;
                best_bid_level->total_quantity -= trade_qty;

                if (resting_order->quantity == 0) {
                    book->orders_by_id[resting_order->id] = NULL;
                    remove_order(best_bid_level, i);
                } else {
                    i++;
                }
            }
            if (best_bid_level->order_count == 0) {
                remove_price_level(book, 'B', 0);
            }
        }
    }

    if (quantity_to_trade > 0) {
        if (id >= MAX_TOTAL_ORDERS) {
            fprintf(stderr, "Error: Order ID %lu exceeds MAX_TOTAL_ORDERS\n", id);
            return -1;
        }
        PriceLevel* level = find_or_create_level(book, price, side);
        if (level == NULL) return -1;

        if (level->order_count >= level->capacity) {
            fprintf(stderr, "Error: Max orders per level reached for price %lu\n", price);
            return -1;
        }

        Order* new_order = arena_alloc(book->arena, sizeof(Order), alignof(Order));
        if (new_order == NULL) {
            fprintf(stderr, "Error: Memory arena is full. Cannot allocate new order.\n");
            return -1;
        }

        new_order->id = id;
        new_order->side = side;
        new_order->price = price;
        new_order->quantity = quantity_to_trade;
        new_order->level = level;

        level->orders[level->order_count++] = new_order;
        level->total_quantity += quantity_to_trade;
        book->orders_by_id[id] = new_order;

        // --- Listing Fee Logging ---
        // FIX: Use correct tagged union initialization.
        LogEntry listing_log = {
            .type = LOG_ENTRY_LISTING,
            .data.listing = { .order_id = id, .side = side, .listing_fee = LISTING_FEE }
        };
        spsc_ring_buffer_push(book->logger_queue, &listing_log);
        // ---------------------------
    }

    return 0;
}

int order_book_cancel_order(OrderBook* book, uint64_t order_id) {
    if (order_id >= MAX_TOTAL_ORDERS) {
        fprintf(stderr, "Cancel failed: Invalid order ID %lu\n", order_id);
        return -1;
    }

    Order* order_to_cancel = book->orders_by_id[order_id];

    if (order_to_cancel == NULL) {
        fprintf(stderr, "Cancel failed: Order ID %lu not found or already filled.\n", order_id);
        return -1;
    }

    PriceLevel* level = order_to_cancel->level;
    
    for (uint32_t i = 0; i < level->order_count; ++i) {
        if (level->orders[i]->id == order_id) {
            level->total_quantity -= order_to_cancel->quantity;
            remove_order(level, i);
            break;
        }
    }

    book->orders_by_id[order_id] = NULL;

    if (level->order_count == 0) {
        PriceLevel* levels = (order_to_cancel->side == 'B') ? book->bids : book->asks;
        uint32_t* level_count = (order_to_cancel->side == 'B') ? &book->bid_level_count : &book->ask_level_count;
        for (uint32_t i = 0; i < *level_count; ++i) {
            if (&levels[i] == level) {
                remove_price_level(book, order_to_cancel->side, i);
                break;
            }
        }
    }

    return 0;
}

void order_book_print(const OrderBook* book) {
    if (!book) return;
    printf("\n--- ORDER BOOK ---\n");
    printf("--- ASKS ---\n%-10s | %-12s\n", "Price", "Quantity");
    printf("------------------------\n");
    for (int i = book->ask_level_count - 1; i >= 0; i--) {
        printf("%-10lu | %-12u\n", book->asks[i].price, book->asks[i].total_quantity);
    }
    printf("------------------------\n");
    printf("--- BIDS ---\n%-10s | %-12s\n", "Price", "Quantity");
    printf("------------------------\n");
    for (uint32_t i = 0; i < book->bid_level_count; i++) {
        printf("%-10lu | %-12u\n", book->bids[i].price, book->bids[i].total_quantity);
    }
    printf("-------------------\n\n");
}

static void remove_order(PriceLevel* level, uint32_t order_index) {
    if (order_index >= level->order_count) return;
    uint32_t remaining_elements = level->order_count - order_index - 1;
    if (remaining_elements > 0) {
        memmove(&level->orders[order_index], 
                &level->orders[order_index + 1], 
                remaining_elements * sizeof(Order*));
    }
    level->order_count--;
}

static void remove_price_level(OrderBook* book, uint8_t side, uint32_t level_index) {
    if (side == 'B') {
        if (level_index >= book->bid_level_count) return;
        uint32_t remaining_levels = book->bid_level_count - level_index - 1;
        if (remaining_levels > 0) {
            memmove(&book->bids[level_index], 
                    &book->bids[level_index + 1], 
                    remaining_levels * sizeof(PriceLevel));
        }
        book->bid_level_count--;
    } else {
        if (level_index >= book->ask_level_count) return;
        uint32_t remaining_levels = book->ask_level_count - level_index - 1;
        if (remaining_levels > 0) {
            memmove(&book->asks[level_index], 
                    &book->asks[level_index + 1], 
                    remaining_levels * sizeof(PriceLevel));
        }
        book->ask_level_count--;
    }
}
