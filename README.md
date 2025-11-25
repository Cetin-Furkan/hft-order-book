# NASDAQ ITCH 5.0 Feed Handler & MoldUDP64 Publisher

> **A deterministic, hardware-tuned market data replay engine architected for the C23 standard and next-generation wireless infrastructure.**

This repository houses a production-grade **Market Data Feed Handler (V2)** engineered to replay terabytes of raw NASDAQ TotalView-ITCH 5.0 data. Unlike academic implementations, this system is built to serve as the "external nervous system" for downstream High-Frequency Trading (HFT) strategies, prioritizing nanosecond-level precision, strict protocol compliance, and predictable network behavior on commodity hardware.

-----

## 🏛 Architectural Evolution: V1 vs. V2

The transition from V1 to V2 represents a paradigm shift from **Algorithmic Logic** to **Systems Infrastructure**. While V1 focused on the internal matching engine (O(1) lookups, memory arenas), V2 addresses the physical transport layer, requiring a mastery of the Linux networking stack, ISA-level intrinsics, and the C23 standard.

| Architecture Component | **V1: In-Memory Engine** | **V2: Network Infrastructure (Feed Handler)** |
| :--- | :--- | :--- |
| **Primary Domain** | Data Structures & Algorithms. | Systems Programming & Network Engineering. |
| **Input Stream** | Synthetic/Sanitized Structs. | **Raw 12GB+ Binary Streams** (Big-Endian Wire Format). |
| **Protocol Stack** | Internal API. | **Full NASDAQ ITCH 5.0** (22 Message Types) encapsulated in **MoldUDP64**. |
| **Standard Compliance** | C11/C17 (Legacy). | **C23** (Leveraging `stdckdint.h` & `constexpr`). |
| **Transport Layer** | None (Pointer passing). | **Hardware-Tuned Multicast** (Intel BE200 / iwlwifi optimized). |
| **Safety Mechanism** | Manual Branching Checks. | **Hardware-Intrinsic Overflow Protection** (`JO`/`CF` flags). |

-----

## 🧬 Core Engineering Principals (V2)

### 1\. The "Zero-Drift" Parsing Engine

Most open-source parsers implement the "Happy Path" (Add/Execute/Cancel), ignoring rare administrative messages. In a binary stream, skipping 12 bytes of an unknown message type causes a permanent offset shift, rendering gigabytes of subsequent data as garbage ("parser drift").

**V2 implements the exhaustive NASDAQ ITCH 5.0 specification**, including:

  * **Administrative:** `mwcb_decline` (Circuit Breakers), `ipo_quoting`, `reg_sho` (Short Sale Restrictions).
  * **Critical Fixes:** Correct implementation of the **Type 'O' (Direct Listing with Capital Raise)** message.
      * *Engineering Note:* The `near_execution_time` field in Type 'O' is 8 bytes (`uint64_t`), not 4. Misinterpreting this field is a common failure mode in legacy parsers dealing with post-2023 datasets.

### 2\. C23: Leveraging Modern Standards for Safety

We migrated to **ISO/IEC 9899:2024 (C23)** to utilize zero-cost abstractions that map directly to CPU flags.

  * **Checked Arithmetic (`<stdckdint.h>`):**
    Sequence number generation is critical. Standard `+` operators risk undefined behavior or silent wrapping.
      * *Implementation:* Replaced manual checks with `ckd_add()`. On x86\_64, this compiles directly to the `ADD` instruction followed by a jump on the Carry Flag (`JC`) or Overflow Flag (`JO`). This provides mathematical safety with **zero instruction pipeline bubbling** compared to branch-heavy manual validation.
  * **Deterministic Memory Layout:**
    Utilized `#pragma pack(1)` alongside C23 `constexpr` to ensure in-memory structures map 1:1 to the wire format, eliminating compiler-inserted padding bytes that destroy binary compatibility.

