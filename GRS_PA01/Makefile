# Makefile for Graduate Systems (CSE638) - PA01: Processes and Threads
# Roll Number: MT25042 (TODO: Update with your roll number)
#
# Usage:
#   make          - Build all programs
#   make clean    - Remove compiled files
#   make run_c    - Run Part C measurements
#   make run_d    - Run Part D measurements
#   make all      - Build and run everything
#
# AI Declaration: This Makefile was generated with AI assistance.

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -lpthread -lm

# Roll number prefix (TODO: Update this!)
ROLL_NUM = MT25042

# Source files
PROGRAM_A_SRC = $(ROLL_NUM)_Part_A_Program_A.c
PROGRAM_B_SRC = $(ROLL_NUM)_Part_A_Program_B.c
WORKERS_SRC = $(ROLL_NUM)_Part_B_workers.c
WORKERS_HDR = $(ROLL_NUM)_Part_B_workers.h

# Output executables
PROGRAM_A = program_a
PROGRAM_B = program_b

# Scripts
SCRIPT_C = $(ROLL_NUM)_Part_C_script.sh
SCRIPT_D = $(ROLL_NUM)_Part_D_script.sh
PLOT_SCRIPT = $(ROLL_NUM)_Part_D_plot.py

#------------------------------------------------------------------------------
# Targets
#------------------------------------------------------------------------------

.PHONY: all clean run_c run_d help

# Default target: build all programs
all: $(PROGRAM_A) $(PROGRAM_B)
	@echo "Build complete!"
	@echo "Executables: $(PROGRAM_A), $(PROGRAM_B)"

# Program A (fork-based)
$(PROGRAM_A): $(PROGRAM_A_SRC) $(WORKERS_SRC) $(WORKERS_HDR)
	@echo "Compiling Program A (fork)..."
	$(CC) $(CFLAGS) -o $@ $(PROGRAM_A_SRC) $(WORKERS_SRC) $(LDFLAGS)
	@echo "Built: $@"

# Program B (pthread-based)
$(PROGRAM_B): $(PROGRAM_B_SRC) $(WORKERS_SRC) $(WORKERS_HDR)
	@echo "Compiling Program B (pthread)..."
	$(CC) $(CFLAGS) -o $@ $(PROGRAM_B_SRC) $(WORKERS_SRC) $(LDFLAGS)
	@echo "Built: $@"

# Run Part C measurements
run_c: $(PROGRAM_A) $(PROGRAM_B)
	@echo "Running Part C measurements..."
	@chmod +x $(SCRIPT_C)
	./$(SCRIPT_C)

# Run Part D measurements
run_d: $(PROGRAM_A) $(PROGRAM_B)
	@echo "Running Part D measurements..."
	@chmod +x $(SCRIPT_D)
	./$(SCRIPT_D)

# Generate plots only (requires CSV data)
plots:
	@echo "Generating plots..."
	@chmod +x $(PLOT_SCRIPT)
	python3 $(PLOT_SCRIPT) $(ROLL_NUM)_Part_D_CSV.csv . $(ROLL_NUM)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(PROGRAM_A) $(PROGRAM_B)
	rm -f /tmp/pa01_io_worker_*.tmp
	@echo "Clean complete."

# Deep clean (includes CSVs and plots)
distclean: clean
	@echo "Removing generated data files..."
	rm -f $(ROLL_NUM)_Part_C_CSV.csv
	rm -f $(ROLL_NUM)_Part_D_CSV.csv
	rm -f $(ROLL_NUM)_Part_D_*_Plot.png
	rm -f plot_*.gp
	@echo "Distclean complete."

# Help message
help:
	@echo "=========================================="
	@echo "PA01: Processes and Threads - Makefile"
	@echo "=========================================="
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build all programs"
	@echo "  make clean    - Remove compiled executables"
	@echo "  make distclean- Remove all generated files"
	@echo "  make run_c    - Run Part C measurements"
	@echo "  make run_d    - Run Part D measurements"
	@echo "  make plots    - Generate plots from CSV data"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Before running, update ROLL_NUM in this Makefile"
	@echo "and in the source files!"
