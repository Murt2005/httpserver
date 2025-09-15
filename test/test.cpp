#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <iterator>
#include <string>

#include "../src/httpMessage.h"
#include "../src/uri.h"

using namespace simpleHttpServer;

#define EXPECT_TRUE(x) { \
    if (!(x)) { \
        err++, std::cerr << __FUNCTION__ << " failed at line " << __LINE__ << std::endl; \
    } \
}

int err = 0;

void testUriPathToLowercase() {
    std::string path = "/SayHello.HTML?name=abc&message=welcome";
    std::string lowercasePath;
    std::transform(path.begin(), path.end(), std::back_inserter(lowercasePath), [](char c) { return tolower(c); });
    Uri uri("/SayHello.html?name=abc&message=welcome");
    EXPECT_TRUE(uri.path() == lowercasePath);
}

void testMethodToString() {
    EXPECT_TRUE(toString(HttpMethod::GET) == "GET");
  }
  
  void testVersionToString() {
    EXPECT_TRUE(toString(HttpVersion::HTTP_1_1) == "HTTP/1.1");
  }
  
  void testStatusCodeToString() {
    EXPECT_TRUE(toString(HttpStatusCode::ImATeapot) == "I'm a Teapot");
    EXPECT_TRUE(toString(HttpStatusCode::NoContent) == "");
  }
  
  void testStringToMethod() {
    EXPECT_TRUE(stringToMethod("GET") == HttpMethod::GET);
    EXPECT_TRUE(stringToMethod("post") == HttpMethod::POST);
  }
  
  void testStringToVersion() {
    EXPECT_TRUE(stringToVersion("HTTP/1.1") == HttpVersion::HTTP_1_1);
  }
  
  void testRequestToString() {
    HttpRequest request;
    request.setMethod(HttpMethod::GET);
    request.setUri(Uri("/"));
    request.setHeader("Connection", "Keep-Alive");
    request.setContent("hello, world\n");
  
    std::string expected_str;
    expected_str += "GET / HTTP/1.1\r\n";
    expected_str += "Connection: Keep-Alive\r\n";
    expected_str += "Content-Length: 13\r\n\r\n";
    expected_str += "hello, world\n";
  
    EXPECT_TRUE(toString(request) == expected_str);
  }
  
  void testResponseToString() {
    HttpResponse response;
    response.setStatusCode(HttpStatusCode::InternalServerError);
  
    std::string expected_str;
    expected_str += "HTTP/1.1 500 Internal Server Error\r\n\r\n";
  
    EXPECT_TRUE(toString(response) == expected_str);
  }
  
  int main(void) {
    std::cout << "Running tests..." << std::endl;
  
    testUriPathToLowercase();
    testMethodToString();
    testVersionToString();
    testStatusCodeToString();
    testStringToMethod();
    testStringToVersion();
    testRequestToString();
    testResponseToString();
  
    std::cout << "All tests have finished. There were " << err << " errors in total" << std::endl;
    return 0;
  }
  
