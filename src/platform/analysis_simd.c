#include "analysis_simd.h"
#include <immintrin.h> // The main header for all Intel SIMD intrinsics

// --- Implementation 1: Plain C (Scalar) ---
// This is the fallback for CPUs that don't support modern instruction sets.
uint64_t calculate_total_volume_scalar(const PriceLevel* levels, uint32_t level_count) {
    uint64_t total_volume = 0;
    for (uint32_t i = 0; i < level_count; ++i) {
        total_volume += levels[i].total_quantity;
    }
    return total_volume;
}

// --- Implementation 2: AVX2 (256-bit vectors) ---
// This attribute tells the compiler that it's safe to use AVX2 instructions
// inside this function. The dispatcher ensures this is only called on AVX2-capable CPUs.
__attribute__((target("avx2")))
uint64_t calculate_total_volume_avx2(const PriceLevel* levels, uint32_t level_count) {
    // A 256-bit vector to accumulate sums. It can hold 4x 64-bit integers.
    __m256i sum_vec = _mm256_setzero_si256();
    uint32_t i = 0;

    // Process 8 levels at a time (since PriceLevel.total_quantity is 32-bit).
    // AVX2 can work with 8x 32-bit integers at once.
    for (; i + 7 < level_count; i += 8) {
        // Load 8x 32-bit quantities into a 256-bit vector.
        // This is tricky because the data is not contiguous. We load them one by one.
        __m256i quantities = _mm256_set_epi32(
            levels[i+7].total_quantity, levels[i+6].total_quantity,
            levels[i+5].total_quantity, levels[i+4].total_quantity,
            levels[i+3].total_quantity, levels[i+2].total_quantity,
            levels[i+1].total_quantity, levels[i+0].total_quantity
        );
        // We need to convert these 32-bit integers to 64-bit to avoid overflow when summing.
        // We split the 8x32-bit vector into two 4x32-bit vectors.
        __m128i low_part = _mm256_extracti128_si256(quantities, 0);
        __m128i high_part = _mm256_extracti128_si256(quantities, 1);

        // Convert each 4x32-bit vector to a 4x64-bit vector.
        __m256i low_64 = _mm256_cvtepi32_epi64(low_part);
        __m256i high_64 = _mm256_cvtepi32_epi64(high_part);
        
        // Add the 64-bit vectors to our accumulator.
        sum_vec = _mm256_add_epi64(sum_vec, low_64);
        sum_vec = _mm256_add_epi64(sum_vec, high_64);
    }

    // Horizontally sum the final accumulator vector.
    uint64_t temp_sum[4];
    _mm256_storeu_si256((__m256i*)temp_sum, sum_vec);
    uint64_t total_volume = temp_sum[0] + temp_sum[1] + temp_sum[2] + temp_sum[3];

    // Add any remaining elements that didn't fit into a block of 8.
    for (; i < level_count; ++i) {
        total_volume += levels[i].total_quantity;
    }

    return total_volume;
}
