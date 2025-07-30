#ifndef ANALYSIS_SIMD_H
#define ANALYSIS_SIMD_H

#include "../core/order_book.h"
#include <stdint.h>

/**
 * @brief Calculates the total volume (sum of quantity) for the top N price levels.
 * This is a function pointer that will be set at runtime by the CPU dispatcher
 * to point to the fastest available implementation (AVX2, SSE, or scalar).
 * @param levels An array of PriceLevel structs.
 * @param level_count The number of levels to sum up.
 * @return The total volume as a 64-bit integer.
 */
extern uint64_t (*calculate_total_volume)(const PriceLevel* levels, uint32_t level_count);

#endif // ANALYSIS_SIMD_H
