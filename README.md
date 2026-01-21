# Ultra-Low Latency Bitcoin Arbitrage Simulator (C++)

## Overview
System Architecture Showcase designed to simulate the engineering constraints of High-Frequency Trading (HFT). A high-performance market microstructure simulator that models institutional crypto arbitrage with ultra-low latency tracking.

## Key Features

### Low-Latency Engineering
Lock-Free Concurrency: Implemented a custom SPSC (Single-Producer Single-Consumer) Ring Buffer to pass market data from Network threads to the Strategy thread without mutex lock.

Zero-Allocation Hot Path: The critical strategy loop avoids new/malloc completely to prevent heap fragmentation and GC pauses.

SIMD Parsing: Used simdjson to parse high-volume JSON feeds using AVX2 instructions.

Optimized Time Parsing: Custom implementation to convert ISO8601 timestamps to UNIX epoch to avoid using slow standard library calls.

### Robust System Architecture
Jitter Spike Detection: Detects and neutralizes clock drift and network jitter inherent in non-real-time OS environments (WSL2).

Decoupled Rendering: The UI runs on 10 FPS (cold path), ensuring that console I/O never blocks the unlimited data fetch (hot path).

Institutional Simulation: Calculting real profits after fees (Regular 0.6% vs. VIP 0.017%).

## Performance Metrics
Strategy Compute Latency: ~300 - 450 nanoseconds

Measured from packet dequeue to decision ready.

Wire-to-Wire Latency: 0 - 30ms (Network dependent)

## System Architecture
The application runs on three dedicated threads to ensure separation of concerns:

Network Thread (I/O): Handles async WebSocket feeds via Boost.Asio. Pushes raw data to the Ring Buffer.

Strategy Thread (Hot Path): Pops data, updates Order Book, calculates spread, and records latency metrics. (Pinned Priority)

Dashboard Thread (Cold Path): Reads state atomically and renders the ANSI-based UI at 10Hz.

## Prerequisites & Build
Dependencies
C++20 Compliant Compiler (GCC 10+ / Clang 11+)

Boost.Asio & Boost.Beast
OpenSSL
simdjson

CMake 3.10+

## Build Instructions

1. Clone the repository
```Bash 
git clone https://github.com/zjavlt/CPTHFT_Bot.git
cd hft-bot
```
2. Create build directory
```Bash 
mkdir build && cd build
```

3. Compile
```Bash
cmake ..
make -j4
```
4. Run

```Bash
# Run with highest process priority (-20)
sudo nice -n -20 ./HFT_Bot
```

## Room for Improvements

- Replace double precision with a int calculations to eliminate floating-point error

- Implement a ring-buffer based logger to completely separate logger and the hot path