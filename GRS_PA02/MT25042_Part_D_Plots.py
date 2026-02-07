#!/usr/bin/env python3
"""
MT25042_Part_D_Plots.py
Graduate Systems (CSE638) - PA02: Analysis of Network I/O primitives

Generates all required plots for Part D using matplotlib with
hardcoded experimental data measured on the test system.

System: CachyOS Linux, Kernel 6.18.8-2-cachyos, GCC 15.2.1
CPU: 11th Gen Intel Core, NVIDIA GeForce GPU
Memory: 32 GB

Plots generated:
  1. Throughput (Gbps) vs Message Size
  2. Latency (µs) vs Thread Count
  3. Cache Misses vs Message Size
  4. CPU Cycles per Byte Transferred

AI Declaration: Asked ChatGPT for help with matplotlib multi-line plot
  layout and legend positioning. Prompt: "How to plot throughput vs
  message size with multiple lines for different implementations
  in matplotlib with proper legends?"
"""

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import os
import sys

# ============================================================================
# Hardcoded experimental data (from perf stat measurements on CachyOS)
# ============================================================================

MSG_SIZES = [1024, 4096, 16384, 65536]
THREAD_COUNTS = [1, 2, 4, 8]

# ---- Throughput (Gbps) ----
# Format: throughput[impl][msg_size_idx][thread_idx]
throughput = {
    'two_copy': [
        [4.0526, 7.2182, 11.5245, 15.0278],     # 1024 bytes
        [9.3792, 17.6207, 29.5292, 36.8553],     # 4096 bytes
        [28.4381, 47.3265, 75.9788, 92.0485],    # 16384 bytes
        [62.1391, 108.8889, 153.0088, 72.6156],  # 65536 bytes
    ],
    'one_copy': [
        [3.4985, 6.1208, 9.5012, 13.1207],       # 1024 bytes
        [8.1453, 14.1012, 22.5464, 32.1907],     # 4096 bytes
        [28.1047, 45.7111, 70.8288, 102.6077],   # 16384 bytes
        [60.8287, 99.6969, 136.3410, 70.7146],   # 65536 bytes
    ],
    'zero_copy': [
        [2.5996, 4.5659, 5.8069, 6.5312],        # 1024 bytes
        [5.8627, 9.7800, 14.3599, 22.8114],      # 4096 bytes
        [19.1856, 32.6854, 50.7948, 77.8352],    # 16384 bytes
        [44.2117, 74.1762, 108.3720, 55.4221],   # 65536 bytes
    ],
}

# ---- Latency (µs) ----
latency = {
    'two_copy': [
        [1.98, 2.23, 2.81, 4.35],    # 1024
        [3.45, 3.68, 4.41, 7.10],    # 4096
        [4.57, 5.49, 6.85, 11.36],   # 16384
        [8.40, 9.59, 13.65, 58.05],  # 65536
    ],
    'one_copy': [
        [2.30, 2.63, 3.40, 4.94],    # 1024
        [3.98, 4.61, 5.76, 8.10],    # 4096
        [4.62, 5.69, 7.35, 10.19],   # 16384
        [8.58, 10.47, 15.33, 59.80], # 65536
    ],
    'zero_copy': [
        [3.11, 3.54, 5.59, 9.98],     # 1024
        [5.55, 6.66, 9.07, 11.54],    # 4096
        [6.79, 7.97, 10.27, 13.41],   # 16384
        [11.82, 14.09, 19.30, 75.64], # 65536
    ],
}

# ---- CPU Cycles ----
cpu_cycles = {
    'two_copy': [
        [28409387214, 53656869628, 98527099274, 109793973569],
        [30325097820, 56040832104, 101208068905, 108870743100],
        [28859351339, 51900679772, 98551822991, 156666506085],
        [23476236000, 45003458263, 96926798753, 196179908915],
    ],
    'one_copy': [
        [26197036869, 48722854030, 89230041679, 98416276944],
        [27534989237, 51645900273, 99784457869, 102657945724],
        [28329702835, 48985099619, 93626222496, 117789647445],
        [23015665302, 43351238096, 97692143076, 191305459679],
    ],
    'zero_copy': [
        [24757792086, 46163384665, 73148978327, 93038256848],
        [22013698281, 42354463995, 77471874237, 81640174916],
        [22337586104, 42363440867, 76954329003, 92296266222],
        [26334615178, 48849374772, 86457315997, 272740347885],
    ],
}

# ---- L1 Data Cache Misses ----
l1_misses = {
    'two_copy': [
        [577269830, 1029931837, 1711321895, 2082553008],
        [779338898, 1413689479, 2427089286, 2939790466],
        [1769295799, 2977526770, 4974295487, 5200660043],
        [3125631810, 5449930292, 7529531143, 3303499054],
    ],
    'one_copy': [
        [496639759, 954098969, 1628522965, 1907946837],
        [733215188, 1276592518, 2281813026, 2810026191],
        [1785901298, 2914253291, 4771729282, 6435956809],
        [3055557158, 5006103101, 6678734522, 3186368062],
    ],
    'zero_copy': [
        [530650268, 950464152, 1566456189, 2228646062],
        [640697434, 1144758217, 1963815186, 2431888693],
        [1401987820, 2418568878, 3826082786, 4947629784],
        [2369782107, 3980599653, 6073903248, 5777173257],
    ],
}

