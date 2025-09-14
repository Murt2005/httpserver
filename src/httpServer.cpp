#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
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
    
    HttpServer::HttpServer(const std::string &host, std::uint16_t port)
        : host_(host)
        , port_(port)
        , sock_fd_(0)
        , running_(false)
        , workerEpollFd()
        , rng_(std::chrono::steady_clock::now().time_since_epoch().count())
        , sleepTimes_(10, 100)
    { CreateSocket(); }

    void HttpServer::Start() {
        int opt = 1;
        sockaddr_in serverAddress;

        if (setsockopt(sock_fd_, SQL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set socket operations");
        }

        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        inet_pton(AF_INET, host_.c_str(), &(serverAddress.sin_addr.s_addr));
        serverAddress.sin_port = htons(port_);

        if (bind(sock_fd_, (sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(sock_fd_, kBacklogSize) < 0) {
            std::ostringstream msg;
            msg << "Failed to listen on port " << port_;
            throw std::runtime_error(msg.str());
        }

        SetUpEpoll();
        running_ = true;
        listenerThread_ = std::thread(&HttpServer::Listen, this);
        for (int i = 0; i < kThreadPoolSize; i++) {
            workerThreads_[i] = std::thread(&HttpServer::ProcessEvents, this, i);
        }
    }

    void HttpServer::Stop() {
        running_ = false;
        listenerThread_.join();
        for (int i = 0; i < kThreadPoolSize; i++) {
            workerThreads_[i].join();
        }
        for (int i = 0; i < kThreadPoolSize; i++) {
            close(workerEpollFD_[i]);
        }
        close(sock_fd_);
    }

    void HttpServer::CreateSocket() {
        if ((sock_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
            throw std::runtime_error("Failed to create a TCP socket");
        }
    }

    void HttpServer::SetUpEpoll() {
        for (int i = 0; i < kThreadPoolSize; i++) {
            if ((workerEpollFD_[i] = epoll_create(0)) < 0) {
                throw std::runtime_error("Failed to create epoll file descriptor for worker");
            }
        }
    }

    void HttpServer::Listen() {

    }

    void HttpServer::ProcessEvents(int workerId) {

    }

    void HttpServer::HandleEpollEvent(int epollFd, EventData* event, std::uint32_t events) {

    }

    HttpResponse HttpServer::HandleHttpResponse(const HttpRequest& request) {

    }

    void controlEpollEvent(int epollFd, int op, int fd, std::uint32_t events, void *data)
}
