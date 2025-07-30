# Makefile for the High-Performance Order Book Project

# Compiler and Flags
# Using the C23 standard to leverage modern features like nullptr, [[nodiscard]], etc.
# Enabling optimizations (-O2) with debug information (-g).
# -Wall and -Wextra are used for strict warnings.
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -std=c23 -Iinclude

# --- Server Configuration ---
SERVER_SRCS = $(shell find src -name '*.c')
SERVER_OBJS = $(patsubst src/%.c,build/%.o,$(SERVER_SRCS))
SERVER_TARGET = order_book_server

# --- Client Configuration ---
CLIENT_SRC = tests/client.c
CLIENT_TARGET = test_client

# Default target
all: $(SERVER_TARGET)

# --- Build Rules ---

# Linking the final server executable
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

# Compiling server source files into object files
build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# Linking the client executable
client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $<

# Phony targets
.PHONY: all clean client

# Clean up build artifacts
clean:
	rm -rf build $(SERVER_TARGET) $(CLIENT_TARGET)