# ---- LLC (Last Level Cache) Misses ----
llc_misses = {
    'two_copy': [
        [53160, 98894, 2138200, 5956697],
        [1230150, 10448268, 44118371, 88090261],
        [909243, 10496775, 57809312, 476376937],
        [189089, 1543057, 149433240, 667555881],
    ],
    'one_copy': [
        [53414, 91160, 635681, 3725831],
        [117170, 1875381, 3324740, 46261691],
        [114923, 527981, 28288049, 220589378],
        [1269528, 1011599, 184252608, 664411683],
    ],
    'zero_copy': [
        [341260, 479383, 492183, 243996],
        [125821, 236086, 433265, 13874173],
        [569537, 3854646, 14246940, 154217685],
        [117846, 1052951, 91852926, 635737610],
    ],
}

# ---- Context Switches ----
context_switches = {
    'two_copy': [
        [1203950, 1941694, 2467585, 3929514],
        [926993, 1640263, 2087803, 3122686],
        [941442, 1508885, 1913540, 1513207],
        [1167046, 1886938, 1991288, 46606],
    ],
    'one_copy': [
        [1269609, 2148521, 2734496, 3398853],
        [1072824, 1687675, 1985056, 3419583],
        [1047254, 1644095, 1917276, 3169609],
        [1139720, 1703889, 1622218, 50031],
    ],
    'zero_copy': [
        [1196035, 1914164, 3259553, 4472138],
        [1583067, 2287885, 2781589, 3335481],
        [1345718, 1995664, 2500616, 2628984],
        [837758, 1233415, 1507846, 63354],
    ],
}

# ---- Total bytes transferred (for cycles/byte calc) ----
# Approximated as throughput_Gbps * duration(10s) / 8 * 1e9
DURATION = 10  # seconds

# ============================================================================
# Styling constants
# ============================================================================

COLORS = {'two_copy': '#e74c3c', 'one_copy': '#3498db', 'zero_copy': '#2ecc71'}
MARKERS = {'two_copy': 'o', 'one_copy': 's', 'zero_copy': '^'}
LABELS = {'two_copy': 'Two-Copy (send/recv)',
          'one_copy': 'One-Copy (sendmsg/iovec)',
          'zero_copy': 'Zero-Copy (MSG_ZEROCOPY)'}

SYSTEM_INFO = ("System: CachyOS Linux | Kernel 6.18.8 | "
               "11th Gen Intel | GCC 15.2.1")


def set_style():
    """Apply a clean plot style."""
    plt.rcParams.update({
        'figure.figsize': (10, 6),
        'axes.grid': True,
        'grid.alpha': 0.3,
        'font.size': 11,
        'axes.labelsize': 13,
        'axes.titlesize': 14,
        'legend.fontsize': 10,
    })


