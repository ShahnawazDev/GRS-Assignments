#!/usr/bin/env python3
"""
MT25042_Part_D_plot.py
Graduate Systems (CSE638) - PA01: Processes and Threads

This script generates plots for Part D analysis:
- CPU Usage vs Process/Thread Count
- Execution Time vs Process/Thread Count
- Memory Usage vs Process/Thread Count
- I/O Statistics vs Process/Thread Count

Usage: python3 MT25042_Part_D_plot.py <csv_file> <output_dir> <roll_num>

AI Declaration: This script was generated with AI assistance.
"""

import sys
import os
import csv

def check_matplotlib():
    """Check if matplotlib is available, provide installation instructions if not."""
    try:
        import matplotlib
        matplotlib.use('Agg')  # Use non-interactive backend
        import matplotlib.pyplot as plt
        return True
    except ImportError:
        print("Error: matplotlib is not installed.")
        print("Install with: pip3 install matplotlib")
        return False

def load_csv_data(csv_file):
    """Load and parse the CSV data file."""
    data = {
        'Program_A': {'cpu': {}, 'mem': {}, 'io': {}},
        'Program_B': {'cpu': {}, 'mem': {}, 'io': {}}
    }
    
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            program = row['Program']
            function = row['Function']
            count = int(row['Count'])
            
            if program not in data:
                continue
            if function not in data[program]:
                continue
                
            data[program][function][count] = {
                'cpu_percent': float(row['CPU_Percent']),
                'memory_kb': float(row['Memory_KB']),
                'io_read': float(row.get('IO_Read_KB', row.get('IO_Read_KBps', 0))),
                'io_write': float(row.get('IO_Write_KB', row.get('IO_Write_KBps', 0))),
                'time_real': float(row['Exec_Time_Real']),
                'time_user': float(row['Exec_Time_User']),
                'time_sys': float(row['Exec_Time_Sys'])
            }
    
    return data

