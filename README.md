# Simple HTTP/1.1 Server

A high-performance, multi-threaded HTTP/1.1 server written in C++17, designed for macOS with kqueue-based I/O multiplexing.

## Features

- **Multi-threaded Architecture**: 1 listener thread + 5 worker threads for optimal performance
- **Efficient I/O**: Uses macOS kqueue for scalable event-driven I/O multiplexing
- **HTTP/1.1 Compliant**: Full support for HTTP/1.1 specification
- **Modern C++**: Built with C++17 features and best practices

## 📋 Supported HTTP Features

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


## ⚡ Performance Characteristics

- **Concurrent Connections**: Supports up to 10,000 concurrent connections
- **Thread Pool**: 5 worker threads + 1 listener thread
- **I/O Multiplexing**: kqueue-based event loop for efficient I/O
- **Memory Management**: Optimized buffer management with 4KB buffers
- **Load Balancing**: Round-robin connection distribution


## Example Output

```
Starting HTTP Server...
Server will be available at: http://localhost:8080
Type 'quit' to stop the server
========================================
Starting the web server...
Server listening on 0.0.0.0:8080
```
