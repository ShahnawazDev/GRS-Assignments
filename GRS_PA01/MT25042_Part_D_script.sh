#!/bin/bash
###############################################################################
# MT25042_Part_D_script.sh
# Graduate Systems (CSE638) - PA01: Processes and Threads
#
# Extends Part C to vary process/thread counts and generate plots.
#
# Usage: ./MT25042_Part_D_script.sh [cpu_list]
#
# AI Declaration: This script structure was generated with AI assistance.
###############################################################################

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
ROLL_NUM="MT25042"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_CSV="${SCRIPT_DIR}/${ROLL_NUM}_Part_D_CSV.csv"
PLOT_OUTPUT_DIR="${SCRIPT_DIR}"
PROGRAM_A="${SCRIPT_DIR}/program_a"
PROGRAM_B="${SCRIPT_DIR}/program_b"
CPU_LIST="${1:-0,1,2,3}"

PROCESS_COUNTS=(2 3 4 5)
THREAD_COUNTS=(2 3 4 5 6 7 8)
WORKER_TYPES=("cpu" "mem" "io")

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

#------------------------------------------------------------------------------
# Functions
#------------------------------------------------------------------------------

print_msg() {
    echo -e "${1}${2}${NC}"
}

check_dependencies() {
    print_msg "$BLUE" "Checking dependencies..."
    
    for dep in taskset awk grep bc; do
        if ! command -v "$dep" &> /dev/null; then
            print_msg "$RED" "Error: '$dep' not found."
            exit 1
        fi
    done
    
    if [[ ! -x /usr/bin/time ]]; then
        print_msg "$RED" "Error: /usr/bin/time not found. Install: sudo apt install time"
        exit 1
    fi
    
    if ! command -v python3 &> /dev/null; then
        print_msg "$YELLOW" "Warning: python3 not found. Plots will not be generated."
    fi
    
    if [[ ! -x "$PROGRAM_A" ]] || [[ ! -x "$PROGRAM_B" ]]; then
        print_msg "$RED" "Error: Programs not found. Run 'make' first."
        exit 1
    fi
    
    print_msg "$GREEN" "Dependencies check complete."
}

init_csv() {
    echo "Program,Function,Count,CPU_Percent,Memory_KB,IO_Read_KB,IO_Write_KB,Exec_Time_Real,Exec_Time_User,Exec_Time_Sys" > "$OUTPUT_CSV"
    print_msg "$GREEN" "Initialized CSV: $OUTPUT_CSV"
}

parse_elapsed() {
    local elapsed="$1"
    elapsed=$(echo "$elapsed" | xargs)
    local colons=$(echo "$elapsed" | tr -cd ':' | wc -c)
    
    if [[ $colons -eq 2 ]]; then
        local h=$(echo "$elapsed" | cut -d: -f1)
        local m=$(echo "$elapsed" | cut -d: -f2)
        local s=$(echo "$elapsed" | cut -d: -f3)
        echo "$h * 3600 + $m * 60 + $s" | bc -l
    elif [[ $colons -eq 1 ]]; then
        local m=$(echo "$elapsed" | cut -d: -f1)
        local s=$(echo "$elapsed" | cut -d: -f2)
        echo "$m * 60 + $s" | bc -l
    else
        echo "$elapsed"
    fi
}

run_and_measure() {
    local program=$1
    local worker_type=$2
    local count=$3
    local program_name program_path count_type
    
    if [[ "$program" == "A" ]]; then
        program_name="Program_A"
        program_path="$PROGRAM_A"
        count_type="processes"
    else
        program_name="Program_B"
        program_path="$PROGRAM_B"
        count_type="threads"
    fi
    
    print_msg "$YELLOW" "Running: ${program_name} + ${worker_type} (${count} ${count_type})"
    
    local tmp_dir=$(mktemp -d)
    local time_output="${tmp_dir}/time.txt"
    
    # Run with /usr/bin/time -v
    /usr/bin/time -v taskset -c "$CPU_LIST" "$program_path" "$worker_type" "$count" \
        > /dev/null 2> "$time_output"
    
    # Parse time output
    local user_time=$(grep "User time" "$time_output" | awk -F': ' '{print $2}')
    local sys_time=$(grep "System time" "$time_output" | awk -F': ' '{print $2}')
    local cpu_percent=$(grep "Percent of CPU" "$time_output" | awk -F': ' '{print $2}' | tr -d '%')
    local elapsed=$(grep "Elapsed" "$time_output" | awk -F'): ' '{print $2}')
    local max_rss=$(grep "Maximum resident" "$time_output" | awk -F': ' '{print $2}')
    local fs_inputs=$(grep "File system inputs" "$time_output" | awk -F': ' '{print $2}')
    local fs_outputs=$(grep "File system outputs" "$time_output" | awk -F': ' '{print $2}')
    
    local real_time=$(parse_elapsed "$elapsed")
    local io_read_kb=$((${fs_inputs:-0} / 2))
    local io_write_kb=$((${fs_outputs:-0} / 2))
    
    # Handle empty values
    [[ -z "$cpu_percent" ]] && cpu_percent="0"
    [[ -z "$max_rss" ]] && max_rss="0"
    [[ -z "$real_time" ]] && real_time="0"
    [[ -z "$user_time" ]] && user_time="0"
    [[ -z "$sys_time" ]] && sys_time="0"
    
    echo "  CPU: ${cpu_percent}%, Mem: ${max_rss}KB, IO_W: ${io_write_kb}KB, Time: ${real_time}s"
    
    echo "${program_name},${worker_type},${count},${cpu_percent},${max_rss},${io_read_kb},${io_write_kb},${real_time},${user_time},${sys_time}" >> "$OUTPUT_CSV"
    
    rm -rf "$tmp_dir"
}

