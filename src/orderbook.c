#include "orderbook.h"
#include <string.h> // For memset, memmove
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> // For qsort

// --- Helper functions for sorting price levels ---
static int compare_bids(const void* a, const void* b) {
    return ((PriceLevel*)b)->price - ((PriceLevel*)a)->price;
}
static int compare_asks(const void* a, const void* b) {
    return ((PriceLevel*)a)->price - ((PriceLevel*)b)->price;
}

void orderbook_init(OrderBook* book) {
    memset(book->bids, 0, sizeof(book->bids));
    memset(book->asks, 0, sizeof(book->asks));
    memset(book->orders_by_id, 0, sizeof(book->orders_by_id));
    book->bid_levels = 0;
    book->ask_levels = 0;
    printf("Order book initialized.\n");
    fflush(stdout);
}

static void add_order_to_level(OrderBook* book, PriceLevel* level, uint64_t id, uint32_t shares) {
    if (level->order_count < MAX_ORDERS_PER_LEVEL && id < MAX_ORDERS) {
        Order* new_order = &level->orders[level->order_count];
        new_order->id = id;
        new_order->shares = shares;
        new_order->level = level;
        level->order_count++;
        level->total_shares += shares;
        book->orders_by_id[id] = new_order;
    }
}

void orderbook_process_order(OrderBook* book, uint64_t id, uint32_t price, uint32_t shares, bool is_buy) {
    printf("Processing order %lu: %s %u shares at %u\n", id, is_buy ? "BUY" : "SELL", shares, price);
    fflush(stdout);
    uint32_t remaining_shares = shares;

    if (is_buy) {
        while (remaining_shares > 0 && book->ask_levels > 0 && book->asks[0].price <= price) {
            PriceLevel* best_ask_level = &book->asks[0];
            Order* resting_order = &best_ask_level->orders[0];

            uint32_t trade_shares = (remaining_shares < resting_order->shares) ? remaining_shares : resting_order->shares;
            
            printf(" ==> Matched %u shares with order %lu at price %u\n", trade_shares, resting_order->id, best_ask_level->price);
            fflush(stdout);

            remaining_shares -= trade_shares;
            orderbook_execute(book, resting_order->id, trade_shares);
        }
    } else {
        while (remaining_shares > 0 && book->bid_levels > 0 && book->bids[0].price >= price) {
            PriceLevel* best_bid_level = &book->bids[0];
            Order* resting_order = &best_bid_level->orders[0];

            uint32_t trade_shares = (remaining_shares < resting_order->shares) ? remaining_shares : resting_order->shares;

            printf(" ==> Matched %u shares with order %lu at price %u\n", trade_shares, resting_order->id, best_bid_level->price);
            fflush(stdout);
            
            remaining_shares -= trade_shares;
            orderbook_execute(book, resting_order->id, trade_shares);
        }
    }

    if (remaining_shares > 0) {
        PriceLevel* levels = is_buy ? book->bids : book->asks;
        uint32_t* level_count = is_buy ? &book->bid_levels : &book->ask_levels;
        
        PriceLevel* level = NULL;
        for (uint32_t i = 0; i < *level_count; ++i) {
            if (levels[i].price == price) {
                level = &levels[i];
                break;
            }
        }
        if (!level && *level_count < MAX_PRICE_LEVELS) {
            level = &levels[*level_count];
            level->price = price;
            level->order_count = 0;
            level->total_shares = 0;
            (*level_count)++;
        }

        if (level) {
            add_order_to_level(book, level, id, remaining_shares);
            if (is_buy) {
                qsort(book->bids, book->bid_levels, sizeof(PriceLevel), compare_bids);
            } else {
                qsort(book->asks, book->ask_levels, sizeof(PriceLevel), compare_asks);
            }
        }
    }
}


void orderbook_execute(OrderBook* book, uint64_t id, uint32_t shares) {
    if (id >= MAX_ORDERS || book->orders_by_id[id] == NULL) return;

    Order* order = book->orders_by_id[id];
    if (!order) return;
    
    PriceLevel* level = order->level;

    if (shares > order->shares) shares = order->shares;

    order->shares -= shares;
    level->total_shares -= shares;

    if (order->shares == 0) {
        orderbook_cancel(book, id);
    }
}

void orderbook_cancel(OrderBook* book, uint64_t id) {
    if (id >= MAX_ORDERS || book->orders_by_id[id] == NULL) return;

    Order* order_to_cancel = book->orders_by_id[id];
    if (!order_to_cancel) return;

    PriceLevel* level = order_to_cancel->level;
    bool is_buy = (level->price == book->bids[level - book->bids].price);

    level->total_shares -= order_to_cancel->shares;

    uint32_t index_to_remove = order_to_cancel - level->orders;
    Order* last_order = &level->orders[level->order_count - 1];

    if (order_to_cancel != last_order) {
        level->orders[index_to_remove] = *last_order;
        book->orders_by_id[last_order->id] = &level->orders[index_to_remove];
    }

    level->order_count--;
    book->orders_by_id[id] = NULL;
    
    // CRITICAL FIX: If the price level is now empty, remove it.
    if (level->order_count == 0) {
        PriceLevel* levels = is_buy ? book->bids : book->asks;
        uint32_t* level_count = is_buy ? &book->bid_levels : &book->ask_levels;
        
        uint32_t level_index = level - levels;
        
        // Use memmove for safe overlapping memory copy (swap and pop)
        memmove(&levels[level_index], &levels[level_index + 1], (*level_count - level_index - 1) * sizeof(PriceLevel));
        (*level_count)--;

        // Re-sort to maintain price priority after removal
        if (is_buy) {
            qsort(book->bids, book->bid_levels, sizeof(PriceLevel), compare_bids);
        } else {
            qsort(book->asks, book->ask_levels, sizeof(PriceLevel), compare_asks);
        }
    }
}
