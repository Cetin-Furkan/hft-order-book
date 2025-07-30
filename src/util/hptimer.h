#ifndef HPTIMER_H
#define HPTIMER_H

#include <stdint.h>

/**
 * @brief Reads the Time-Stamp Counter (TSC) of the CPU.
 * This provides the highest-resolution timer available on x86 platforms,
 * counting individual CPU clock cycles.
 * Uses the `rdtscp` instruction which is serializing, ensuring that all
 * previous instructions have completed before the timestamp is read. This
 * prevents out-of-order execution from affecting measurement accuracy.
 * @return A 64-bit integer representing the number of clock cycles since boot.
 */
static inline uint64_t rdtsc() {
    uint32_t lo, hi;
    // This is an inline assembly command to read the TSC.
    // "=a"(lo), "=d"(hi) tells the compiler that the output will be in the EAX and EDX registers.
    // "%rcx" is in the clobber list because rdtscp modifies the RCX register.
    __asm__ __volatile__ (
        "rdtscp" : "=a"(lo), "=d"(hi) :: "%rcx"
    );
    return ((uint64_t)hi << 32) | lo;
}

#endif // HPTIMER_H
