# Engineering Journey: The Ares Project

This document chronicles my 30-day intensive journey into the world of high-performance systems engineering. It is both a personal diary of the concepts I have studied and a manifesto for the kind of software I aim to build.

## My Philosophy & Goals

This project is a testament to my capacity for rapid, deep learning and my ambition to build complex, high-performance systems. My goal extends beyond a single domain like exchange technology; I aim to master the art of engineering software for high-compute environments, with a specific focus on extracting maximum performance from the entire **Intel hardware ecosystem**—from the CPU and iGPU to specialized NICs and FPGAs.

This journey is proof of my commitment. Despite the constraints of a demanding physical job, I dedicated the first nine days of this period exclusively to intensive theoretical study, building a robust foundation before writing a single line of code. This document logs the vast range of topics I have absorbed in that short time:

* **Hardware Internals:** Deep microarchitecture of CPUs, cache coherency, and the mechanics of SIMD.

* **Advanced Algorithms:** High-performance data structures and algorithms for streaming data.

* **Modern Networking:** The intricacies of UDP, multicast, and modern transport protocols like QUIC/HTTP3.

* **Systems Programming:** Kernel interactions, memory models, advanced debugging, and performance profiling.

* **Coding Philosophy:** A shift from complexity to a disciplined approach centered on clarity, measurement, and targeted optimization.

My objective for this project is to build the "Ares" market data handler as a real-world application of this knowledge. It will be engineered to process live financial data from a real NASDAQ protocol (ITCH), demonstrating not just code optimization, but a holistic understanding of the entire system stack.

**What comes next?** The completion of the Ares project is not an end, but the foundation for the next stage of my growth. My immediate plan is to synthesize this experience into a new, more ambitious project. I will deepen my knowledge of modern networking by working with specialized NICs, and use this expertise to build my own high-performance server environment on a custom virtual machine. Concurrently, I will explore the advanced technologies I've learned about, like building a high-fidelity backtester and beginning practical development with FPGA devices. Throughout this process, I will continue to upgrade my knowledge of the Intel architecture, combining everything I learn to gain even more practical, real-world experience.

## The Learning Diary

### **Day 1: Friday, July 30, 2025**

**Topic:** Mechanical Sympathy & CPU Architecture.
Today marks a fundamental shift in my thinking. I've moved from seeing the computer as an abstract machine to a physical one with measurable characteristics. The core concept is "Mechanical Sympathy"—understanding the hardware to write software that works with it, not against it. I've studied the specific architecture of my Intel i7-11th Gen CPU ("Cypress Cove"), memorizing its cache hierarchy (L1/L2/L3 sizes and latencies) and understanding that the 64-byte cache line is the fundamental unit of memory transfer. This is the foundation for everything else.

### **Day 2: Saturday, July 31, 2025**

**Topic:** Advanced Cache Mechanics & False Sharing.
Went deeper into the rules of the cache. Learned about N-way set associativity and how it dictates where memory can be placed. Studied the MESI cache coherency protocol, which is how multiple cores keep their data in sync. This led to the discovery of a critical performance bug: **false sharing**. This happens when independent variables used by different threads land on the same cache line, causing the cores to fight a constant, invisible war for ownership of that line. The solution is explicit cache-line padding, a technique I will use in all my concurrent data structures from now on.

### **Day 3: Sunday, August 1, 2025**

**Topic:** CPU Pipelines & Branch Prediction.
Today was about looking inside the CPU core itself. I learned that modern CPUs use a deep instruction pipeline, like an assembly line, to work on multiple instructions at once. The key is to keep this pipeline full. The biggest enemy of a full pipeline is a **branch misprediction**. I studied how the CPU's branch predictor tries to guess the outcome of `if` statements and the massive performance penalty (a full pipeline flush) it pays for guessing wrong. This has completely changed how I will approach writing conditional logic in tight loops.

### **Day 4: Monday, August 2, 2025**

**Topic:** Modern CPU Microarchitecture (μops).
This was a deep dive. I learned that modern x86 CPUs don't execute x86 instructions directly. They first translate them in the "Front-End" into simple, RISC-like internal instructions called **micro-operations (μops)**. The "Back-End" is then a powerful Out-of-Order engine that executes these μops in parallel. This lesson taught me how to help the compiler and CPU by writing code with independent instruction chains, allowing the Out-of-Order engine to find more Instruction-Level Parallelism.

### **Day 5: Tuesday, August 3, 2025**

**Topic:** SIMD (Single Instruction, Multiple Data).
Learned how to command the CPU to perform parallel computations using its wide vector registers. I studied the AVX2 and AVX-512 instruction sets available on my CPU. The core of today's lesson was learning to use **compiler intrinsics** (`_mm256_...`) to write C code that maps directly to these powerful SIMD instructions, allowing me to process 8 floats or 4 doubles in a single clock cycle.

### **Day 6: Wednesday, August 4, 2025**

**Topic:** Memory Alignment.
A direct follow-up to SIMD. I learned that for maximum performance, SIMD operations require their data to be aligned to the vector size (e.g., 32 bytes for AVX2). An unaligned access can cause a **cache-line split**, forcing the CPU to perform two memory fetches instead of one. I learned how to use `aligned_alloc` to request properly aligned memory from the OS, a crucial technique for any serious performance work.

### **Day 7: Thursday, August 5, 2025**

**Topic:** Data-Oriented Design (AoS vs. SoA).
This lesson connected all the cache and memory concepts. I studied how the compiler pads structs and the massive performance difference between an Array of Structures (AoS) and a Structure of Arrays (SoA). By organizing data based on how it will be processed (SoA), I can ensure perfect cache efficiency and create data layouts that are ideal for SIMD vectorization. This principle is fundamental and applies to CPU, GPU, and AI programming.

### **Day 8: Friday, August 6, 2025**

**Topic:** Performance Profiling with `perf`.
Theory is one thing, but measurement is everything. Today I learned how to use the professional Linux profiler, `perf`. I now understand how to use it to find "hot spots" in my code and, more importantly, how to use its access to hardware **Performance Monitoring Counters (PMCs)** to understand *why* the code is slow. I can now directly measure `cache-misses`, `branch-misses`, and `IPC` to verify my optimization hypotheses.

### **Day 9: Friday, August 8, 2025**

**Topic:** The Shift in Philosophy.
Today was less about a new technical topic and more about a new way of thinking. After reviewing the past week, I've realized the flaws in my old approach to coding. My new philosophy is to prioritize clarity and simplicity in design. I will build robust, testable, and understandable systems first. Only then, guided by measurement from tools like `perf`, will I apply targeted, aggressive optimizations to the proven bottlenecks. This is the path to creating software that is not just fast, but also stable and maintainable.
