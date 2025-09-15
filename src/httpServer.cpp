#include <arpa/inet.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "httpServer.h"
#include "httpMessage.h"
#include "uri.h"

namespace simpleHttpServer {
    
    // initializes the HTTP server with the specified host and port. the constructor
    // sets up the configuration, initializes the random number generator for load balancing
    // and creates the main server socket.
    HttpServer::HttpServer(const std::string &host, std::uint16_t port)
        : host_(host)
        , port_(port)
        , sock_fd_(0)
        , running_(false)
        , workerKqueueFD_()
        , rng_(std::chrono::steady_clock::now().time_since_epoch().count())
        , sleepTimes_(10, 100) {
    CreateSocket();
}

    void HttpServer::Start() {
        int opt = 1; // option value for socket configuration (1 = enable)
        sockaddr_in serverAddress; // internet socket address structure for binding

        // configure socket options to allow address and port reuse
        // SO_REUSEADDR: allows binding to an address that's already in use
        // SO_REUSEPORT: allows multiple sockets to bind to the same port (load balancing)
        if (setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set SO_REUSEADDR");
        }
        if (setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set SO_REUSEPORT");
        }

        serverAddress.sin_family = AF_INET; // set address family to IPv4
        serverAddress.sin_addr.s_addr = INADDR_ANY; // set address to accept connections from any interface
        inet_pton(AF_INET, host_.c_str(), &(serverAddress.sin_addr.s_addr)); // convert host string to network byte order
        serverAddress.sin_port = htons(port_); // convert port number to network byte order

        // bind the socket to the specified address and port
        if (bind(sock_fd_, (sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }

        // start listening for incoming connections with the specified backlog size
        if (listen(sock_fd_, kBacklogSize) < 0) {
            std::ostringstream msg;
            msg << "Failed to listen on port " << port_;
            throw std::runtime_error(msg.str());
        }

        SetUpKqueue(); // create kqueue instances for all worker threads
        running_ = true;
        listenerThread_ = std::thread(&HttpServer::Listen, this); // start the listener thread
        for (int i = 0; i < kThreadPoolSize; i++) {
            workerThreads_[i] = std::thread(&HttpServer::ProcessEvents, this, i); // start each worker thread
        }
    }

    void HttpServer::Stop() {
        running_ = false; 
        listenerThread_.join();
        for (int i = 0; i < kThreadPoolSize; i++) {
            workerThreads_[i].join();
        }
        for (int i = 0; i < kThreadPoolSize; i++) {
            close(workerKqueueFD_[i]);
        }
        close(sock_fd_);
    }

    void HttpServer::CreateSocket() {
        // create a TCP socket for IPv4 communication
        // AF_INET: IPv4 address family
        // SOCK_STREAM: TCP socket type (reliable, connection-oriented)
        // 0: protocol (0 = default for TCP)
        if ((sock_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            throw std::runtime_error("Failed to create a TCP socket");
        }
        
        // make the socket non-blocking for use with kqueue
        int flags = fcntl(sock_fd_, F_GETFL, 0);
        if (flags < 0 || fcntl(sock_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(sock_fd_);
            throw std::runtime_error("Failed to make socket non-blocking");
        }
    }

    void HttpServer::SetUpKqueue() {
        for (int i = 0; i < kThreadPoolSize; i++) {
            // create a kqueue file descriptor for this worker thread
            // kqueue(): create kqueue instance
            // returns file descriptor on success, -1 on failure
            if ((workerKqueueFD_[i] = kqueue()) < 0) {
                throw std::runtime_error("Failed to create kqueue file descriptor for worker");
            }
        }
    }

    void HttpServer::Listen() {
        EventData *clientData; // pointer to EventData for new client connection
        sockaddr_in clientAddress; // client address structure for accept4()
        socklen_t clientLen = sizeof(clientAddress); // length of client address structure
        int clientFd; // file descriptor for accepted client connection
        int currentWorker = 0; // index of current worker thread
        bool active = true; // flag indicating if we're actively accepting connections

        // accept new connections and distribute them to worker threads
        while (running_) {
            // if not actively accepting connections, sleep for a random time to avoid busy waiting when no connections available
            if (!active) {
                std::this_thread::sleep_for(std::chrono::microseconds(sleepTimes_(rng_)));
            }

            // accept a new client connection
            // sock_fd_: server socket file descriptor
            // clientAddress: structure to store client address information
            // clientLen: length of client address structure (input/output parameter)
            clientFd = accept(sock_fd_, (sockaddr *)&clientAddress, &clientLen);

            // if accept4() failed, set active flag to false, and continue to next interation
            if (clientFd < 0) {
                active = false;
                continue;
            }

            // set active flag to true (connection accepted), create new EventData structure for the client
            // make the client socket non-blocking
            int flags = fcntl(clientFd, F_GETFL, 0);
            if (flags >= 0) {
                fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);
            }
            
            // store client file descriptor in EventData
            active = true;
            clientData = new EventData();
            clientData->fd = clientFd;

            // add the client socket to the current worker thread's kqueue instance
            // workerKqueueFD_[currentWorker]: kqueue file descriptor for current worker
            // EV_ADD: add the file descriptor to kqueue monitoring
            // clientFd: client socket file descriptor to monitor
            // EVFILT_READ: monitor for read events (incoming data)
            // clientData: user data pointer associated with this file descriptor
            controlKqueueEvent(workerKqueueFD_[currentWorker], clientFd, EVFILT_READ, EV_ADD, 0, 0, clientData);
            currentWorker++;
            if (currentWorker == HttpServer::kThreadPoolSize) {
                currentWorker = 0;
            }
        }
    }

    void HttpServer::ProcessEvents(int workerId) {
        EventData *data; // pointer to EventData for current event
        int kqueueFd = workerKqueueFD_[workerId]; // kqueue file descriptor for this worker thread
        bool active = true; // flag indicating if we're actively processing events

        while (running_) {
            // if not actively processing events, sleep for a random time to avoid busy waiting when no events are available
            if (!active) {
                std::this_thread::sleep_for(std::chrono::microseconds(sleepTimes_(rng_)));
            }

            // wait for kqueue events on this worker thread's monitored file descriptors
            // workerKqueueFD_[workerId]: kqueue file descriptor for this worker
            // workerEvents_[workerId]: array to store returned events
            // kMaxEvents: maximum number of events to return
            // 0: timeout (0 = non-blocking, return immediately)
            struct timespec timeout = {0, 0}; // non-blocking
            int nfds = kevent(kqueueFd, nullptr, 0, workerEvents_[workerId], HttpServer::kMaxEvents, &timeout);

            // if no events or error occurred, set active flag to false, continue to next iteration
            if (nfds <= 0) {
                active = false;
                continue;
            }

            active = true;

            // process each returned event
            for (int i = 0; i < nfds; i++) {
                const struct kevent &current_event = workerEvents_[workerId][i]; // get reference to current event
                data = reinterpret_cast<EventData *>(current_event.udata); // cast user data back to EventData*

                // check for connection errors or hangup
                // if connection was closed by peer or if an error occurred on the socket
                if ((current_event.flags & EV_EOF) || (current_event.flags & EV_ERROR)) {
                    controlKqueueEvent(kqueueFd, data->fd, EVFILT_READ, EV_DELETE); // remove socket from kqueue monitoring
                    close(data->fd); // close the socket file descriptor
                    delete data; // free the EventData memory
                // if socket is ready for reading or if socket is ready for writing
                } else if ((current_event.filter == EVFILT_READ) || (current_event.filter == EVFILT_WRITE)) {
                    HandleKqueueEvent(kqueueFd, data, current_event.filter); // process the specific event
                } else { // something unexpected happens
                    controlKqueueEvent(kqueueFd, data->fd, EVFILT_READ, EV_DELETE); // remove socket from kqueue monitoring
                    close(data->fd); // close the socket file descriptor
                    delete data; // free the EventData memory
                }
            }
        }
    }

    void HttpServer::HandleKqueueEvent(int kqueueFd, EventData* data, int filter) {
        int fd = data->fd; // extract socket file descriptor from EventData
        EventData *request, *response; // pointers for request and response EventData

        // if this is a read event
        if (filter == EVFILT_READ) {
            request = data; // treat the event data as a request buffer

            // attempt to read data from the socket into the request buffer
            // fd: socket file descriptor to read from
            // request->buffer: buffer to store the received data
            // kMaxBufferSize: max number of bytes to read
            // 0: no special flags
            ssize_t byte_count = recv(fd, request->buffer, kMaxBufferSize, 0);

            // if we successfully read some data
            if (byte_count > 0) {
                response = new EventData(); // create new EventData for response
                response->fd = fd; // set response file descriptor to client socket
                HandleHttpData(*request, response); // process the HTTP request and generate response
                controlKqueueEvent(kqueueFd, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, response); // modify kqueue monitoring to watch for write events (ready to send response)
                delete request; // free the request EventData memory
            // if client has closed the connection
            } else if (byte_count == 0) {
                controlKqueueEvent(kqueueFd, fd, EVFILT_READ, EV_DELETE); // remove socket from kqueue monitoring
                close(fd); // close the socket file descriptor
                delete request; // free the request EventData memory
            // if recv() returned an error
            } else {
                // if error is "try again"
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    request->fd = fd; // ensure file descriptor is set
                    controlKqueueEvent(kqueueFd, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, request); // keep monitoring for read events (data will be available later)
                // if error is something else (connection broken, etc...)
                } else {
                    controlKqueueEvent(kqueueFd, fd, EVFILT_READ, EV_DELETE); // remove socket from kqueue monitoring
                    close(fd); // close the socket file descriptor
                    delete request; // free the request EventData memory
                }
            }
        // if this is a write event (socket ready to send data)
        } else if (filter == EVFILT_WRITE) {
            response = data; // treat the EventData as a response buffer
            // attempt to send data from the response buffer to the socket
            // fd: socket file descriptor to write to
            // response->buffer + response->cursor: start writing from current cursor position
            // response->length: number of bytes remaining to write
            // 0: no special flags
            ssize_t byte_count = send(fd, response->buffer + response->cursor, response->length, 0);

            // if send was successful
            if (byte_count >= 0) {
                // if we didn't send all of the data
                if (static_cast<size_t>(byte_count) < response->length) {
                    response->cursor += byte_count; // update cursor to position after sent bytes
                    response->length -= byte_count; // reduce remaining length by sent bytes
                    controlKqueueEvent(kqueueFd, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, response); // keep monitoring for write events
                // if we sent all of the data
                } else {
                    request = new EventData(); // create new EventData for next request
                    request->fd = fd; // set file descriptor for next request
                    controlKqueueEvent(kqueueFd, fd, EVFILT_WRITE, EV_DELETE); // remove write monitoring
                    controlKqueueEvent(kqueueFd, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, request); // switch back to monitoring read events
                    delete response; // free the response EventData memory
                }
            // if send() returned an error
            } else {
                // if error is "try again" (socket not ready)
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    controlKqueueEvent(kqueueFd, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, response); // keep monitoring for write events (socket will be ready later)
                // if error is something else
                } else {
                    controlKqueueEvent(kqueueFd, fd, EVFILT_READ, EV_DELETE); // remove socket from kqueue monitoring
                    close(fd); // close the socket file descriptor
                    delete response; // free the response EventData memory
                }
            }
        }
    }

    void HttpServer::HandleHttpData(const EventData& request, EventData* response) {
        std::string requestString(request.buffer), responseString; // convert buffer to string and prepare response string
        HttpRequest httpRequest; // httpRequest object for parsed request
        HttpResponse httpResponse; // httpResponse object for parsed response

        try {
            httpRequest = stringToRequest(requestString); // parse the raw HTTP request string into HttpRequest object
            httpResponse = HandleHttpResponse(httpRequest); // route the request to appropriate handler and get response
        } catch (const std::invalid_argument &e) { // if request parsing failed (malformed request)
            httpResponse = HttpResponse(HttpStatusCode::BadRequest); // create 400 Bad Request response
            httpResponse.setContent(e.what()); // set error message as response content
        } catch (const std::logic_error &e) { // if HTTP version is not supported
            httpResponse = HttpResponse(HttpStatusCode::HttpVersionNotSupported); // create 505 HTTP Version Not Supported response
            httpResponse.setContent(e.what()); // set error message as response content
        } catch (const std::exception &e) { // if any other unexpected error occured
            httpResponse = HttpResponse(HttpStatusCode::InternalServerError); // create 500 Internal Server Error response
            httpResponse.setContent(e.what()); // set error message as response content
        }

        // convert the HttpResponse object to a string for network transmission
        // for HEAD requests, don't include the response body
        // for all other requests, include the response body
        responseString = toString(httpResponse, httpRequest.method() != HttpMethod::HEAD);

        // copy the response string to the raw response buffer
        memcpy(response->buffer, responseString.c_str(), kMaxBufferSize);
        response->length = responseString.length();
    }

    HttpResponse HttpServer::HandleHttpResponse(const HttpRequest& request) {
        // look up the request handlers for the requests URI
        // requestHandlers_ is a map: Uri -> map<HttpMethod, HttpRequestHandler_t>
        auto it = requestHandlers_.find(request.uri());

        // if no handlers are registered for this URI return NotFound statuscode
        if (it == requestHandlers_.end()) {
            return HttpResponse(HttpStatusCode::NotFound);
        }

        // look up the specific handler for the request's HTTP method
        // it->second is the map<HttpMethod, HttpRequestHandler_t> for this URI
        auto callBackIt = it->second.find(request.method());

        // if no handler is registered for this method on this URI return MethodNotAllowed statuscode
        if (callBackIt == it->second.end()) {
            return HttpResponse(HttpStatusCode::MethodNotAllowed);
        }

        // call the registered handler function with the request and return its response
        return callBackIt->second(request);
    }

    void HttpServer::controlKqueueEvent(int kqueueFd, int ident, int filter, int flags, int fflags, intptr_t data, void* udata) {
        struct kevent changes[1];
        
        // create kevent structure for the operation
        EV_SET(&changes[0], ident, filter, flags, fflags, data, udata);
        
        // call kevent to modify the kqueue
        if (kevent(kqueueFd, changes, 1, nullptr, 0, nullptr) < 0) {
            if (flags & EV_DELETE) {
                throw std::runtime_error("Failed to remove file descriptor from kqueue");
            } else {
                throw std::runtime_error("Failed to add/modify file descriptor in kqueue");
            }
        }
    }
}
