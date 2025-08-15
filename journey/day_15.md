# Today's Journey: From Theory to a Live System

Today was a pivotal day. We transitioned from the world of pure theory into the practical realm of engineering. The goal was not just to write code, but to take the vast library of concepts we've built—from cache mechanics to network protocols—and forge them into the foundation of a real, high-performance application. This report details that journey.

## The Initial Challenge: Taming the Build System

Our day began not with algorithms, but with the essential and often overlooked foundation of any serious project: the build system. We chose `CMake` and the Intel `icx` compiler to establish a professional, performance-oriented environment from the start.

* **Why did we do this?** A simple `gcc` command is fine for one file, but a real system requires a way to manage dependencies, handle different build types (Debug vs. Release), and automate compilation. Starting with a professional build system instills discipline and saves immense time later.

* **How did we do it?** We created a `CMakeLists.txt` file. This immediately led to a series of real-world debugging challenges:

  1. We solved an **environment issue** by learning that the Intel compiler wasn't in the system's `PATH` and needed the `setvars.sh` script to be sourced.

  2. We learned a core `CMake` principle—**order of operations**—by fixing errors that occurred because we were trying to configure a target before it was created.

  3. We saw how the build system depends on the existence of source files, fixing the final configuration error by creating a placeholder `main.c`.

This initial phase was a critical lesson in itself: building software isn't just about writing C code; it's about mastering the entire toolchain that turns that code into a working program.

## Milestone 2: From Raw Bytes to Structured Intelligence

With a working receiver, we moved to the brain of the operation: understanding the data.

* **Why did we do this?** Receiving raw bytes is useless without the ability to interpret them. This milestone was about transforming that raw data into a structured, meaningful representation of the market.

* **How did we do it?**

  1. **Zero-Copy Parsing:** We implemented the core of our high-performance design. By creating `__attribute__((packed))` structs in `itch_protocol.h`, we built a parser that performs no memory copies. The cast `(AddOrderMessage*)buffer` is the physical manifestation of this theory—it's instantaneous and maximally efficient.

  2. **Building the Order Book:** We created the `OrderBook` data structure, applying the principles of Data-Oriented Design. This led to the most important bug and lesson of the day: the **Segmentation Fault**.

  3. **The Stack vs. The Heap:** We diagnosed the crash as a **stack overflow**. This was a practical, visceral demonstration of a fundamental computer science concept. We learned that small, local variables live on the fast but limited stack, while large data structures like our multi-megabyte `OrderBook` **must** be allocated on the vast, flexible heap using `malloc`. This is a lesson every systems programmer must learn, and we learned it through direct experience.

## Milestone 3: Achieving True Parallelism

The final and most significant step of the day was to evolve our single-threaded program into its final, high-performance, multi-threaded architecture.

* **Why did we do this?** A single-threaded design is a bottleneck. The program is either waiting for the network or processing data, but it can never do both at once. To achieve high throughput, we must decouple these concerns.

* **How did we do it?**

  1. **The SPSC Queue:** We built the lock-free, cache-padded SPSC queue. This is the "conveyor belt" that connects our two threads, built using the C11 atomics and memory ordering principles we studied.

  2. **The Great Refactoring:** We completely rewrote `main.c` to be a pure **orchestrator**. It now creates a dedicated **Network Thread** and a **Processing Thread**.

  3. **CPU Affinity:** We used `pthread_setaffinity_np` to pin each thread to a specific CPU core. This is the practical application of our hardware knowledge—preventing OS scheduler jitter and maximizing cache performance.

The final, successful test run proved the architecture works. But it also revealed a critical, real-world phenomenon: an out-of-order message. This wasn't a failure; it was a success, because it perfectly demonstrated *why* our next and final step—a sequencing engine—is not an optional feature, but an absolute requirement for a robust system.
)
