# Ultra-Low Latency Bitcoin Arbitrage Simulator (C++20)

## Overview
System Architecture Showcase designed to simulate the engineering constraints of High-Frequency Trading (HFT). A high-performance market microstructure simulator that models institutional crypto arbitrage with ultra-low latency tracking.

## Key Features

### Low-Latency Engineering
Lock-Free Concurrency: Implemented a custom SPSC (Single-Producer Single-Consumer) Ring Buffer to pass market data from Network threads to the Strategy thread without mutex contention.

Zero-Allocation Hot Path: The critical strategy loop avoids new/malloc completely to prevent heap fragmentation and GC pauses.

SIMD-Accelerated Parsing: Utilizes simdjson to parse high-volume JSON feeds using AVX2 instructions, significantly faster than standard libraries.

Optimized Time Parsing: Custom implementation to convert ISO8601 timestamps to UNIX epoch in microseconds, bypassing slow standard library calls like strptime.

### Robust System Architecture
Jitter Normalization Engine: Features a dynamic "floor tracking" algorithm that detects and neutralizes clock drift and network jitter inherent in non-real-time OS environments (WSL2).

Decoupled Rendering: The UI runs on a strict 10 FPS cold path, ensuring that console I/O never blocks the unlimited speed hot path.

Institutional Simulation: Dual-ledger profit calculation showing "True Profit" after fees (Retail 0.6% vs. VIP 0.017%), proving mathematical constraints of crypto arbitrage.

## Performance Metrics
Strategy Compute Latency: ~300 - 450 nanoseconds

Measured from packet dequeue to decision ready.

Wire-to-Wire Latency: 0 - 30ms (Network dependent)

Throughput: Capable of ingesting 10k+ ticks/second with zero backpressure.

## System Architecture
The application runs on three dedicated threads to ensure separation of concerns:

Network Thread (I/O): Handles async WebSocket feeds via Boost.Asio. Pushes raw data to the Ring Buffer.

Strategy Thread (Hot Path): Pops data, updates Order Book, calculates spread, and records latency metrics. (Pinned Priority)

Dashboard Thread (Cold Path): Reads state atomically and renders the ANSI-based UI at 10Hz.

## Prerequisites & Build
Dependencies
C++20 Compliant Compiler (GCC 10+ / Clang 11+)

Boost.Asio & Boost.Beast (Networking)

OpenSSL (Secure WebSockets)

simdjson (Fast Parsing)

CMake 3.10+

## Build Instructions
Bash
# 1. Clone the repository
git clone https://github.com/yourusername/hft-arbitrage-sim.git
cd hft-arbitrage-sim

# 2. Create build directory
mkdir build && cd build

# 3. Compile
cmake ..
make -j4
Run Instructions (Critical)
To ensure the Strategy Thread gets CPU priority over OS background tasks, run with nice:

Bash
# Run with highest process priority (-20)
sudo nice -n -20 ./HFT_Bot
🔮 Future Improvements
While this simulator achieves microsecond-level performance, the following optimizations would be required for a production-grade live trading system:

Kernel Bypass: Migrate from WSL2/Linux Kernel networking to Solarflare OpenOnload or DPDK to bypass the OS network stack and eliminate the 30ms jitter entirely.

Fixed-Point Arithmetic: Replace double precision with a Fixed-Point math library to eliminate floating-point non-determinism.

Hardware Acceleration: Offload the limit order book matching logic to an FPGA for sub-microsecond wire-to-wire latency.

Async Logging: Implement a ring-buffer based logger to write trade logs to disk without blocking the hot path.