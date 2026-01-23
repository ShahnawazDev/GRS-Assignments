# MT25042
# Graduate Systems (CSE638) - PA01: Processes and Threads

## Roll Number: MT25042


## Assignment Overview

This assignment explores the differences between processes and threads in Linux through practical implementation and measurement. It consists of four parts:

- **Part A**: Create programs using `fork()` (processes) and `pthread` (threads)
- **Part B**: Implement CPU-intensive, memory-intensive, and I/O-intensive worker functions
- **Part C**: Measure and analyze performance metrics for 6 program combinations
- **Part D**: Analyze scalability with varying process/thread counts

---

## File Structure

```
├── MT25042_Part_A_Program_A.c    # Program A: fork-based process creation
├── MT25042_Part_A_Program_B.c    # Program B: pthread-based thread creation
├── MT25042_Part_B_workers.c      # Worker function implementations
├── MT25042_Part_B_workers.h      # Worker function declarations
├── MT25042_Part_C_script.sh      # Automation script for Part C
├── MT25042_Part_D_script.sh      # Automation script for Part D
├── MT25042_Part_D_plot.py        # Python plotting script
├── Makefile                       # Build automation
└── README.md                      # This file
```

---

## Prerequisites

### Required Tools
- GCC compiler with pthread support
- Linux system with:
  - `top` - CPU/memory monitoring
  - `iostat` - I/O statistics (from `sysstat` package)
  - `taskset` - CPU affinity setting
  - `time` - execution time measurement
- Python 3 with `matplotlib` (for plotting)

### Installation (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install build-essential sysstat python3 python3-matplotlib
```

---

## Building the Programs

### Compile all programs:
```bash
make
```

### Clean build artifacts:
```bash
make clean
```

---

## Usage

### Running Individual Programs

**Program A (Processes):**
```bash
./program_a <worker_type> [num_processes]
# Examples:
./program_a cpu 2      # Run 2 processes with CPU-intensive worker
./program_a mem 4      # Run 4 processes with memory-intensive worker
./program_a io 3       # Run 3 processes with I/O-intensive worker
```

**Program B (Threads):**
```bash
./program_b <worker_type> [num_threads]
# Examples:
./program_b cpu 2      # Run 2 threads with CPU-intensive worker
./program_b mem 6      # Run 6 threads with memory-intensive worker
./program_b io 4       # Run 4 threads with I/O-intensive worker
```

### Worker Types
| Type | Description |
|------|-------------|
| `cpu` | CPU-intensive: Pi calculation, trigonometric functions, matrix operations |
| `mem` | Memory-intensive: Large array operations, random memory access |
| `io` | I/O-intensive: File read/write with fsync |

---

## Part C: Basic Measurements

Run the automated measurement script:
```bash
make run_c
# Or directly:
./MT25042_Part_C_script.sh [cpu_list]
```

This script:
1. Executes all 6 combinations (A+cpu, A+mem, A+io, B+cpu, B+mem, B+io)
2. Collects CPU%, Memory, I/O, and execution time metrics
3. Outputs results to `MT25042_Part_C_CSV.csv`

### Output Format
```
Program+Function    CPU%    Mem(KB)    IO_Read    IO_Write    Time(s)
Program_A+cpu       ...     ...        ...        ...         ...
Program_A+mem       ...     ...        ...        ...         ...
...
```

---

## Part D: Scalability Analysis

Run the extended measurement script:
```bash
make run_d
# Or directly:
./MT25042_Part_D_script.sh [cpu_list]
```

This script:
1. Tests Program A with 2, 3, 4, 5 processes
2. Tests Program B with 2, 3, 4, 5, 6, 7, 8 threads
3. Generates plots showing trends
4. Outputs results to `MT25042_Part_D_CSV.csv`

### Generated Plots
- `MT25042_Part_D_CPU_Plot.png` - CPU usage vs count
- `MT25042_Part_D_Time_Plot.png` - Execution time vs count
- `MT25042_Part_D_Memory_Plot.png` - Memory usage vs count
- `MT25042_Part_D_Combined_Plot.png` - All metrics comparison

---

## Configuration

### Loop Count
The loop count is defined in `MT25042_Part_B_workers.h`:
```c
#define LOOP_COUNT 5000  // last_digit × 1000
```

**Update this value based on your roll number's last digit!**
- If last digit is 0, use 9000
- Otherwise, use (last_digit × 1000)

### CPU Affinity
By default, programs are pinned to CPUs 0,1. Change via script argument:
```bash
./MT25042_Part_C_script.sh 0,1,2,3
```

---

## Worker Function Details

### CPU Worker (`worker_cpu`)
- **Algorithm**: Leibniz series for Pi, trigonometric calculations, matrix multiplication
- **Characteristics**: High CPU utilization, minimal memory/I/O
- **Expected Behavior**: CPU% near 100% per process/thread

### Memory Worker (`worker_mem`)
- **Algorithm**: Large array allocation (16MB), random access patterns, memcpy
- **Characteristics**: Memory bandwidth limited, cache misses
- **Expected Behavior**: Moderate CPU%, high memory usage

### I/O Worker (`worker_io`)
- **Algorithm**: File creation, write (1MB blocks), read, fsync
- **Characteristics**: I/O wait time dominant
- **Expected Behavior**: Low CPU%, high I/O statistics

---

## Troubleshooting

### "iostat not found"
```bash
sudo apt-get install sysstat
```

### "Permission denied" on scripts
```bash
chmod +x MT25042_Part_C_script.sh MT25042_Part_D_script.sh
```

### "matplotlib not found"
```bash
pip3 install matplotlib
```

### Programs not found
```bash
make clean && make
```

---

## AI Usage Declaration

The following components were generated with AI assistance:
- Code structure and organization
- Worker function implementations
- Bash automation scripts
- Python plotting script
- Makefile
- README documentation

All code has been reviewed and understood.

---


## GitHub Repository
Repo that will contain all GRS assignments
URL: https://github.com/ShahnawazDev/GRS-Assignments
