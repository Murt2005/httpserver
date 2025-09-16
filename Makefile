# Makefile for HTTP Server Project
# Compatible with macOS (kqueue) and Linux (epoll)

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
DEBUG_FLAGS = -std=c++17 -Wall -Wextra -g -pthread -DDEBUG

# Directories
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
HEADERS = $(wildcard $(SRC_DIR)/*.h)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Test files
TEST_SOURCES = $(TEST_DIR)/test.cpp
TEST_OBJECTS = $(BUILD_DIR)/test.o

# Target executables
SERVER_TARGET = $(BIN_DIR)/httpserver
TEST_TARGET = $(BIN_DIR)/test

# Default target
all: $(SERVER_TARGET) $(TEST_TARGET)

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build the HTTP server
$(SERVER_TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)
	@echo "HTTP Server built successfully: $@"

# Build individual object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build the test suite
$(TEST_TARGET): $(TEST_OBJECTS) $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS)) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Test suite built successfully: $@"

# Build test object file
$(BUILD_DIR)/test.o: $(TEST_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

# Debug build
debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: clean $(SERVER_TARGET) $(TEST_TARGET)
	@echo "Debug build completed"

# Run the HTTP server
run: $(SERVER_TARGET)
	@echo "Starting HTTP Server..."
	@echo "Server will be available at: http://localhost:8080"
	@echo "Type 'quit' to stop the server"
	@echo "========================================"
	./$(SERVER_TARGET)

# Run tests
test: $(TEST_TARGET)
	@echo "Running test suite..."
	@echo "========================================"
	./$(TEST_TARGET)

# Run tests with verbose output
test-verbose: $(TEST_TARGET)
	@echo "🧪 Running test suite (verbose)..."
	@echo "========================================"
	./$(TEST_TARGET) || true

# Benchmark the server (requires server to be running)
benchmark: $(SERVER_TARGET)
	@echo "🚀 Running server benchmarks..."
	@echo "Make sure the server is running in another terminal: make run"
	@echo "========================================"
	@if curl -s http://localhost:8080 > /dev/null 2>&1; then \
		echo "✅ Server is running, starting benchmark..."; \
		./benchmark.sh; \
	else \
		echo "❌ Server is not running at http://localhost:8080"; \
		echo "💡 Start the server with: make run"; \
		exit 1; \
	fi

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Clean and rebuild everything
rebuild: clean all
	@echo "Rebuilt project"

# Install (copy to system path - requires sudo)
install: $(SERVER_TARGET)
	sudo cp $(SERVER_TARGET) /usr/local/bin/
	@echo "HTTP Server installed to /usr/local/bin/"

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/httpserver
	@echo "HTTP Server uninstalled"

# Show help
help:
	@echo "HTTP Server Project Makefile"
	@echo "============================"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build server and tests (default)"
	@echo "  server       - Build only the HTTP server"
	@echo "  tests        - Build only the test suite"
	@echo "  debug        - Build with debug symbols"
	@echo "  run          - Build and run the HTTP server"
	@echo "  test         - Build and run tests"
	@echo "  test-verbose - Run tests with verbose output"
	@echo "  benchmark    - Run performance benchmarks (server must be running)"
	@echo "  clean        - Remove build artifacts"
	@echo "  rebuild      - Clean and rebuild everything"
	@echo "  install      - Install server to /usr/local/bin (requires sudo)"
	@echo "  uninstall    - Remove server from /usr/local/bin"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make run     - Build and start the server"
	@echo "  make test    - Build and run tests"
	@echo "  make clean   - Clean build files"
	@echo "  make rebuild - Fresh build"

# Aliases for convenience
server: $(SERVER_TARGET)
tests: $(TEST_TARGET)

# Phony targets
.PHONY: all debug run test test-verbose clean rebuild install uninstall help server tests

# Show build info
info:
	@echo "Build Configuration:"
	@echo "  Compiler: $(CXX)"
	@echo "  Flags: $(CXXFLAGS)"
	@echo "  Source Dir: $(SRC_DIR)"
	@echo "  Build Dir: $(BUILD_DIR)"
	@echo "  Binary Dir: $(BIN_DIR)"
	@echo "  Server Target: $(SERVER_TARGET)"
	@echo "  Test Target: $(TEST_TARGET)"