generate_plots() {
    if ! command -v python3 &> /dev/null; then
        print_msg "$YELLOW" "Skipping plots (python3 not available)"
        return
    fi
    
    print_msg "$BLUE" "Generating plots..."
    
    if [[ -f "${SCRIPT_DIR}/MT25042_Part_D_plot.py" ]]; then
        python3 "${SCRIPT_DIR}/MT25042_Part_D_plot.py" "$OUTPUT_CSV" "$PLOT_OUTPUT_DIR" "$ROLL_NUM" 2>&1 || \
            print_msg "$YELLOW" "Plot generation failed. Install matplotlib: pip3 install matplotlib"
    else
        print_msg "$YELLOW" "Plot script not found"
    fi
}

print_summary() {
    print_msg "$GREEN" "=========================================="
    print_msg "$GREEN" "Part D Summary"
    print_msg "$GREEN" "=========================================="
    
    echo ""
    echo "Program A (Processes) Results:"
    printf "%-8s %-8s %-10s %-12s %-12s %-12s\n" "Worker" "Count" "CPU%" "Mem(KB)" "IO_W(KB)" "Time(s)"
    printf "%s\n" "------------------------------------------------------------------------"
    grep "Program_A" "$OUTPUT_CSV" | while IFS=',' read -r prog func count cpu mem io_r io_w time_r time_u time_s; do
        printf "%-8s %-8s %-10s %-12s %-12s %-12s\n" "$func" "$count" "$cpu" "$mem" "$io_w" "$time_r"
    done
    
    echo ""
    echo "Program B (Threads) Results:"
    printf "%-8s %-8s %-10s %-12s %-12s %-12s\n" "Worker" "Count" "CPU%" "Mem(KB)" "IO_W(KB)" "Time(s)"
    printf "%s\n" "------------------------------------------------------------------------"
    grep "Program_B" "$OUTPUT_CSV" | while IFS=',' read -r prog func count cpu mem io_r io_w time_r time_u time_s; do
        printf "%-8s %-8s %-10s %-12s %-12s %-12s\n" "$func" "$count" "$cpu" "$mem" "$io_w" "$time_r"
    done
    
    echo ""
    print_msg "$GREEN" "Raw data saved to: $OUTPUT_CSV"
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------

main() {
    print_msg "$GREEN" "=========================================="
    print_msg "$GREEN" "PA01 Part D: Scalability Analysis Script"
    print_msg "$GREEN" "Roll Number: $ROLL_NUM"
    print_msg "$GREEN" "=========================================="
    echo ""
    
    check_dependencies
    init_csv
    
    print_msg "$BLUE" "Testing Program A (fork) with varying process counts..."
    for worker in "${WORKER_TYPES[@]}"; do
        for count in "${PROCESS_COUNTS[@]}"; do
            run_and_measure "A" "$worker" "$count"
        done
    done
    
    print_msg "$BLUE" "Testing Program B (pthread) with varying thread counts..."
    for worker in "${WORKER_TYPES[@]}"; do
        for count in "${THREAD_COUNTS[@]}"; do
            run_and_measure "B" "$worker" "$count"
        done
    done
    
    print_summary
    generate_plots
    
    print_msg "$GREEN" "=========================================="
    print_msg "$GREEN" "Part D Execution Complete!"
    print_msg "$GREEN" "=========================================="
}

main "$@"
