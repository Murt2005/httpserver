#!/bin/bash

# HTTP Server Benchmarking Script

echo "HTTP Server Benchmarking Suite"
echo "=================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Server configuration
SERVER_URL="http://localhost:8080"
SERVER_BINARY="./bin/httpserver"
BENCHMARK_DURATION="30s"

# Check if server binary exists
if [ ! -f "$SERVER_BINARY" ]; then
    echo -e "${RED} Server binary not found. Building...${NC}"
    make clean && make
    if [ $? -ne 0 ]; then
        echo -e "${RED} Failed to build server${NC}"
        exit 1
    fi
fi

# Function to run benchmark
run_benchmark() {
    local connections=$1
    local threads=$2
    local description=$3
    
    echo -e "\n${BLUE} Testing: $description${NC}"
    echo "Connections: $connections, Threads: $threads, Duration: $BENCHMARK_DURATION"
    echo "----------------------------------------"
    
    wrk -c $connections -t $threads -d $BENCHMARK_DURATION --latency $SERVER_URL
}

# Function to check if server is running
check_server() {
    if ! curl -s $SERVER_URL > /dev/null 2>&1; then
        echo -e "${RED} Server is not running at $SERVER_URL${NC}"
        echo -e "${YELLOW} Start the server with: make run${NC}"
        exit 1
    fi
    echo -e "${GREEN} Server is running at $SERVER_URL${NC}"
}

# Main benchmarking sequence
main() {
    echo -e "${YELLOW} Checking server status...${NC}"
    check_server
    
    echo -e "\n${GREEN} Starting C10K Benchmark Tests${NC}"
    echo "This will test the server's ability to handle 10,000 concurrent connections"
    
    # Test 1: Light load baseline
    run_benchmark 100 4 "Light Load (100 connections)"
    
    # Test 2: Medium load
    run_benchmark 1000 8 "Medium Load (1,000 connections)"
    
    # Test 3: Heavy load
    run_benchmark 5000 12 "Heavy Load (5,000 connections)"
    
    # Test 4: C10K Challenge
    run_benchmark 10000 16 "C10K Challenge (10,000 connections)"
    
    # Test 5: Extreme load
    run_benchmark 15000 20 "Extreme Load (15,000 connections)"
    
    echo -e "\n${GREEN} Benchmarking Complete!${NC}"
    echo -e "${YELLOW} Tips for analysis:${NC}"
    echo "   • Requests/sec: Higher is better"
    echo "   • Latency: Lower is better (especially 99th percentile)"
    echo "   • Errors: Should be 0 for healthy server"
    echo "   • C10K success: Server should handle 10K+ connections without errors"
}

# Handle script arguments
case "${1:-all}" in
    "light")
        check_server
        run_benchmark 100 4 "Light Load Test"
        ;;
    "medium")
        check_server
        run_benchmark 1000 8 "Medium Load Test"
        ;;
    "heavy")
        check_server
        run_benchmark 5000 12 "Heavy Load Test"
        ;;
    "c10k")
        check_server
        run_benchmark 10000 16 "C10K Challenge Test"
        ;;
    "extreme")
        check_server
        run_benchmark 15000 20 "Extreme Load Test"
        ;;
    "all"|*)
        main
        ;;
esac
