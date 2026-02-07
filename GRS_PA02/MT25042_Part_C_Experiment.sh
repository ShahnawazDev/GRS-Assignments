#!/bin/bash
###############################################################################
# MT25042_Part_C_Experiment.sh
# Graduate Systems (CSE638) - PA02: Analysis of Network I/O primitives
#
# Automated experiment script that:
#   1. Compiles all implementations
#   2. Sets up Linux network namespaces (ns_server / ns_client)
#   3. Runs experiments across message sizes and thread counts
#   4. Collects perf stat metrics + application-level throughput/latency
#   5. Stores everything in CSV format
#
# MUST be run as root (sudo) because namespace creation requires it.
#
# Usage:  sudo ./MT25042_Part_C_Experiment.sh
#
# AI Declaration: Asked ChatGPT "How to create Linux network namespaces
#   connected with veth for testing TCP locally?" and adapted the setup.
#   Also asked "How to parse perf stat CSV output in bash?"
###############################################################################

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
ROLL_NUM="MT25042"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_CSV="${SCRIPT_DIR}/${ROLL_NUM}_Part_B_Results.csv"

# Namespace names
NS_SERVER="ns_server"
NS_CLIENT="ns_client"

# veth pair
VETH_S="veth_srv"
VETH_C="veth_cli"

# IP addresses
IP_SERVER="10.0.0.1"
IP_CLIENT="10.0.0.2"
SUBNET="/24"

# Experiment parameters
MSG_SIZES=(1024 4096 16384 65536)
THREAD_COUNTS=(1 2 4 8)
IMPLEMENTATIONS=("a1" "a2" "a3")
IMPL_NAMES=("two_copy" "one_copy" "zero_copy")

# Duration per experiment (seconds)
DURATION=10

# Server binaries
declare -A SERVER_BIN=( [a1]="a1_server" [a2]="a2_server" [a3]="a3_server" )
declare -A CLIENT_BIN=( [a1]="a1_client" [a2]="a2_client" [a3]="a3_client" )

# Colours for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PORT=9876

#------------------------------------------------------------------------------
# Utility functions
#------------------------------------------------------------------------------
msg() { echo -e "${1}${2}${NC}"; }

cleanup_namespaces() {
    msg "$BLUE" "Cleaning up namespaces..."
    ip netns pids "$NS_SERVER" 2>/dev/null | xargs -r kill 2>/dev/null || true
    ip netns pids "$NS_CLIENT" 2>/dev/null | xargs -r kill 2>/dev/null || true
    sleep 0.3
    ip netns del "$NS_SERVER" 2>/dev/null || true
    ip netns del "$NS_CLIENT" 2>/dev/null || true
    ip link del "$VETH_S" 2>/dev/null || true
}

setup_namespaces() {
    msg "$BLUE" "Setting up network namespaces..."

    # Clean up any leftovers
    cleanup_namespaces

    # Create namespaces
    ip netns add "$NS_SERVER"
    ip netns add "$NS_CLIENT"

    # Create veth pair
    ip link add "$VETH_S" type veth peer name "$VETH_C"

    # Move each end into its namespace
    ip link set "$VETH_S" netns "$NS_SERVER"
    ip link set "$VETH_C" netns "$NS_CLIENT"

    # Configure IPs and bring up interfaces
    ip netns exec "$NS_SERVER" ip addr add "${IP_SERVER}${SUBNET}" dev "$VETH_S"
    ip netns exec "$NS_SERVER" ip link set "$VETH_S" up
    ip netns exec "$NS_SERVER" ip link set lo up

    ip netns exec "$NS_CLIENT" ip addr add "${IP_CLIENT}${SUBNET}" dev "$VETH_C"
    ip netns exec "$NS_CLIENT" ip link set "$VETH_C" up
    ip netns exec "$NS_CLIENT" ip link set lo up

    # Increase socket buffer sizes for zero-copy tests
    ip netns exec "$NS_SERVER" sysctl -w net.core.wmem_max=16777216 >/dev/null 2>&1
    ip netns exec "$NS_SERVER" sysctl -w net.core.rmem_max=16777216 >/dev/null 2>&1
    ip netns exec "$NS_CLIENT" sysctl -w net.core.wmem_max=16777216 >/dev/null 2>&1
    ip netns exec "$NS_CLIENT" sysctl -w net.core.rmem_max=16777216 >/dev/null 2>&1

    # Verify connectivity
    if ip netns exec "$NS_CLIENT" ping -c 1 -W 2 "$IP_SERVER" >/dev/null 2>&1; then
        msg "$GREEN" "Namespace connectivity verified."
    else
        msg "$RED" "ERROR: Cannot ping server from client namespace!"
        exit 1
    fi
}

