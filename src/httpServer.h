#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <chrono>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "httpMessage.h"
#include "uri.h"

namespace simpleHttpServer {

constexpr size_t kMaxBufferSize = 4096;

// struct contains data for managing socket events in the kqueue based event loop
struct EventData {
    EventData()
        : fd(0)
        , length(0)
        , cursor(0)
        , buffer()
    { }

    int fd; // file descriptor for the socket connection
    size_t length; // total length of data in buffer (for writes) or bytes to read
    size_t cursor; // current position in buffer for partial I/O operations
    char buffer [kMaxBufferSize]; // buffer for storing HTTP message data during I/O
};

using HttpRequestHandler_t = std::function<HttpResponse(const HttpRequest&)>;

// the httpserver class implements a scalable HTTP server using the following architecture:
// - 1 main thread which handles user interaction and server lifecycle management
// - 1 listener thread dedicated to accepting incoming client connections
// - 5 worker threads which process HTTP requests and send responses using kqueue I/O
//
// the server uses macOS kqueue for efficient I/O multiplexing, allowing it to handle
// thousands of concurrent connections with minimal resource overhead. Each worker thread
// manages its own kqueue instance and processes events independently
class HttpServer {
public:
    // creates a new HTTP server instance and initializes the socket for the specified
    // host and port. the server is not started until Start() is called
    explicit HttpServer(const std::string& host, std::uint16_t port);

    // destructor
    ~HttpServer() = default;

    // move semantics
    HttpServer() = default; // default constructor
    HttpServer(HttpServer&&) = default; // move constructor
    HttpServer& operator=(HttpServer&) = delete; // move assignment operator

    // initializes the server socket, sets up kqueue instances for worker threads,
    // and starts the listener and worker threads. the server will begin accepting
    // HTTP requests once this method completes successfully
    void Start();

    // shuts down the server by stopping all threads, closing connections,
    // and cleaning up system resources. this method blocks until all threads have
    // finished and resources are released
    void Stop();

    // registers a user defined handler function that will be called when a request
    // matches the specified path and HTTP method. the handler receives the request
    // and must return an appropriate response
    void registerHttpRequestHandler(const std::string& path, HttpMethod method, const HttpRequestHandler_t callback) {
        Uri uri(path);
        requestHandlers_[uri].insert(std::make_pair(method, std::move(callback)));
    }

    // alternative registration method that accepts a Uri object instead of a string
    // allows for more complex URI handling and validation
    void registerHttpRequestHandler(const Uri& uri, HttpMethod method, const HttpRequestHandler_t callback) {
        requestHandlers_[uri].insert(std::make_pair(method, std::move(callback)));
    }

    // getters for server configuration and state
    std::string host() const { return host_; }
    std::uint16_t port() const { return port_; }
    bool running() const { return running_; }

private:
    static constexpr int kBacklogSize = 1000; // maximum pending connections in listen queue
    static constexpr int kMaxConnections = 10000; // maximum concurrent connections supported
    static constexpr int kMaxEvents = 10000; // maximum events per kevent call
    static constexpr int kThreadPoolSize = 5; // number of worker threads in the pool

    std::string host_; // host address the server is bound to
    std::uint16_t port_; // port number the server is listening on
    int sock_fd_; // main server socket file descriptor
    bool running_; // flag indicating if server is currently running

    std::thread listenerThread_; // thread dedicated to accepting new connections
    std::thread workerThreads_[kThreadPoolSize]; // array of worker threads for request processing

    int workerKqueueFD_[kThreadPoolSize]; // kqueue file descriptor for each worker thread
    struct kevent workerEvents_[kThreadPoolSize][kMaxEvents]; // event arrays for kevent results

    std::map<Uri, std::map<HttpMethod, HttpRequestHandler_t>> requestHandlers_; // registered request handlers

    std::mt19937 rng_; // random number generator for load balancing
    std::uniform_int_distribution<int> sleepTimes_; // distribution for random sleep times

    // creates a non blocking TCP socket and configures it for server use
    // this method is called during server initilization
    void CreateSocket();

    // creates kqueue file descriptors for each worker thread to enable efficient
    // I/O multiplexing. each worker thread will have its own kqueue instance
    // to monitor socket events
    void SetUpKqueue();

    // continously accepts incoming client connections and distributes them
    // round-robin to worker threads. this method runs in the dedicated
    // listener thread and handles the initial connection setup
    void Listen();

    // processes kqueue events for a specific worker thread. this method runs
    // continuously in each worker thread, handling read/write events on
    // client connections and processing HTTP requests
    void ProcessEvents(int workerId);

    // processes individual kqueue events, handling both read (EVFILT_READ) and write
    // (EVFILT_WRITE) operations. manages partial I/O operations and connection state
    void HandleKqueueEvent(int kqueueFd, EventData* event, int filter);

    // parses the incoming HTTP request, processes it through registered handlers,
    // and generates the appriopriate HTTP response. handles parsing errors and
    // generates appriopriate error responses
    void HandleHttpData(const EventData& request, EventData* response);

    // looks up the registered handler for the request's URI and method,
    // calls the handler, and it returns the response. Returns 404 or 405
    // error response if no handler is found
    HttpResponse HandleHttpResponse(const HttpRequest& request);

    // wrapper function for kevent that provides error handling and 
    // consistent interface for managing kqueue events across the server
    void controlKqueueEvent(int kqueueFd, int ident, int filter, int flags, int fflags = 0, intptr_t data = 0, void* udata = nullptr);
};

}

#endif
