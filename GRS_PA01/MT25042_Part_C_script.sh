#!/bin/bash
###############################################################################
# MT25042_Part_C_script.sh
# Graduate Systems (CSE638) - PA01: Processes and Threads
#
# Measurements collected using /usr/bin/time -v:
# - CPU%: from time output
# - Memory: Maximum resident set size
# - I/O: File system inputs/outputs
# - Execution time: Elapsed wall clock time
#
# Usage: ./MT25042_Part_C_script.sh [cpu_list]
#
# AI Declaration: This script structure was generated with AI assistance.
###############################################################################

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
ROLL_NUM="MT25042"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_CSV="${SCRIPT_DIR}/${ROLL_NUM}_Part_C_CSV.csv"
PROGRAM_A="${SCRIPT_DIR}/program_a"
PROGRAM_B="${SCRIPT_DIR}/program_b"
CPU_LIST="${1:-0,1}"

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
            print_msg "$RED" "Error: '$dep' not found. Please install it."
            exit 1
        fi
    done
    
    if [[ ! -x /usr/bin/time ]]; then
        print_msg "$RED" "Error: /usr/bin/time not found. Install with: sudo apt install time"
        exit 1
    fi
    
    if [[ ! -x "$PROGRAM_A" ]] || [[ ! -x "$PROGRAM_B" ]]; then
        print_msg "$RED" "Error: Programs not found. Run 'make' first."
        exit 1
    fi
    
    print_msg "$GREEN" "All dependencies satisfied."
}

init_csv() {
    echo "Program,Function,CPU_Percent,Memory_KB,IO_Read_KB,IO_Write_KB,Exec_Time_Real,Exec_Time_User,Exec_Time_Sys" > "$OUTPUT_CSV"
    print_msg "$GREEN" "Initialized CSV: $OUTPUT_CSV"
}

# Parse elapsed time from format "h:mm:ss" or "m:ss.xx" to seconds
parse_elapsed() {
    local elapsed="$1"
    local seconds=0
    
    # Remove leading/trailing whitespace
    elapsed=$(echo "$elapsed" | xargs)
    
    # Count colons to determine format
    local colons=$(echo "$elapsed" | tr -cd ':' | wc -c)
    
    if [[ $colons -eq 2 ]]; then
        # h:mm:ss format
        local h=$(echo "$elapsed" | cut -d: -f1)
        local m=$(echo "$elapsed" | cut -d: -f2)
        local s=$(echo "$elapsed" | cut -d: -f3)
        seconds=$(echo "$h * 3600 + $m * 60 + $s" | bc -l)
    elif [[ $colons -eq 1 ]]; then
        # m:ss.xx format
        local m=$(echo "$elapsed" | cut -d: -f1)
        local s=$(echo "$elapsed" | cut -d: -f2)
        seconds=$(echo "$m * 60 + $s" | bc -l)
    else
        seconds="$elapsed"
    fi
    
    echo "$seconds"
}

