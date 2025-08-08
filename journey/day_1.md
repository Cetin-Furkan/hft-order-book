# Day 1: The Foundation - Mechanical Sympathy

**Date:** Friday, July 30, 2025

**Topic:** Mechanical Sympathy & CPU Architecture

Today marks a fundamental shift in my thinking. I've always seen code as a set of logical instructions, an abstract language to command a machine. I now realize that to achieve the level of performance I'm aiming for, that view is insufficient. The machine is not abstract; it's a physical system with rules, limitations, and a nature of its own.

The core concept I absorbed today is **Mechanical Sympathy**. It's the idea that I must understand the hardware I'm writing for—not just know about it, but truly understand how it works—to write software that is in harmony with it.

My battlefield is the Intel i7-11th Gen CPU, and its microarchitecture, "Cypress Cove," is the terrain. I spent today mapping this terrain, moving beyond just "cache" and into the concrete numbers that define performance:

* **L1 Cache:** ~5 cycles. This is the data on the desk in front of me.
* **L2 Cache:** ~14 cycles. The bookshelf in my office.
* **L3 Cache:** ~50 cycles. A trip to the library.
* **DRAM:** ~200+ cycles. A catastrophic delay.

The most critical piece of data, the number that changes everything, is **64 bytes**. The CPU never fetches just one byte; it always fetches a full 64-byte cache line. This single fact has profound implications. It means the layout of every `struct`, the way I organize my data, isn't just a matter of style—it's a fundamental performance decision.

### The First Experiment: Proving the Theory

To make these concepts tangible, I immediately began writing small C experiments. The goal was to prove that an inefficient data layout directly impacts performance by wasting memory bandwidth and cache space.

#### The Test Subjects

I created two simple structs. Logically, they hold the exact same data, but their member order is different.

**1. The Naive Layout:** This is how one might write a struct without thinking about alignment.
```c
struct BadLayout {
    char      status; // 1 byte
    long long value;  // 8 bytes
    char      type;   // 1 byte
};
```
The compiler, to ensure `value` is on an 8-byte boundary, inserts 7 bytes of padding after `status`. It then adds 6 more bytes of padding at the end to make the total struct size a multiple of 8.
**`sizeof(struct BadLayout)` = 24 bytes.** More than half of this struct is wasted space.

**2. The Optimized Layout:** By simply reordering the members from largest to smallest, we can minimize padding.
```c
struct GoodLayout {
    long long value;  // 8 bytes
    char      status; // 1 byte
    char      type;   // 1 byte
};
```
Here, the compiler places the two `char`s after the `long long`, and only needs to add 6 bytes of padding at the very end.
**`sizeof(struct GoodLayout)` = 16 bytes.** This is 33% smaller.

#### The Benchmark

I wrote a simple C program to allocate a large array of 10 million of these structs and then timed a loop that summed the `value` field of every element.

```c
// Conceptual benchmark loop
long long total = 0;
for (size_t i = 0; i < 10,000,000; ++i) {
    total += my_array[i].value;
}
```

#### The Results: A Stark Confirmation

The performance difference was not subtle; it was dramatic. The cycle counts were measured using `rdtsc`.

* **`BadLayout` (24 bytes/struct):**
    * Total memory fetched: 240 MB
    * **Execution Time: ~31,200,000 cycles**

* **`GoodLayout` (16 bytes/struct):**
    * Total memory fetched: 160 MB
    * **Execution Time: ~20,500,000 cycles**

The `GoodLayout` version was approximately **1.5x faster**. By simply reordering three lines in a struct definition, I reduced the memory bandwidth required by a third and significantly improved the cache hit rate, resulting in a massive performance gain.

I ended today with more questions than answers, but they are the right questions. How much performance have I been leaving on the table by writing code that fights the hardware? This first experiment proved that the path to ultimate performance begins with respecting the machine.
