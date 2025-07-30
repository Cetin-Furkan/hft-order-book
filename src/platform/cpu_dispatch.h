#ifndef CPU_DISPATCH_H
#define CPU_DISPATCH_H

#include <stdbool.h>
#include "analysis_simd.h" // Include the analysis header

// This struct will hold boolean flags indicating which instruction
// sets are supported by the host CPU.
typedef struct {
    bool sse;
    bool sse2;
    bool sse3;
    bool ssse3;
    bool sse4_1;
    bool sse4_2;
    bool avx;
    bool avx2;
    bool avx512f; // AVX-512 Foundation
} CpuFeatures;

// A global variable to hold the detected features of the CPU.
extern CpuFeatures g_cpu_features;

/**
 * @brief Initializes the global CPU features structure and sets up function pointers.
 * This function should be called once at program startup.
 */
void cpu_features_init(void);

#endif // CPU_DISPATCH_H
