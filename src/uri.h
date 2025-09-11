#ifndef URI_H_
#define URI_H_

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>


namespace httpServer {


class Uri {
public:
    Uri() = default;

    explicit Uri(const std::string& path)
        : path_(path)
    { SetPathToLowercase(); }

    ~Uri() = default;

    inline bool operator<(const Uri& other) const {
        return path_ < other.path_;
    }

    inline bool operator==(const Uri& other) const {
        return path_ == other.path_;
    }

    void setPath(const std::string& path) {
        path_ = std::move(path);
        SetPathToLowercase();
    }

    std::string scheme() const {
        return scheme_;
    }

    std::string host() const {
        return host_;
    }
    
    std::uint16_t port() const {
        return port_;
    }
    
    std::string path() const {
        return path_;
    }

private:
    std::string path_;
    std::string scheme_;
    std::string host_;
    std::uint16_t port_;

    void SetPathToLowercase() {
        std::transform(path_.begin(), path_.end(), path_.begin(), [](char c) { return tolower(c); });
    }
};


};

#endif
