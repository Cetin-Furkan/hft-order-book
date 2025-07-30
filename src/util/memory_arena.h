#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

#include <stddef.h> // For size_t
#include <stdint.h> // For uint8_t

// The Arena struct holds the state of our custom memory allocator.
// It contains a pointer to a large block of memory and tracks its usage.
typedef struct {
    uint8_t* base;    // Pointer to the start of the memory block.
    size_t   size;    // The total size of the memory block in bytes.
    size_t   used;    // The number of bytes currently used in the block.
} Arena;

/**
 * @brief Initializes a memory arena.
 * Allocates a large, contiguous block of memory for the arena to manage.
 * @param arena Pointer to the Arena struct to initialize.
 * @param size The total number of bytes to allocate for the arena.
 * @return 0 on success, -1 on failure.
 */
int arena_init(Arena* arena, size_t size);

/**
 * @brief Allocates a block of memory from the arena.
 * This is an O(1) operation using a pointer-bumping strategy.
 * It is alignment-aware to ensure performance and correctness.
 * @param arena Pointer to the Arena.
 * @param request_size The number of bytes to allocate.
 * @param alignment The desired memory alignment for the allocation.
 * @return A pointer to the allocated memory, or NULL if the arena is full.
 */
void* arena_alloc(Arena* arena, size_t request_size, size_t alignment);

/**
 * @brief Resets the arena, effectively "freeing" all memory within it.
 * This is an O(1) operation that simply resets the 'used' counter.
 * @param arena Pointer to the Arena to reset.
 */
void arena_reset(Arena* arena);

/**
 * @brief Frees the underlying memory block of the arena.
 * Should be called once when the program is shutting down.
 * @param arena Pointer to the Arena to destroy.
 */
void arena_destroy(Arena* arena);

#endif // MEMORY_ARENA_H
