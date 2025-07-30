# Project Journal: An Engineering Narrative

This document chronicles the engineering journey of building a high-performance limit order book from first principles. It is structured not as a daily log, but as a narrative of the architectural decisions, optimizations, and performance gains at each major stage of development. The goal is to illustrate a methodical approach to systems engineering where every decision is deliberate and justified by the core principle of achieving deterministic, low-latency performance.

---

### **Day 1: Foundational Architecture & Eliminating OS Jitter**

* **Date:** July 28, 2025
* **Objective:** To architect a system foundation that is inherently fast by design, systematically eliminating dependencies on slow and non-deterministic operating system primitives from the very beginning.

* **Technical Decisions & Rationale:**

    1.  **Language & Standard (C23):** The decision was made to use the strict `-std=c23` standard. This was not merely for novelty; it enforces a modern, safer style of C programming. Features like `nullptr` provide type safety over the old `NULL` macro, and attributes like `[[nodiscard]]` allow the compiler to enforce API contracts, preventing critical errors like unchecked return values from initialization functions.

    2.  **Modular Architecture:** A multi-directory structure (`/src/core`, `/src/platform`, etc.) was established. This enforces a strict separation of concerns, which is the bedrock of any maintainable, high-performance system. It allows the core matching logic to be developed and tested in complete isolation from the networking or concurrency models.

    3.  **Deterministic Memory Management (The Memory Arena):** The first and most critical implementation was a custom memory arena. The standard `malloc()` function is the single greatest source of non-deterministic latency in many C programs due to its reliance on system calls and kernel-space locks. By pre-allocating a large, contiguous memory block at startup and using a simple "pointer-bumping" scheme, we transformed memory allocation from a multi-microsecond, unpredictable operation into a constant-time, sub-nanosecond sequence of a few CPU instructions. This single decision is responsible for a massive reduction in potential system jitter.

    4.  **Non-Blocking I/O (`epoll`):** The networking layer was designed around `epoll` in edge-triggered (`EPOLLET`) mode. This is the industry standard for high-throughput servers, allowing a single thread to manage thousands of connections without the overhead of thread-per-client models and without ever blocking on a `read()` or `write()` call.

---

### **Day 2: Core Logic & Cache-Conscious Design**

* **Date:** July 29, 2025
* **Objective:** To implement the core matching engine with data structures explicitly designed to maximize CPU cache performance.

* **Technical Decisions & Rationale:**

    1.  **Cache-Friendly Order Book:** The `OrderBook` was implemented using arrays, not trees. While a balanced binary tree offers O(log n) search complexity, this is a misleading metric in a memory-bound system. Each node traversal in a tree is a potential cache miss (a multi-hundred-cycle penalty). Our array-based design guarantees **data locality**. When the matching engine iterates through price levels, it performs a linear scan over physically contiguous memory, which is ideal for the CPU's prefetcher and results in an extremely high L1/L2 cache hit rate.

    2.  **O(1) Order Cancellation:** A critical performance requirement is the ability to cancel an order instantly. To achieve this, we implemented a hybrid data structure. While orders are stored in arrays within each `PriceLevel`, a global direct-lookup table (`Order* orders_by_id[]`) was added to the main `Exchange` struct. This allows the system to locate any order by its ID in constant time, O(1). To complete the fast removal, each `Order` struct contains a back-pointer to its parent `PriceLevel`, eliminating the need for a secondary search.

    3.  **Robust Stream Parsing:** The initial network handler was vulnerable to subtle timing issues due to the nature of TCP streams. It was re-engineered to be fully robust by creating a dedicated state object (`ConnectionState`) for each client. This object contains a private buffer, allowing the server to correctly accumulate partial messages and process multiple messages from a single `read()` call, making the system resilient to network timing variations.

---

### **Day 3: Concurrency, Hardware Affinity & Final Hardening**

* **Date:** July 30, 2025
* **Objective:** To transition from a single-threaded model to a full, multi-threaded, hardware-aware system, eliminating the final sources of latency.

* **Technical Decisions & Rationale:**

    1.  **Decoupled Multi-Threading:** The system was split into three specialized threads: **Network**, **Processing**, and **Logging**. This isolates the unpredictable latency of I/O (network and disk) from the deterministic, CPU-bound work of the matching engine.

    2.  **Lock-Free SPSC Queues:** Communication between these threads is handled by custom Single-Producer, Single-Consumer (SPSC) ring buffers built on **C11 atomics**. This is the project's most advanced concurrency feature. By using hardware atomic instructions instead of `mutex` locks, we enable threads to exchange data with **zero kernel involvement**. This completely eliminates the catastrophic performance penalties of context switching, thread sleeping, and scheduler contention.

    3.  **CPU Affinity:** Each of the three threads is pinned to a dedicated CPU core using `pthread_setaffinity_np()`. This is a non-negotiable step for achieving predictable performance. It prevents the OS scheduler from migrating our threads, which would cause a complete L1/L2 cache flush—a multi-thousand-cycle penalty. Pinning guarantees maximum cache residency and eliminates a major source of system jitter.

    4.  **Cache Warming & The 7x Performance Leap:** The final optimization was to combat the "cold cache" problem on startup. A `system_warmup()` routine was implemented to pre-load all critical data structures and functions into the CPU's caches before processing the first real message. This single change yielded a **7x reduction in latency for the first transaction** (from ~20,000 to ~3,000 CPU cycles), ensuring the system operates at peak performance from the very first order.