def create_plots(data, output_dir, roll_num):
    """Generate all plots."""
    import matplotlib.pyplot as plt
    
    # Color scheme
    colors = {
        'cpu': '#e74c3c',   # Red
        'mem': '#3498db',   # Blue
        'io': '#2ecc71'     # Green
    }
    
    markers = {
        'Program_A': 'o',   # Circle for processes
        'Program_B': 's'    # Square for threads
    }
    
    linestyles = {
        'Program_A': '-',   # Solid for processes
        'Program_B': '--'   # Dashed for threads
    }
    
    # Get all count values
    all_counts_a = set()
    all_counts_b = set()
    for func in ['cpu', 'mem', 'io']:
        all_counts_a.update(data['Program_A'][func].keys())
        all_counts_b.update(data['Program_B'][func].keys())
    
    counts_a = sorted(list(all_counts_a))
    counts_b = sorted(list(all_counts_b))
    
    # =========================================================================
    # Plot 1: CPU Usage vs Count
    # =========================================================================
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle(f'{roll_num}: CPU Usage Analysis', fontsize=14, fontweight='bold')
    
    # Program A (Processes)
    for func in ['cpu', 'mem', 'io']:
        counts = sorted(data['Program_A'][func].keys())
        values = [data['Program_A'][func][c]['cpu_percent'] for c in counts]
        ax1.plot(counts, values, marker='o', color=colors[func], 
                label=f'{func.upper()} worker', linewidth=2, markersize=8)
    
    ax1.set_xlabel('Number of Processes', fontsize=12)
    ax1.set_ylabel('CPU Usage (%)', fontsize=12)
    ax1.set_title('Program A (fork)', fontsize=12)
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    ax1.set_xticks(counts_a)
    
    # Program B (Threads)
    for func in ['cpu', 'mem', 'io']:
        counts = sorted(data['Program_B'][func].keys())
        values = [data['Program_B'][func][c]['cpu_percent'] for c in counts]
        ax2.plot(counts, values, marker='s', color=colors[func],
                label=f'{func.upper()} worker', linewidth=2, markersize=8)
    
    ax2.set_xlabel('Number of Threads', fontsize=12)
    ax2.set_ylabel('CPU Usage (%)', fontsize=12)
    ax2.set_title('Program B (pthread)', fontsize=12)
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    ax2.set_xticks(counts_b)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, f'{roll_num}_Part_D_CPU_Plot.png'), dpi=150)
    plt.close()
    
    # =========================================================================
    # Plot 2: Execution Time vs Count
    # =========================================================================
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle(f'{roll_num}: Execution Time Analysis', fontsize=14, fontweight='bold')
    
    # Program A
    for func in ['cpu', 'mem', 'io']:
        counts = sorted(data['Program_A'][func].keys())
        values = [data['Program_A'][func][c]['time_real'] for c in counts]
        ax1.plot(counts, values, marker='o', color=colors[func],
                label=f'{func.upper()} worker', linewidth=2, markersize=8)
    
    ax1.set_xlabel('Number of Processes', fontsize=12)
    ax1.set_ylabel('Execution Time (seconds)', fontsize=12)
    ax1.set_title('Program A (fork)', fontsize=12)
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    ax1.set_xticks(counts_a)
    
    # Program B
    for func in ['cpu', 'mem', 'io']:
        counts = sorted(data['Program_B'][func].keys())
        values = [data['Program_B'][func][c]['time_real'] for c in counts]
        ax2.plot(counts, values, marker='s', color=colors[func],
                label=f'{func.upper()} worker', linewidth=2, markersize=8)
    
    ax2.set_xlabel('Number of Threads', fontsize=12)
    ax2.set_ylabel('Execution Time (seconds)', fontsize=12)
    ax2.set_title('Program B (pthread)', fontsize=12)
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    ax2.set_xticks(counts_b)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, f'{roll_num}_Part_D_Time_Plot.png'), dpi=150)
    plt.close()
    
    # =========================================================================
    # Plot 3: Memory Usage vs Count
    # =========================================================================
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle(f'{roll_num}: Memory Usage Analysis', fontsize=14, fontweight='bold')
    
    # Program A
    for func in ['cpu', 'mem', 'io']:
        counts = sorted(data['Program_A'][func].keys())
        values = [data['Program_A'][func][c]['memory_kb'] / 1024 for c in counts]  # Convert to MB
        ax1.plot(counts, values, marker='o', color=colors[func],
                label=f'{func.upper()} worker', linewidth=2, markersize=8)
    
    ax1.set_xlabel('Number of Processes', fontsize=12)
    ax1.set_ylabel('Memory Usage (MB)', fontsize=12)
    ax1.set_title('Program A (fork)', fontsize=12)
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    ax1.set_xticks(counts_a)
    
    # Program B
    for func in ['cpu', 'mem', 'io']:
        counts = sorted(data['Program_B'][func].keys())
        values = [data['Program_B'][func][c]['memory_kb'] / 1024 for c in counts]
        ax2.plot(counts, values, marker='s', color=colors[func],
                label=f'{func.upper()} worker', linewidth=2, markersize=8)
    
    ax2.set_xlabel('Number of Threads', fontsize=12)
    ax2.set_ylabel('Memory Usage (MB)', fontsize=12)
    ax2.set_title('Program B (pthread)', fontsize=12)
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    ax2.set_xticks(counts_b)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, f'{roll_num}_Part_D_Memory_Plot.png'), dpi=150)
    plt.close()
    
    # =========================================================================
    # Plot 4: Combined Comparison (All metrics, comparing A vs B)
    # =========================================================================
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f'{roll_num}: Process vs Thread Comparison', fontsize=14, fontweight='bold')
    
    # CPU comparison for each worker type
    ax = axes[0, 0]
    for func in ['cpu', 'mem', 'io']:
        counts_a_list = sorted(data['Program_A'][func].keys())
        values_a = [data['Program_A'][func][c]['cpu_percent'] for c in counts_a_list]
        counts_b_list = sorted(data['Program_B'][func].keys())
        values_b = [data['Program_B'][func][c]['cpu_percent'] for c in counts_b_list]
        
        ax.plot(counts_a_list, values_a, marker='o', linestyle='-', color=colors[func],
                label=f'Process - {func}', alpha=0.8)
        ax.plot(counts_b_list, values_b, marker='s', linestyle='--', color=colors[func],
                label=f'Thread - {func}', alpha=0.8)
    
    ax.set_xlabel('Count')
    ax.set_ylabel('CPU Usage (%)')
    ax.set_title('CPU Usage Comparison')
    ax.legend(loc='best', fontsize=8)
    ax.grid(True, alpha=0.3)
    
    # Time comparison
    ax = axes[0, 1]
    for func in ['cpu', 'mem', 'io']:
        counts_a_list = sorted(data['Program_A'][func].keys())
        values_a = [data['Program_A'][func][c]['time_real'] for c in counts_a_list]
        counts_b_list = sorted(data['Program_B'][func].keys())
        values_b = [data['Program_B'][func][c]['time_real'] for c in counts_b_list]
        
        ax.plot(counts_a_list, values_a, marker='o', linestyle='-', color=colors[func],
                label=f'Process - {func}', alpha=0.8)
        ax.plot(counts_b_list, values_b, marker='s', linestyle='--', color=colors[func],
                label=f'Thread - {func}', alpha=0.8)
    
    ax.set_xlabel('Count')
    ax.set_ylabel('Time (seconds)')
    ax.set_title('Execution Time Comparison')
    ax.legend(loc='best', fontsize=8)
    ax.grid(True, alpha=0.3)
    
    # Memory comparison
    ax = axes[1, 0]
    for func in ['cpu', 'mem', 'io']:
        counts_a_list = sorted(data['Program_A'][func].keys())
        values_a = [data['Program_A'][func][c]['memory_kb']/1024 for c in counts_a_list]
        counts_b_list = sorted(data['Program_B'][func].keys())
        values_b = [data['Program_B'][func][c]['memory_kb']/1024 for c in counts_b_list]
        
        ax.plot(counts_a_list, values_a, marker='o', linestyle='-', color=colors[func],
                label=f'Process - {func}', alpha=0.8)
        ax.plot(counts_b_list, values_b, marker='s', linestyle='--', color=colors[func],
                label=f'Thread - {func}', alpha=0.8)
    
    ax.set_xlabel('Count')
    ax.set_ylabel('Memory (MB)')
    ax.set_title('Memory Usage Comparison')
    ax.legend(loc='best', fontsize=8)
    ax.grid(True, alpha=0.3)
    
    # I/O Write comparison
    ax = axes[1, 1]
    for func in ['cpu', 'mem', 'io']:
        counts_a_list = sorted(data['Program_A'][func].keys())
        values_a = [data['Program_A'][func][c]['io_write'] for c in counts_a_list]
        counts_b_list = sorted(data['Program_B'][func].keys())
        values_b = [data['Program_B'][func][c]['io_write'] for c in counts_b_list]
        
        ax.plot(counts_a_list, values_a, marker='o', linestyle='-', color=colors[func],
                label=f'Process - {func}', alpha=0.8)
        ax.plot(counts_b_list, values_b, marker='s', linestyle='--', color=colors[func],
                label=f'Thread - {func}', alpha=0.8)
    
    ax.set_xlabel('Count')
    ax.set_ylabel('I/O Write (KB/s)')
    ax.set_title('I/O Write Comparison')
    ax.legend(loc='best', fontsize=8)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, f'{roll_num}_Part_D_Combined_Plot.png'), dpi=150)
    plt.close()
    
    print(f"Plots saved to {output_dir}")

def main():
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <csv_file> <output_dir> <roll_num>")
        sys.exit(1)
    
    csv_file = sys.argv[1]
    output_dir = sys.argv[2]
    roll_num = sys.argv[3]
    
    if not os.path.exists(csv_file):
        print(f"Error: CSV file not found: {csv_file}")
        sys.exit(1)
    
    if not check_matplotlib():
        sys.exit(1)
    
    print(f"Loading data from {csv_file}...")
    data = load_csv_data(csv_file)
    
    print("Generating plots...")
    create_plots(data, output_dir, roll_num)
    
    print("Done!")

if __name__ == "__main__":
    main()