### 3\. Hardware-Aware Network Tuning (Intel Wi-Fi 7 / BE200)

Running high-frequency multicast over 802.11be (Wi-Fi 7) presents unique physical layer challenges compared to 10GbE fiber. The replay engine handles the specific physics of the `iwlwifi` driver:

  * **MTU & Fragmentation:** Packet sizes are strictly capped at **1400 bytes**. This leaves specific headroom for the MoldUDP64 header (20 bytes), IPv4 header (20 bytes), and UDP header (8 bytes) to fit within the standard 1500-byte MTU, preventing kernel-level IP fragmentation which is catastrophic for UDP reliability.
  * **Micro-Burst Traffic Shaping:**
      * *The Problem:* "Firehosing" UDP packets saturates the NIC's TX ring buffer faster than the Wi-Fi PHY can aggregate MPDUs (Mac Protocol Data Units), causing head-of-line blocking and non-deterministic latency spikes.
      * *The Solution:* Implemented a `BUSY_WAIT_NS` spinloop (tuned to \~2µs). This micro-pause allows the `iwlwifi` firmware to flush the TX ring to the air interface linearly, maintaining a consistent inter-packet arrival time (IAT) for clients.
  * **Multicast Routing:** Explicit sets for `IP_MULTICAST_TTL=16` and `SO_REUSEADDR`, allowing the feed to traverse local switching fabric and survive rapid process restarts during stress testing.

### 4\. Instruction-Level Optimization

  * **Endianness Intrinsics:**
    Financial data is Big-Endian (Network Byte Order). x86\_64 is Little-Endian.
      * *V1 Approach:* Manual bit-shifting macros (`(x << 8) | (x >> 8)`).
      * *V2 Approach:* Replaced with Linux Native `<endian.h>` intrinsics (`htobe64`, `be16toh`). This compiles deterministically to the **`BSWAP` assembly instruction**, reducing register pressure and CPU cycles per message parse.

-----

## 📂 Infrastructure Layout

  * **`v2/` (Production Environment)**
      * `main.c`: The hardened MoldUDP64 publisher. Contains the replay loop, socket option tuning, and C23 safe-math logic.
      * `itch_protocol.h`: The "Golden" header. Defines the exact byte-layout of all 22 ITCH message types using packed structs.
  * **`src/` (Legacy V1)**
      * Reference implementation of the in-memory order book (Arenas, Lock-free queues).

-----

## ⚡ Deployment & Compilation

This system requires a modern toolchain compliant with the C23 standard.

**Prerequisites:**

  * GCC 13+ or Clang 16+ (Required for `-std=c2x`/`-std=c23`).
  * Linux Kernel 5.10+ (For modern socket options and `io_uring` compatibility in future extensions).
  * **Input Data:** Raw, uncompressed NASDAQ ITCH 5.0 binary files (e.g., `01302019.NASDAQ_ITCH50.bin`).

**Build Command:**

```bash
# Compile with C23 standard, Level 3 Optimization, and all warnings enabled
gcc -std=c2x -O3 -Wall -Wextra -Wpedantic itch-reader/itch_protocol.c -o nasdaq_feed
```

**Execution:**

```bash
# Usage: ./nasdaq_feed <binary_file> <interface_ip>
# interface_ip: The local IP of your Intel BE200 card (to bind the multicast route).

./nasdaq_feed data/01302019.NASDAQ_ITCH50.bin 192.168.1.X
```

-----

## 🔬 Performance & Latency Profile

This system is engineered for **deterministic latency**, not just raw throughput. By manually controlling the packetization loop, enforcing memory alignment, and managing CPU sleep states, we achieve a replay stream that mimics the behavior of an FPGA-based feed handler.

The use of `clock_nanosleep` (implied via helpers) and busy-wait loops ensures that the replay respects the physical limitations of the wireless medium while maintaining the logical sequence integrity required by institutional trading engines.