run_and_measure() {
    local program=$1
    local worker_type=$2
    local program_name program_path
    
    if [[ "$program" == "A" ]]; then
        program_name="Program_A"
        program_path="$PROGRAM_A"
    else
        program_name="Program_B"
        program_path="$PROGRAM_B"
    fi
    
    print_msg "$YELLOW" "=========================================="
    print_msg "$YELLOW" "Running: ${program_name} + ${worker_type}"
    print_msg "$YELLOW" "=========================================="
    
    local tmp_dir=$(mktemp -d)
    local time_output="${tmp_dir}/time.txt"
    local prog_output="${tmp_dir}/prog.txt"
    
    print_msg "$BLUE" "Starting with CPU affinity: $CPU_LIST"
    
    # Run with /usr/bin/time -v to get detailed stats
    /usr/bin/time -v taskset -c "$CPU_LIST" "$program_path" "$worker_type" \
        > "$prog_output" 2> "$time_output"
    
    # Parse the time output
    local user_time=$(grep "User time" "$time_output" | awk -F': ' '{print $2}')
    local sys_time=$(grep "System time" "$time_output" | awk -F': ' '{print $2}')
    local cpu_percent=$(grep "Percent of CPU" "$time_output" | awk -F': ' '{print $2}' | tr -d '%')
    local elapsed=$(grep "Elapsed" "$time_output" | awk -F'): ' '{print $2}')
    local max_rss=$(grep "Maximum resident" "$time_output" | awk -F': ' '{print $2}')
    local fs_inputs=$(grep "File system inputs" "$time_output" | awk -F': ' '{print $2}')
    local fs_outputs=$(grep "File system outputs" "$time_output" | awk -F': ' '{print $2}')
    
    # Convert elapsed to seconds
    local real_time=$(parse_elapsed "$elapsed")
    
    # File system inputs/outputs are in 512-byte blocks, convert to KB
    local io_read_kb=$((${fs_inputs:-0} / 2))
    local io_write_kb=$((${fs_outputs:-0} / 2))
    
    # Handle empty values
    [[ -z "$cpu_percent" ]] && cpu_percent="0"
    [[ -z "$max_rss" ]] && max_rss="0"
    [[ -z "$real_time" ]] && real_time="0"
    [[ -z "$user_time" ]] && user_time="0"
    [[ -z "$sys_time" ]] && sys_time="0"
    
    # Print results
    print_msg "$GREEN" "Results for ${program_name} + ${worker_type}:"
    echo "  CPU%:         ${cpu_percent}%"
    echo "  Memory:       ${max_rss} KB"
    echo "  I/O Read:     ${io_read_kb} KB"
    echo "  I/O Write:    ${io_write_kb} KB"
    echo "  Real Time:    ${real_time}s"
    echo "  User Time:    ${user_time}s"
    echo "  Sys Time:     ${sys_time}s"
    
    # Append to CSV
    echo "${program_name},${worker_type},${cpu_percent},${max_rss},${io_read_kb},${io_write_kb},${real_time},${user_time},${sys_time}" >> "$OUTPUT_CSV"
    
    # Show program output
    print_msg "$BLUE" "Program output:"
    cat "$prog_output" 2>/dev/null | head -20
    
    rm -rf "$tmp_dir"
    print_msg "$GREEN" "Completed: ${program_name} + ${worker_type}"
    echo ""
}

print_summary() {
    print_msg "$GREEN" "=========================================="
    print_msg "$GREEN" "Summary Table"
    print_msg "$GREEN" "=========================================="
    
    echo ""
    printf "%-20s %-10s %-12s %-12s %-12s %-12s\n" \
           "Program+Function" "CPU%" "Mem(KB)" "IO_Read(KB)" "IO_Write(KB)" "Time(s)"
    printf "%s\n" "------------------------------------------------------------------------------------"
    
    tail -n +2 "$OUTPUT_CSV" | while IFS=',' read -r prog func cpu mem io_r io_w time_r time_u time_s; do
        printf "%-20s %-10s %-12s %-12s %-12s %-12s\n" \
               "${prog}+${func}" "$cpu" "$mem" "$io_r" "$io_w" "$time_r"
    done
    
    echo ""
    print_msg "$GREEN" "Raw data saved to: $OUTPUT_CSV"
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------

main() {
    print_msg "$GREEN" "=========================================="
    print_msg "$GREEN" "PA01 Part C: Automated Measurement Script"
    print_msg "$GREEN" "Roll Number: $ROLL_NUM"
    print_msg "$GREEN" "=========================================="
    echo ""
    
    check_dependencies
    init_csv
    
    for worker in "${WORKER_TYPES[@]}"; do
        run_and_measure "A" "$worker"
        sleep 1
        run_and_measure "B" "$worker"
        sleep 1
    done
    
    print_summary
    
    print_msg "$GREEN" "=========================================="
    print_msg "$GREEN" "Part C Execution Complete!"
    print_msg "$GREEN" "=========================================="
}

main "$@"
