#ifndef _NSTL_HTTP_CLIENT
#define _NSTL_HTTP_CLIENT 1

#include <array>
#include <cstdint>
#include <memory>
#include <regex>
#include <string>
#include <string_view>

extern "C"
{
    using CURL = void;
    struct curl_slist;
}

namespace nstl::http
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

// success between 200 <= and < 300
bool is_http_success(std::int32_t status_code_);
// is SSL supported
bool is_ssl_supported();

class Client
{
    static constexpr const size_t error_size = 256;
    std::array<char, error_size> _error;
    curl_ptr _curl;
    curl_headers_ptr _headers;

    void _common(const char* url_, bool verify_);

public:
    explicit Client(bool verbose_ = false);
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    std::pair<std::int32_t, std::string> get(const char* url_, bool verify_ = true);
    std::pair<std::int32_t, std::string> post(const char* url_, bool verify_ = true);
    std::pair<std::int32_t, std::string> post(const char* url_, const std::string_view data_, bool verify_ = true);

    bool add_header(const char* header_);

    std::string url_encode(std::string_view data_) const;
    std::string url_decode(std::string_view data_) const;

    void reset();

    std::string_view error_view() const;
};
} // namespace nstl::http

namespace nstl::url
{
enum ResIdx : std::uint32_t
{
    Protocol = 2,
    Hostname = 4,
    Path = 5,
    Params = 7,
};
// RFC 3986 (page 50-51)
using view_results = std::match_results<std::string_view::const_iterator>;
bool is_valid_url(std::string_view url_);
bool is_valid_url(std::string_view url_, view_results& result_);
} // namespace nstl::url

#endif
