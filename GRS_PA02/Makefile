# MT25042
# Makefile for Graduate Systems (CSE638) - PA02: Network I/O Primitives
#
# Usage:
#   make          - Build all server/client binaries
#   make clean    - Remove compiled files
#   make help     - Show available targets
#
# AI Declaration: Makefile structure adapted from PA01 with new targets.

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -g
LDFLAGS  = -lpthread -lm
ROLL_NUM = MT25042
COMMON   = $(ROLL_NUM)_Part_A_Common.h

#------------------------------------------------------------------------------
# Source → Binary mapping
#------------------------------------------------------------------------------
A1_SERVER_SRC = $(ROLL_NUM)_Part_A1_Server.c
A1_CLIENT_SRC = $(ROLL_NUM)_Part_A1_Client.c
A2_SERVER_SRC = $(ROLL_NUM)_Part_A2_Server.c
A2_CLIENT_SRC = $(ROLL_NUM)_Part_A2_Client.c
A3_SERVER_SRC = $(ROLL_NUM)_Part_A3_Server.c
A3_CLIENT_SRC = $(ROLL_NUM)_Part_A3_Client.c

A1_SERVER = a1_server
A1_CLIENT = a1_client
A2_SERVER = a2_server
A2_CLIENT = a2_client
A3_SERVER = a3_server
A3_CLIENT = a3_client

ALL_BINS = $(A1_SERVER) $(A1_CLIENT) $(A2_SERVER) $(A2_CLIENT) \
           $(A3_SERVER) $(A3_CLIENT)

#------------------------------------------------------------------------------
# Targets
#------------------------------------------------------------------------------
.PHONY: all clean help

all: $(ALL_BINS)
	@echo "Build complete: $(ALL_BINS)"

# --- A1: Two-copy (send/recv) ---
$(A1_SERVER): $(A1_SERVER_SRC) $(COMMON)
	@echo "Compiling A1 Server (two-copy)..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(A1_CLIENT): $(A1_CLIENT_SRC) $(COMMON)
	@echo "Compiling A1 Client (two-copy)..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# --- A2: One-copy (sendmsg/iovec) ---
$(A2_SERVER): $(A2_SERVER_SRC) $(COMMON)
	@echo "Compiling A2 Server (one-copy)..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(A2_CLIENT): $(A2_CLIENT_SRC) $(COMMON)
	@echo "Compiling A2 Client (one-copy)..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# --- A3: Zero-copy (MSG_ZEROCOPY) ---
$(A3_SERVER): $(A3_SERVER_SRC) $(COMMON)
	@echo "Compiling A3 Server (zero-copy)..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(A3_CLIENT): $(A3_CLIENT_SRC) $(COMMON)
	@echo "Compiling A3 Client (zero-copy)..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# --- Housekeeping ---
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(ALL_BINS)
	@echo "Clean complete."

help:
	@echo "=========================================="
	@echo "PA02: Network I/O Primitives - Makefile"
	@echo "=========================================="
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build all 6 binaries"
	@echo "  make clean    - Remove compiled executables"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Binaries produced:"
	@echo "  a1_server / a1_client  - Two-copy (send/recv)"
	@echo "  a2_server / a2_client  - One-copy (sendmsg/iovec)"
	@echo "  a3_server / a3_client  - Zero-copy (MSG_ZEROCOPY)"
