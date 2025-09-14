/**
 * @file httpMessage.h
 * @brief Defines HTTP message objects and utility functions
 * 
 * Provides core data structures and utilities needed to represent, parse, and manipulate HTTP requests and responses
 * Includes enums for HTTP methods, versions, and status codes
 * Includes classes that model HTTP messages according to HTTP/1.1 specification
 */

#ifndef HTTPMESSAGE_H_
#define HTTPMESSAGE_H_

#include <map>
#include <string>
#include <utility>

#include "uri.h"

namespace simpleHttpServer {

enum class HttpMethod {
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
    PATCH
};

enum class HttpVersion {
    HTTP_0_9 = 9,
    HTTP_1_0 = 10,
    HTTP_1_1 = 11,
    HTTP_2_0 = 20
};

enum class HttpStatusCode {
    // 1xx Informational
    Continue = 100,
    SwitchingProtocols = 101,

    // 2xx Successful
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,

    // 3xx Redirection
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    NotModified = 304,

    //4xx Client Error
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    RequestTimeout = 408,
    ImATeapot = 418,

    // 5xx Server Error
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HttpVersionNotSupported = 505
};

std::string toString(HttpMethod method);
std::string toString(HttpVersion version);
std::string toString(HttpStatusCode statusCode); 
HttpMethod stringToMethod(const std::string& methodString);
HttpVersion stringToVersion(const std::string& versionString);

class HttpMessageInferace {
public:
    HttpMessageInferace()
        : version_(HttpVersion::HTTP_1_1)
    { }

    virtual ~HttpMessageInferace() = default;

    void setHeader(const std::string& key, const std::string& value) {
        headers_[key] = std::move(value);
    }

    void removeHeader(const std::string& key) {
        headers_.erase(key);
    }

    void clearHeader() {
        headers_.clear();
    }

    void setContent(const std::string& content) {
        content_ = std::move(content);
        SetContentLength();
    }

    void clearContent(const std::string& content) {
        content_.clear();
        SetContentLength();
    }

    HttpVersion version() const { return version_; }

    std::string header(const std::string& key) const {
        if (headers_.count(key) > 0 ) {
            return headers_.at(key);
        }
        return std::string();
    }

    std::map<std::string, std::string> headers() const { return headers_; }

    std::string content() const { return content_; }

    size_t content_length() const { return content_.length(); }

protected:
    HttpVersion version_;
    std::map<std::string, std::string> headers_;
    std::string content_;

    void SetContentLength() {
        setHeader("Content-Length", std::to_string(content_.length()));
    }
};

class HttpRequest : public HttpMessageInferace {
public:
    HttpRequest()
        : method_(HttpMethod::GET)
    { }

    ~HttpRequest() = default;

    void setMethod(HttpMethod method) {
        method_ = method;
    }

    void setUri(const Uri& uri) {
        uri_ = std::move(uri);
    }

    HttpMethod method() const {
        return method_;
    }

    Uri uri() const {
        return uri_;
    }

    friend std::string toString(const HttpRequest& request);
    friend HttpRequest stringToRequest(const std::string& requestString);

private:
    HttpMethod method_;
    Uri uri_;
};

class HttpResponse : public HttpMessageInferace {
public:
    HttpResponse()
        : statusCode_(HttpStatusCode::Ok)
    { }

    ~HttpResponse() = default;

    void setStatusCode(HttpStatusCode statusCode) {
        statusCode_ = statusCode;
    }

    HttpStatusCode statusCode() const {
        return statusCode_;
    }

    friend std::string toString(const HttpRequest& request, bool sendContent);
    friend HttpResponse stringToResponse(const std::string& responseString);

private:
    HttpStatusCode statusCode_;
};

std::string toString(const HttpRequest& request);
std::string toString(const HttpResponse& response, bool sendContent = true);
HttpRequest stringToRequest(const std::string& requestString);
HttpResponse stringToResponse(const std::string& responseString);

}

#endif
