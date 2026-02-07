# MT25042
# Graduate Systems (CSE638) - PA02: Analysis of Network I/O Primitives

## Roll Number: MT25042

## Assignment Overview

This assignment studies the cost of data movement in network I/O by implementing
and comparing three TCP socket communication strategies:

- **Two-Copy** (A1): Standard `send()`/`recv()` with serialized buffer
- **One-Copy** (A2): `sendmsg()` with scatter-gather `iovec` — eliminates user-space copy
- **Zero-Copy** (A3): `sendmsg()` with `MSG_ZEROCOPY` — kernel pins user pages for DMA

Each implementation consists of a multithreaded TCP server (one thread per client)
and a multithreaded client measuring throughput and latency.

---

## File Structure

```
MT25042_Part_A_Common.h         # Common header: message struct, helpers, timing
MT25042_Part_A1_Server.c        # Two-copy server (send)
MT25042_Part_A1_Client.c        # Two-copy client (recv)
MT25042_Part_A2_Server.c        # One-copy server (sendmsg/iovec)
MT25042_Part_A2_Client.c        # One-copy client
MT25042_Part_A3_Server.c        # Zero-copy server (MSG_ZEROCOPY)
MT25042_Part_A3_Client.c        # Zero-copy client
MT25042_Part_C_Experiment.sh    # Automated experiment script
MT25042_Part_D_Plots.py         # Matplotlib plots (hardcoded data)
MT25042_Part_B_Results.csv      # Raw experimental measurements
Makefile                        # Build automation
README.md                       # This file
MT25042_Report.pdf              # Analysis report
```

---

## Prerequisites

- **Linux** with kernel ≥ 4.14 (for MSG_ZEROCOPY support)
- **GCC** with pthread support
- **perf** tool (`perf stat`)
- **Network namespaces** (`ip netns`) — requires root access
- **Python 3** with `matplotlib` (for generating plots)

### Installation (Arch/CachyOS)
```bash
sudo pacman -S base-devel linux-tools python python-matplotlib
```

---

## Building

```bash
make            # Build all 6 binaries
make clean      # Remove compiled executables
make help       # Show available targets
```

Produces: `a1_server`, `a1_client`, `a2_server`, `a2_client`, `a3_server`, `a3_client`

---

## Running Manually

### Start a server (in one terminal / namespace):
```bash
# Two-copy, 4096-byte messages, accept up to 4 clients:
./a1_server 4096 4

# One-copy:
./a2_server 4096 4

# Zero-copy:
./a3_server 4096 4
```

### Start corresponding client (in another terminal / namespace):
```bash
# Connect to server at 10.0.0.1, msg_size=4096, 4 threads, run for 10 seconds:
./a1_client 10.0.0.1 4096 4 10
```

---

## Running the Full Experiment Suite

The automated script sets up Linux network namespaces, compiles everything,
runs all experiments, and collects results with `perf stat`:

```bash
sudo ./MT25042_Part_C_Experiment.sh
```

This will:
1. Compile all 6 binaries
2. Create `ns_server` and `ns_client` namespaces connected via veth pair
3. Run 48 experiments (3 implementations × 4 message sizes × 4 thread counts)
4. Collect throughput, latency, CPU cycles, L1/LLC cache misses, context switches
5. Output results to `MT25042_Part_B_Results.csv`
6. Clean up namespaces on exit

**Parameters tested:**
- Message sizes: 1024, 4096, 16384, 65536 bytes
- Thread counts: 1, 2, 4, 8
- Duration: 10 seconds per experiment

---

## Generating Plots

```bash
python3 MT25042_Part_D_Plots.py [output_directory]
```

Generates 4 PNG plots with hardcoded experimental data:
1. Throughput vs Message Size
2. Latency vs Thread Count
3. Cache Misses (L1 + LLC) vs Message Size
4. CPU Cycles per Byte vs Message Size

---

## System Configuration

| Property    | Value                           |
|-------------|---------------------------------|
| OS          | CachyOS Linux (rolling)         |
| Kernel      | 6.18.8-2-cachyos                |
| CPU         | 11th Gen Intel Core             |
| Memory      | 32 GB                           |
| Compiler    | GCC 15.2.1                      |
| perf        | 6.18-3                          |

---

## AI Usage Declaration

The following components were developed with AI assistance (ChatGPT):

- **Part A (Common header)**: Asked about `sendmsg()` iovec layout and `MSG_ZEROCOPY` completion flow
- **Part A1**: Asked for multithreaded TCP server pattern with `send()`/`recv()`
- **Part A2**: Asked how `sendmsg` with `iovec` reduces copies vs plain `send`
- **Part A3**: Asked about `MSG_ZEROCOPY` and `MSG_ERRQUEUE` notification handling
- **Part C**: Asked how to set up Linux network namespaces with veth for local TCP testing
- **Part D**: Asked for matplotlib multi-line plot structure with legends

All generated code was reviewed, understood, and adapted to fit the assignment requirements.

---

## GitHub Repository

URL: https://github.com/ShahnawazDev/GRS-Assignments
