# Simple HTTP/1.1 Server

A high-performance, multi-threaded HTTP/1.1 server written in C++17, designed for macOS with kqueue-based I/O multiplexing.

## Features

- **Multi-threaded Architecture**: 1 listener thread + 5 worker threads for optimal performance
- **Efficient I/O**: Uses macOS kqueue for scalable event-driven I/O multiplexing
- **HTTP/1.1 Compliant**: Full support for HTTP/1.1 specification
- **Modern C++**: Built with C++17 features and best practices

## Supported HTTP Features

### HTTP Methods
- GET, HEAD, POST, PUT, DELETE
- CONNECT, OPTIONS, TRACE, PATCH

### HTTP Status Codes
- **2xx**: 200 OK, 201 Created, 202 Accepted, 204 No Content, etc.
- **3xx**: 300 Multiple Choices, 301 Moved Permanently, 302 Found, 304 Not Modified
- **4xx**: 400 Bad Request, 401 Unauthorized, 403 Forbidden, 404 Not Found, 405 Method Not Allowed
- **5xx**: 500 Internal Server Error, 501 Not Implemented, 502 Bad Gateway, 503 Service Unavailable

### HTTP Versions
- HTTP/0.9, HTTP/1.0, HTTP/1.1, HTTP/2.0 (parsing support)

## Architecture

```
┌─────────────────┐
│   Main Thread   │ ← User interaction & lifecycle management
└─────────────────┘
         │
┌─────────────────┐
│ Listener Thread │ ← Accepts connections & distributes to workers
└─────────────────┘
         │
    ┌────┴────┐
    │ Round-  │
    │ Robin   │
    │ Load    │
    │ Balance │
    └────┬────┘
    ┌────┴────┐
    │ Worker  │ ← kqueue-based I/O processing
    │ Threads │   (5 threads)
    └─────────┘
```

### Key Components

- **HttpServer**: Main server class managing threads and connections
- **HttpRequest/HttpResponse**: HTTP message objects with full parsing/serialization
- **Uri**: URI handling with automatic lowercase conversion
- **EventData**: Buffer management for efficient I/O operations
- **kqueue Integration**: macOS-optimized event loop

## Installation & Build

### Prerequisites
- macOS (tested on macOS 14.6.0)
- C++17 compatible compiler (GCC 7+ or Clang 5+)
- Make

### Quick Start
```bash
# Clone and build
git clone <repository-url>
cd httpserver

# Build everything
make

# Run the server
make run

# Run tests
make test
```

### Build Options
```bash
make help          # Show all available commands
make all           # Build server and tests
make server        # Build only the HTTP server
make tests         # Build only the test suite
make debug         # Build with debug symbols
make clean         # Clean build artifacts
make rebuild       # Clean and rebuild everything
```


## Performance Characteristics

- **Concurrent Connections**: Supports up to 10,000 concurrent connections
- **Thread Pool**: 5 worker threads + 1 listener thread
- **I/O Multiplexing**: kqueue-based event loop for efficient I/O
- **Memory Management**: Optimized buffer management with 4KB buffers
- **Load Balancing**: Round-robin connection distribution

## Benchmarking

This HTTP server is designed to handle the C10K problem of efficiently handling 10,000 concurrent connections efficiently. I used [wrk](https://github.com/wg/wrk) for performance testing.

### Quick Benchmark

```bash
# Make sure server is running first
make run

# In another terminal, run the benchmark script
./benchmark.sh

# Or run specific tests
./benchmark.sh light    # 100 connections
./benchmark.sh medium   # 1,000 connections  
./benchmark.sh heavy    # 5,000 connections
./benchmark.sh c10k     # 10,000 connections (C10K challenge)
./benchmark.sh extreme  # 15,000 connections
```

### Manual Benchmarking with wrk

```bash
# Basic benchmark (100 connections, 12 threads, 30 seconds)
wrk -c 100 -t 12 -d 30s --latency http://localhost:8080

# C10K Challenge (10,000 concurrent connections)
wrk -c 10000 -t 16 -d 30s --latency http://localhost:8080

# Extreme load test (15,000 connections)
wrk -c 15000 -t 20 -d 30s --latency http://localhost:8080
```

## Results

Result when running the server

```
Starting HTTP Server...
Server will be available at: http://localhost:8080
Type 'quit' to stop the server
========================================
Starting the web server...
Server listening on 0.0.0.0:8080
```

Results from running each benchmarking test.

100 connections:
```
HTTP Server Benchmarking Suite
==================================
 Server is running at http://localhost:8080

 Testing: Light Load Test
Connections: 100, Threads: 4, Duration: 30s
----------------------------------------
Running 30s test @ http://localhost:8080
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   517.24us  171.50us   6.28ms   75.84%
    Req/Sec    40.80k     1.33k   44.02k    81.40%
  Latency Distribution
     50%  493.00us
     75%  595.00us
     90%  725.00us
     99%    1.02ms
  4887035 requests in 30.10s, 363.53MB read
Requests/sec: 162356.25
Transfer/sec:     12.08MB
```
1000 connections:
```
HTTP Server Benchmarking Suite
==================================
 Server is running at http://localhost:8080

 Testing: Medium Load Test
Connections: 1000, Threads: 8, Duration: 30s
----------------------------------------
Running 30s test @ http://localhost:8080
  8 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     5.91ms    2.11ms  44.15ms   78.75%
    Req/Sec    20.28k     1.50k   33.93k    84.08%
  Latency Distribution
     50%    5.57ms
     75%    6.76ms
     90%    8.29ms
     99%   12.57ms
  4847102 requests in 30.06s, 360.56MB read
Requests/sec: 161259.74
Transfer/sec:     12.00MB
```

10,000 connections:
```
HTTP Server Benchmarking Suite
==================================
 Server is running at http://localhost:8080

 Testing: C10K Challenge Test
Connections: 10000, Threads: 16, Duration: 30s
----------------------------------------
Running 30s test @ http://localhost:8080
  16 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   124.49ms  185.38ms   2.00s    87.85%
    Req/Sec     5.92k     2.03k   14.38k    74.79%
  Latency Distribution
     50%   46.33ms
     75%  130.25ms
     90%  383.11ms
     99%  854.83ms
  2822775 requests in 30.09s, 209.98MB read
  Socket errors: connect 0, read 9024, write 0, timeout 445
Requests/sec:  93799.05
Transfer/sec:      6.98MB
```
