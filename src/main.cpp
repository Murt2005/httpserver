#include <sys/resource.h>
#include <sys/time.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "httpMessage.h"
#include "httpServer.h"
#include "uri.h"

using simpleHttpServer::HttpMethod;
using simpleHttpServer::HttpRequest;
using simpleHttpServer::HttpResponse;
using simpleHttpServer::HttpServer;
using simpleHttpServer::HttpStatusCode;

int main(void) {
    std::string host = "0.0.0.0";
    int port = 8080;
    HttpServer server(host, port);

    auto sayHello = []([[maybe_unused]] const HttpRequest& request) -> HttpResponse {
        HttpResponse response(HttpStatusCode::Ok);
        response.setHeader("Content-Type", "text/plain");
        response.setContent("Hello, world\n");
        return response;
    };

    auto sendHtml = []([[maybe_unused]] const HttpRequest& request) -> HttpResponse {
        HttpResponse response(HttpStatusCode::Ok);
        std::string content;
        content += "<!doctype html>\n";
        content += "<html>\n<body>\n\n";
        content += "<h1>Hello, world in an Html page</h1>\n";
        content += "<p>A Paragraph</p>\n\n";
        content += "</body>\n</html>\n";

        response.setHeader("Content-Type", "text/html");
        response.setContent(content);
        return response;
    };

    server.registerHttpRequestHandler("/", HttpMethod::HEAD, sayHello);
    server.registerHttpRequestHandler("/", HttpMethod::GET, sayHello);
    server.registerHttpRequestHandler("/hello.html", HttpMethod::HEAD, sendHtml);
    server.registerHttpRequestHandler("/hello.html", HttpMethod::GET, sendHtml);

    try {
        std::cout << "Starting the web server..." << std::endl;
        server.Start();
        std::cout << "Server listening on " << host << ":" << port << std::endl;
        std::string command;
        while (std::cin >> command, command != "quit") {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "'quit' command entered. Stopping the web server..." << std::endl;
        server.Stop();
        std::cout << "Server stopped" << std::endl;
    } catch (std::exception& e) {
        std::cerr << "An exception occured: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