kill_server() {
    # Kill all processes in the server namespace
    ip netns pids "$NS_SERVER" 2>/dev/null | xargs -r kill 2>/dev/null || true
    sleep 0.5
}

wait_for_port() {
    local ns=$1
    local port=$2
    local retries=30
    while [ $retries -gt 0 ]; do
        if ip netns exec "$ns" bash -c "echo >/dev/tcp/${IP_SERVER}/${port}" 2>/dev/null; then
            return 0
        fi
        sleep 0.2
        retries=$((retries - 1))
    done
    msg "$RED" "ERROR: Server did not start listening on port $port"
    return 1
}

#------------------------------------------------------------------------------
# Parse perf stat output (expects --field-separator=, -x, format)
#------------------------------------------------------------------------------
parse_perf() {
    local perf_file=$1

    # perf stat -x, output format:  <value>,<unit>,<event>,<...>
    # We extract by event name
    local cycles=$(grep "cpu-cycles\|cycles" "$perf_file" | head -1 | cut -d',' -f1)
    local l1_miss=$(grep "L1-dcache-load-misses" "$perf_file" | head -1 | cut -d',' -f1)
    local llc_miss=$(grep "LLC-load-misses" "$perf_file" | head -1 | cut -d',' -f1)
    local ctx_sw=$(grep "context-switches\|cs" "$perf_file" | head -1 | cut -d',' -f1)

    # Clean up – remove thousand separators, handle <not counted>
    cycles=$(echo "$cycles" | tr -d ' ' | sed 's/<notcounted>/0/')
    l1_miss=$(echo "$l1_miss" | tr -d ' ' | sed 's/<notcounted>/0/')
    llc_miss=$(echo "$llc_miss" | tr -d ' ' | sed 's/<notcounted>/0/')
    ctx_sw=$(echo "$ctx_sw" | tr -d ' ' | sed 's/<notcounted>/0/')

    echo "${cycles:-0},${l1_miss:-0},${llc_miss:-0},${ctx_sw:-0}"
}

