#ifndef _NSTL_HTTP_CLIENT
#define _NSTL_HTTP_CLIENT 1

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
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

// RFC 7231: 200 <= code < 300 is good
bool is_http_success(std::int32_t status_code_);
// is SSL supported
bool is_ssl_supported();

using HeaderLine = std::function<void(std::string_view line_)>;

struct ClientCbs
{
    void line(const char* data_, size_t size_) const;
    HeaderLine line_cb;
};

class Client
{
    static constexpr const size_t error_size = 256;
    std::array<char, error_size> _error;
    curl_ptr _curl;
    curl_headers_ptr _headers;
    ClientCbs _cbs;

    // less then 10 kB/s for more than 10 seconds
    long _bw_period{ 10 };
    long _bw_speed{ 10000 };

public:
    using duration = std::chrono::milliseconds;
    static constexpr duration default_timeout{ 10000 };

    explicit Client(bool verbose_ = false);
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    std::pair<std::int32_t, std::string> get(const char* url_, duration timeout_ = default_timeout,
                                             bool verify_ = true);
    std::pair<std::int32_t, std::string> post(const char* url_, duration timeout_ = default_timeout,
                                              bool verify_ = true);
    std::pair<std::int32_t, std::string> post(const char* url_, const std::string_view data_,
                                              duration timeout_ = default_timeout, bool verify_ = true);

    bool add_header(const char* header_);

    std::string url_encode(std::string_view data_) const;
    std::string url_decode(std::string_view data_) const;

    void reset();
    void reset_hdrs();

    std::string_view error_view() const;

    HeaderLine setHdrCb(HeaderLine cb_);

    void minimumBandwidth(std::chrono::seconds check_period_ = std::chrono::seconds::zero(),
                          std::uint32_t bandwidth_ = 0);

private:
    void _common(const char* url_, duration timeout_, bool verify_);
};
} // namespace nstl::http

namespace nstl::url
{
enum ResIdx : std::uint16_t
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
