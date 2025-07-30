#include "cpu_dispatch.h"
#include <cpuid.h> // Header for the __get_cpuid wrapper
#include <stdio.h>

// --- Global Variable Definitions ---
CpuFeatures g_cpu_features = {0};
uint64_t (*calculate_total_volume)(const PriceLevel* levels, uint32_t level_count) = NULL;

// --- Extern Function Declarations from analysis_simd.c ---
// We need to tell the compiler about the different versions of our function.
extern uint64_t calculate_total_volume_scalar(const PriceLevel* levels, uint32_t level_count);
extern uint64_t calculate_total_volume_avx2(const PriceLevel* levels, uint32_t level_count);


// A helper macro to check a bit in a register.
#define CHECK_BIT(reg, bit) ((reg) & (1 << (bit)))

// This is the resolver function that selects the best implementation.
static void resolve_implementations(void) {
    // Select the fastest available version of the function based on CPU support.
    // We check for the best one first (AVX2), then fall back.
    if (g_cpu_features.avx2) {
        calculate_total_volume = &calculate_total_volume_avx2;
        printf("Dispatch: Using AVX2 implementation.\n");
    } else {
        calculate_total_volume = &calculate_total_volume_scalar;
        printf("Dispatch: Using Scalar C implementation.\n");
    }
}

void cpu_features_init(void) {
    unsigned int eax, ebx, ecx, edx;

    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        g_cpu_features.sse    = CHECK_BIT(edx, 25);
        g_cpu_features.sse2   = CHECK_BIT(edx, 26);
        g_cpu_features.sse3   = CHECK_BIT(ecx, 0);
        g_cpu_features.ssse3  = CHECK_BIT(ecx, 9);
        g_cpu_features.sse4_1 = CHECK_BIT(ecx, 19);
        g_cpu_features.sse4_2 = CHECK_BIT(ecx, 20);
        g_cpu_features.avx    = CHECK_BIT(ecx, 28);
    }

    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        g_cpu_features.avx2     = CHECK_BIT(ebx, 5);
        g_cpu_features.avx512f  = CHECK_BIT(ebx, 16);
    }

    // After detecting features, resolve which function implementations to use.
    resolve_implementations();
}