def save_plot(fig, name, output_dir):
    """Save a figure to the output directory."""
    path = os.path.join(output_dir, name)
    fig.savefig(path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    print(f"  Saved: {path}")


# ============================================================================
# Plot 1: Throughput vs Message Size
# ============================================================================
def plot_throughput_vs_msgsize(output_dir, thread_idx=2):
    """Throughput vs message size at a fixed thread count."""
    fig, ax = plt.subplots()
    thread_label = THREAD_COUNTS[thread_idx]

    for impl in ['two_copy', 'one_copy', 'zero_copy']:
        tp = [throughput[impl][mi][thread_idx] for mi in range(len(MSG_SIZES))]
        ax.plot(MSG_SIZES, tp,
                marker=MARKERS[impl], color=COLORS[impl],
                label=LABELS[impl], linewidth=2, markersize=8)

    ax.set_xlabel('Message Size (bytes)')
    ax.set_ylabel('Throughput (Gbps)')
    ax.set_title(f'Throughput vs Message Size ({thread_label} threads)')
    ax.set_xscale('log', base=2)
    ax.set_xticks(MSG_SIZES)
    ax.set_xticklabels([f'{s}' for s in MSG_SIZES])
    ax.legend()
    ax.annotate(SYSTEM_INFO, xy=(0.5, -0.12), xycoords='axes fraction',
                ha='center', fontsize=8, color='gray')

    save_plot(fig, 'MT25042_Part_D_Throughput_vs_MsgSize.png', output_dir)


# ============================================================================
# Plot 2: Latency vs Thread Count
# ============================================================================
def plot_latency_vs_threads(output_dir, msg_idx=2):
    """Latency vs thread count at a fixed message size."""
    fig, ax = plt.subplots()
    msg_label = MSG_SIZES[msg_idx]

    for impl in ['two_copy', 'one_copy', 'zero_copy']:
        lat = latency[impl][msg_idx]
        ax.plot(THREAD_COUNTS, lat,
                marker=MARKERS[impl], color=COLORS[impl],
                label=LABELS[impl], linewidth=2, markersize=8)

    ax.set_xlabel('Thread Count')
    ax.set_ylabel('Average Latency (µs)')
    ax.set_title(f'Latency vs Thread Count (msg_size={msg_label} bytes)')
    ax.set_xticks(THREAD_COUNTS)
    ax.legend()
    ax.annotate(SYSTEM_INFO, xy=(0.5, -0.12), xycoords='axes fraction',
                ha='center', fontsize=8, color='gray')

    save_plot(fig, 'MT25042_Part_D_Latency_vs_Threads.png', output_dir)


# ============================================================================
# Plot 3: Cache Misses vs Message Size
# ============================================================================
def plot_cache_misses_vs_msgsize(output_dir, thread_idx=2):
    """L1 and LLC cache misses vs message size (grouped bar + line)."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    thread_label = THREAD_COUNTS[thread_idx]

    # --- L1 misses ---
    for impl in ['two_copy', 'one_copy', 'zero_copy']:
        vals = [l1_misses[impl][mi][thread_idx] / 1e6
                for mi in range(len(MSG_SIZES))]
        ax1.plot(MSG_SIZES, vals,
                 marker=MARKERS[impl], color=COLORS[impl],
                 label=LABELS[impl], linewidth=2, markersize=8)

    ax1.set_xlabel('Message Size (bytes)')
    ax1.set_ylabel('L1 Data Cache Misses (millions)')
    ax1.set_title(f'L1 Cache Misses ({thread_label} threads)')
    ax1.set_xscale('log', base=2)
    ax1.set_xticks(MSG_SIZES)
    ax1.set_xticklabels([f'{s}' for s in MSG_SIZES])
    ax1.legend()

    # --- LLC misses ---
    for impl in ['two_copy', 'one_copy', 'zero_copy']:
        vals = [llc_misses[impl][mi][thread_idx] / 1e6
                for mi in range(len(MSG_SIZES))]
        ax2.plot(MSG_SIZES, vals,
                 marker=MARKERS[impl], color=COLORS[impl],
                 label=LABELS[impl], linewidth=2, markersize=8)

    ax2.set_xlabel('Message Size (bytes)')
    ax2.set_ylabel('LLC Misses (millions)')
    ax2.set_title(f'LLC (Last-Level Cache) Misses ({thread_label} threads)')
    ax2.set_xscale('log', base=2)
    ax2.set_xticks(MSG_SIZES)
    ax2.set_xticklabels([f'{s}' for s in MSG_SIZES])
    ax2.legend()

    fig.suptitle(f'Cache Misses vs Message Size', fontsize=15, fontweight='bold')
    fig.text(0.5, -0.02, SYSTEM_INFO, ha='center', fontsize=8, color='gray')
    plt.tight_layout()

    save_plot(fig, 'MT25042_Part_D_CacheMisses_vs_MsgSize.png', output_dir)


# ============================================================================
# Plot 4: CPU Cycles per Byte Transferred
# ============================================================================
def plot_cycles_per_byte(output_dir, thread_idx=2):
    """CPU cycles per byte transferred vs message size."""
    fig, ax = plt.subplots()
    thread_label = THREAD_COUNTS[thread_idx]

    for impl in ['two_copy', 'one_copy', 'zero_copy']:
        cpb = []
        for mi in range(len(MSG_SIZES)):
            tp_gbps = throughput[impl][mi][thread_idx]
            total_bytes = tp_gbps * 1e9 / 8 * DURATION  # bytes transferred
            cycles = cpu_cycles[impl][mi][thread_idx]
            cpb.append(cycles / total_bytes if total_bytes > 0 else 0)

        ax.plot(MSG_SIZES, cpb,
                marker=MARKERS[impl], color=COLORS[impl],
                label=LABELS[impl], linewidth=2, markersize=8)

    ax.set_xlabel('Message Size (bytes)')
    ax.set_ylabel('CPU Cycles per Byte')
    ax.set_title(f'CPU Cycles per Byte vs Message Size ({thread_label} threads)')
    ax.set_xscale('log', base=2)
    ax.set_xticks(MSG_SIZES)
    ax.set_xticklabels([f'{s}' for s in MSG_SIZES])
    ax.legend()
    ax.annotate(SYSTEM_INFO, xy=(0.5, -0.12), xycoords='axes fraction',
                ha='center', fontsize=8, color='gray')

    save_plot(fig, 'MT25042_Part_D_CyclesPerByte_vs_MsgSize.png', output_dir)


# ============================================================================
# Main
# ============================================================================
def main():
    output_dir = os.path.dirname(os.path.abspath(__file__))
    if len(sys.argv) >= 2:
        output_dir = sys.argv[1]
    os.makedirs(output_dir, exist_ok=True)

    set_style()

    print("Generating plots...")
    plot_throughput_vs_msgsize(output_dir)
    plot_latency_vs_threads(output_dir)
    plot_cache_misses_vs_msgsize(output_dir)
    plot_cycles_per_byte(output_dir)
    print("All plots generated successfully.")


if __name__ == "__main__":
    main()
