/**
 * @file httpMessage.cpp
 * @brief Implementation of HTTP message serialization and parsing utilities
 * 
 * Contains the implementation of all utility functions declared in httpMessage.h
 * Provides conversion between HTTP objects and strings
 * Provides serialization and deserialization of HTTP request and response strings
 * Supports HTTP/1.1
 */

#include "httpMessage.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace simpleHttpServer {

std::string toString(HttpMethod method) {
  switch (method) {
    case HttpMethod::GET:
      return "GET";
    case HttpMethod::HEAD:
      return "HEAD";
    case HttpMethod::POST:
      return "POST";
    case HttpMethod::PUT:
      return "PUT";
    case HttpMethod::DELETE:
      return "DELETE";
    case HttpMethod::CONNECT:
      return "CONNECT";
    case HttpMethod::OPTIONS:
      return "OPTIONS";
    case HttpMethod::TRACE:
      return "TRACE";
    case HttpMethod::PATCH:
      return "PATCH";
    default:
      return std::string();
  }
}

std::string toString(HttpVersion version) {
  switch (version) {
    case HttpVersion::HTTP_0_9:
      return "HTTP/0.9";
    case HttpVersion::HTTP_1_0:
      return "HTTP/1.0";
    case HttpVersion::HTTP_1_1:
      return "HTTP/1.1";
    case HttpVersion::HTTP_2_0:
      return "HTTP/2.0";
    default:
      return std::string();
  }
}

std::string toString(HttpStatusCode status_code) {
  switch (status_code) {
    case HttpStatusCode::Continue:
      return "Continue";
    case HttpStatusCode::Ok:
      return "OK";
    case HttpStatusCode::Accepted:
      return "Accepted";
    case HttpStatusCode::MovedPermanently:
      return "Moved Permanently";
    case HttpStatusCode::Found:
      return "Found";
    case HttpStatusCode::BadRequest:
      return "Bad Request";
    case HttpStatusCode::Forbidden:
      return "Forbidden";
    case HttpStatusCode::NotFound:
      return "Not Found";
    case HttpStatusCode::MethodNotAllowed:
      return "Method Not Allowed";
    case HttpStatusCode::ImATeapot:
      return "I'm a Teapot";
    case HttpStatusCode::InternalServerError:
      return "Internal Server Error";
    case HttpStatusCode::NotImplemented:
      return "Not Implemented";
    case HttpStatusCode::BadGateway:
      return "Bad Gateway";
    default:
      return std::string();
  }
}

HttpMethod stringToMethod(const std::string& methodString) {
  std::string methodStringUppercase;
  std::transform(methodString.begin(), methodString.end(), std::back_inserter(methodStringUppercase), [](char c) { return toupper(c); });
  if (methodStringUppercase == "GET") {
    return HttpMethod::GET;
  } else if (methodStringUppercase == "HEAD") {
    return HttpMethod::HEAD;
  } else if (methodStringUppercase == "POST") {
    return HttpMethod::POST;
  } else if (methodStringUppercase == "PUT") {
    return HttpMethod::PUT;
  } else if (methodStringUppercase == "DELETE") {
    return HttpMethod::DELETE;
  } else if (methodStringUppercase == "CONNECT") {
    return HttpMethod::CONNECT;
  } else if (methodStringUppercase == "OPTIONS") {
    return HttpMethod::OPTIONS;
  } else if (methodStringUppercase == "TRACE") {
    return HttpMethod::TRACE;
  } else if (methodStringUppercase == "PATCH") {
    return HttpMethod::PATCH;
  } else {
    throw std::invalid_argument("Unexpected HTTP method");
  }
}

HttpVersion stringToVersion(const std::string& versionString) {
  std::string versionStringUppercase;
  std::transform(versionString.begin(), versionString.end(), std::back_inserter(versionStringUppercase), [](char c) { return toupper(c); });
  if (versionStringUppercase == "HTTP/0.9") {
    return HttpVersion::HTTP_0_9;
  } else if (versionStringUppercase == "HTTP/1.0") {
    return HttpVersion::HTTP_1_0;
  } else if (versionStringUppercase == "HTTP/1.1") {
    return HttpVersion::HTTP_1_1;
  } else if (versionStringUppercase == "HTTP/2" ||
             versionStringUppercase == "HTTP/2.0") {
    return HttpVersion::HTTP_2_0;
  } else {
    throw std::invalid_argument("Unexpected HTTP version");
  }
}

std::string toString(const HttpRequest& request) {
  std::ostringstream oss;

  oss << toString(request.method()) << ' ';
  oss << request.uri().path() << ' ';
  oss << toString(request.version()) << "\r\n";

  for (const auto& p : request.headers()) {
    oss << p.first << ": " << p.second << "\r\n";
  }

  oss << "\r\n";

  oss << request.content();

  return oss.str();
}

std::string toString(const HttpResponse& response, bool sendContent) {
  std::ostringstream oss;

  oss << toString(response.version()) << ' ';
  oss << static_cast<int>(response.statusCode()) << ' ';
  oss << toString(response.statusCode()) << "\r\n";

  for (const auto& p : response.headers()) {
    oss << p.first << ": " << p.second << "\r\n";
  }

  oss << "\r\n";

  if (sendContent) {
    oss << response.content();
  }

  return oss.str();
}

HttpRequest stringToRequest(const std::string& requestString) {
  std::string startLine, headerLines, messageBody;
  std::istringstream iss;
  HttpRequest request;
  std::string line, method, path, version;
  std::string key, value;
  Uri uri;
  size_t lpos = 0, rpos = 0;

  rpos = requestString.find("\r\n", lpos);
  if (rpos == std::string::npos) {
    throw std::invalid_argument("Could not find request start line");
  }
  startLine = requestString.substr(lpos, rpos - lpos);
  lpos = rpos + 2;

  rpos = requestString.find("\r\n\r\n", lpos);
  if (rpos != std::string::npos) {
    headerLines = requestString.substr(lpos, rpos - lpos);
    lpos = rpos + 4;
    rpos = requestString.length();
    if (lpos < rpos) {
      messageBody = requestString.substr(lpos, rpos - lpos);
    }
  }

  iss.clear();
  iss.str(startLine);
  iss >> method >> path >> version;
  if (!iss.good() && !iss.eof()) {
    throw std::invalid_argument("Invalid start line format");
  }

  request.setMethod(stringToMethod(method));
  request.setUri(Uri(path));
  if (stringToVersion(version) != request.version()) {
    throw std::logic_error("HTTP version not supported");
  }

  iss.clear();
  iss.str(headerLines);
  while (std::getline(iss, line)) {
    std::istringstream headerStream(line);
    std::getline(headerStream, key, ':');
    std::getline(headerStream, value);

    key.erase(std::remove_if(key.begin(), key.end(), [](char c) { return std::isspace(c); }), key.end());
    value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return std::isspace(c); }), value.end());
    request.setHeader(key, value);
  }

  request.setContent(messageBody);

  return request;
}

// Function not implemented since server primarily needs to generate responses rather than parse them
HttpResponse stringToResponse([[maybe_unused]] const std::string& responseString) {
  throw std::logic_error("Method not implemented");
}
}
