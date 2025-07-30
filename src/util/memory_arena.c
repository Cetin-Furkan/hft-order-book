#include "memory_arena.h"
#include <stdlib.h> // For malloc, free
#include <stdio.h>  // For perror

int arena_init(Arena* arena, size_t size) {
    if (!arena || size == 0) {
        return -1;
    }
    // In a real production system, mmap would be preferred over malloc
    // as it provides more control over memory pages.
    arena->base = (uint8_t*)malloc(size);
    if (!arena->base) {
        perror("arena_init: failed to allocate memory");
        return -1;
    }
    arena->size = size;
    arena->used = 0;
    return 0;
}

void* arena_alloc(Arena* arena, size_t request_size, size_t alignment) {
    // Calculate the memory address of the next allocation, including alignment.
    // The alignment calculation ensures that the returned pointer has an address
    // that is a multiple of the 'alignment' value. This is critical for
    // performance and for correctness of certain instructions (e.g., SIMD).
    size_t current_ptr_val = (size_t)arena->base + arena->used;
    size_t padding = 0;
    
    // The modulo operation determines how far off the current pointer is
    // from the desired alignment boundary.
    if (current_ptr_val % alignment != 0) {
        padding = alignment - (current_ptr_val % alignment);
    }

    // Check if there is enough space in the arena for the requested size
    // plus any necessary padding.
    if (arena->used + padding + request_size > arena->size) {
        // Not enough memory left in the arena.
        return NULL;
    }

    // Apply the padding to the used counter.
    arena->used += padding;
    
    // Get the correctly aligned pointer to return to the user.
    void* ptr = arena->base + arena->used;
    
    // "Bump" the pointer by the requested size for the next allocation.
    arena->used += request_size;

    return ptr;
}

void arena_reset(Arena* arena) {
    if (arena) {
        // Resetting the arena is as simple as moving the 'used' counter
        // back to the beginning. No actual memory deallocation happens,
        // which makes this extremely fast.
        arena->used = 0;
    }
}

void arena_destroy(Arena* arena) {
    if (arena && arena->base) {
        free(arena->base);
        arena->base = NULL;
        arena->size = 0;
        arena->used = 0;
    }
}