#------------------------------------------------------------------------------
# Run a single experiment
#------------------------------------------------------------------------------
run_experiment() {
    local impl=$1
    local impl_name=$2
    local msg_size=$3
    local threads=$4

    local server="${SCRIPT_DIR}/${SERVER_BIN[$impl]}"
    local client="${SCRIPT_DIR}/${CLIENT_BIN[$impl]}"

    msg "$YELLOW" "--- ${impl_name} | msg=${msg_size} | threads=${threads} ---"

    # Kill any leftover server
    kill_server

    # Start server in ns_server (background)
    ip netns exec "$NS_SERVER" "$server" "$msg_size" "$threads" &
    local server_pid=$!
    sleep 1

    # Verify server is running
    if ! kill -0 "$server_pid" 2>/dev/null; then
        msg "$RED" "Server failed to start!"
        echo "${impl_name},${msg_size},${threads},0,0,0,0,0,0" >> "$OUTPUT_CSV"
        return
    fi

    # Run client in ns_client, wrapped with perf stat
    local perf_out=$(mktemp /tmp/perf_XXXXXX.txt)
    local client_out=$(mktemp /tmp/client_XXXXXX.txt)

    ip netns exec "$NS_CLIENT" perf stat \
        -e cpu-cycles,L1-dcache-load-misses,LLC-load-misses,context-switches \
        -x, \
        -o "$perf_out" \
        "$client" "$IP_SERVER" "$msg_size" "$threads" "$DURATION" \
        > "$client_out" 2>&1

    # Parse application-level results from client output
    local result_line=$(grep "^RESULT," "$client_out" | tail -1)
    local tp_gbps=0
    local avg_lat=0

    if [ -n "$result_line" ]; then
        tp_gbps=$(echo "$result_line" | cut -d',' -f5)
        avg_lat=$(echo "$result_line" | cut -d',' -f6)
    fi

    # Parse perf metrics
    local perf_metrics
    perf_metrics=$(parse_perf "$perf_out")
    local cpu_cycles=$(echo "$perf_metrics" | cut -d',' -f1)
    local l1_misses=$(echo "$perf_metrics" | cut -d',' -f2)
    local llc_misses=$(echo "$perf_metrics" | cut -d',' -f3)
    local ctx_switches=$(echo "$perf_metrics" | cut -d',' -f4)

    msg "$GREEN" "  Throughput: ${tp_gbps} Gbps | Latency: ${avg_lat} µs"
    msg "$GREEN" "  Cycles: ${cpu_cycles} | L1miss: ${l1_misses} | LLCmiss: ${llc_misses} | CtxSw: ${ctx_switches}"

    # Append to CSV
    echo "${impl_name},${msg_size},${threads},${tp_gbps},${avg_lat},${cpu_cycles},${l1_misses},${llc_misses},${ctx_switches}" \
        >> "$OUTPUT_CSV"

    # Clean up server
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
    rm -f "$perf_out" "$client_out"

    # Brief pause between experiments
    sleep 1
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------
main() {
    msg "$GREEN" "========================================================="
    msg "$GREEN" "PA02: Automated Experiment Script"
    msg "$GREEN" "Roll Number: ${ROLL_NUM}"
    msg "$GREEN" "========================================================="
    echo ""

    # Must be root
    if [ "$(id -u)" -ne 0 ]; then
        msg "$RED" "ERROR: This script must be run as root (sudo)."
        exit 1
    fi

    # Step 1: Compile
    msg "$BLUE" "[Step 1] Compiling all implementations..."
    cd "$SCRIPT_DIR"
    make clean && make
    echo ""

    # Step 2: Set up namespaces
    msg "$BLUE" "[Step 2] Setting up network namespaces..."
    setup_namespaces
    echo ""

    # Step 3: Initialise CSV
    msg "$BLUE" "[Step 3] Initialising CSV output..."
    echo "implementation,msg_size,threads,throughput_gbps,latency_us,cpu_cycles,l1_cache_misses,llc_cache_misses,context_switches" \
        > "$OUTPUT_CSV"
    echo ""

    # Step 4: Run experiments
    msg "$BLUE" "[Step 4] Running experiments..."
    local total=$(( ${#IMPLEMENTATIONS[@]} * ${#MSG_SIZES[@]} * ${#THREAD_COUNTS[@]} ))
    local count=0

    for idx in "${!IMPLEMENTATIONS[@]}"; do
        local impl="${IMPLEMENTATIONS[$idx]}"
        local impl_name="${IMPL_NAMES[$idx]}"

        for msg_size in "${MSG_SIZES[@]}"; do
            for threads in "${THREAD_COUNTS[@]}"; do
                count=$((count + 1))
                msg "$BLUE" "=== Experiment ${count}/${total} ==="
                run_experiment "$impl" "$impl_name" "$msg_size" "$threads"
                echo ""
            done
        done
    done

    # Step 5: Clean up
    msg "$BLUE" "[Step 5] Cleaning up namespaces..."
    cleanup_namespaces

    msg "$GREEN" "========================================================="
    msg "$GREEN" "All experiments complete!"
    msg "$GREEN" "Results saved to: ${OUTPUT_CSV}"
    msg "$GREEN" "========================================================="

    # Print summary
    echo ""
    msg "$BLUE" "CSV contents:"
    column -t -s',' "$OUTPUT_CSV"
}

# Trap to ensure cleanup on exit
trap cleanup_namespaces EXIT

main "$@"
