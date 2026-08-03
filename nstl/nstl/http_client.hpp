#ifndef _NSTL_HTTP_CLIENT
#define _NSTL_HTTP_CLIENT 1

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

extern "C"
{
    using CURL = void;
    struct curl_slist;
}

namespace nstl
{
struct CurlDeleter
{
    void operator()(CURL* curl_) const;
};

using curl_ptr = std::unique_ptr<CURL, CurlDeleter>;

struct CurlListDeleter
{
    void operator()(curl_slist* ptr_) const;
};

using curl_headers_ptr = std::unique_ptr<curl_slist, CurlListDeleter>;

class HttpClient
{
    static constexpr const size_t error_size = 256;
    std::array<char, error_size> _error;
    curl_ptr _curl;
    curl_headers_ptr _headers;

    void _common(const char* url_, bool verify_);

public:
    // success between 200 <= and < 300
    static bool is_http_success(std::int32_t status_code_);
    // RFC 3986 (page 50-51)
    static bool is_valid_url(std::string_view url_);
    // is SSL supported
    static bool is_ssl_supported();

    explicit HttpClient(bool verbose_ = false);
    ~HttpClient();
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    std::pair<std::int32_t, std::string> get(const char* url_, bool verify_ = true);
    std::pair<std::int32_t, std::string> post(const char* url_, bool verify_ = true);
    std::pair<std::int32_t, std::string> post(const char* url_, const std::string_view data_, bool verify_ = true);

    bool add_header(const char* header_);

    std::string url_encode(std::string_view data_) const;
    std::string url_decode(std::string_view data_) const;

    void reset();

    std::string_view error_view() const;
};
} // namespace nstl

#endif
