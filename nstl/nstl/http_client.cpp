#include "http_client.hpp"
#include "compiler.hpp"
#include "exception.hpp"
#include "logging.hpp"
#include "scope_exit.hpp"
#include "string.hpp"

#include <curl/curl.h>

#include <cstring>
#include <utility>

#define NSTL_CURL_CHECK(command)                                                             \
    do                                                                                       \
    {                                                                                        \
        const auto _curl_res = command;                                                      \
        if (_curl_res != CURLE_OK) [[unlikely]]                                              \
        {                                                                                    \
            NSTL2_THROW_EXCEPTION(#command << " failed: " << ::curl_easy_strerror(_curl_res) \
                                           << "; error: " << this->error_view());            \
        }                                                                                    \
    } while (false)

namespace nstl::http
{
namespace
{
size_t writeFunction(const char* ptr, size_t size, size_t nmemb, std::string* data)
{
    if (ptr && data) [[likely]]
    {
        data->append(ptr, size * nmemb);
        return size * nmemb;
    }
    return 0;
}

size_t headerFunction(const char* ptr, size_t size, size_t nmemb, ClientCbs* data)
{
    if (ptr && data) [[likely]]
    {
        data->line(ptr, size * nmemb);
        return size * nmemb;
    }
    return 0;
}

int my_trace(CURL* /* curl*/, const curl_infotype type, const char* data, size_t size, void* /* userp*/)
{
    const std::string_view data_view_base{ data, size };
    const auto data_view = right_trim_view(data_view_base);
    NSTL_WRN_SWITCH_ENUM_PUSH
    switch (type)
    {
    case CURLINFO_TEXT:
        NSTL_INFO("CURL" << log::delimiter << data_view);
        return 0;
    case CURLINFO_HEADER_IN:
        NSTL_DEBUG("CURL" << log::delimiter << " <= header recv - " << data_view);
        return 0;
    case CURLINFO_HEADER_OUT:
        NSTL_DEBUG("CURL" << log::delimiter << " => header send - " << data_view);
        return 0;
    case CURLINFO_DATA_IN:
        NSTL_DEBUG("CURL" << log::delimiter << " <= data recv");
        return 0;
    case CURLINFO_DATA_OUT:
        NSTL_DEBUG("CURL" << log::delimiter << " => data send");
        return 0;
    case CURLINFO_SSL_DATA_IN:
        NSTL_DEBUG("CURL" << log::delimiter << " <= ssl data recv");
        return 0;
    case CURLINFO_SSL_DATA_OUT:
        NSTL_DEBUG("CURL" << log::delimiter << " => ssl data send");
        return 0;
    default:
        return 0;
    }
    NSTL_WRN_SWITCH_ENUM_POP
}
} // namespace

void ClientCbs::line(const char* data_, size_t size_) const
{
    if (!line_cb)
    {
        return;
    }
    const auto hdr_line = right_trim_view(std::string_view{ data_, size_ });
    if (hdr_line.empty()) [[unlikely]]
    {
        return;
    }
    line_cb(hdr_line);
}

void CurlDeleter::operator()(CURL* curl_) const { ::curl_easy_cleanup(curl_); }

void CurlListDeleter::operator()(curl_slist* ptr_) const { ::curl_slist_free_all(ptr_); }

Client::Client(const bool verbose_) : _curl{ ::curl_easy_init() }
{
    static_assert(CURL_ERROR_SIZE <= error_size, "Error size should be at least CURL_ERROR_SIZE");
    std::memset(_error.data(), 0, _error.size());
    NSTL2_THROW_EXCEPTION_IF(!_curl, "curl_easy_init failed");
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_NOSIGNAL, 1L));               // no signal
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_ERRORBUFFER, _error.data())); // error buffer
    if (verbose_)
    {
        NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_DEBUGFUNCTION, my_trace));
        NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_VERBOSE, 1L));
    }
}

Client::~Client() = default;

bool is_http_success(const std::int32_t status_code_) { return 200 <= status_code_ && status_code_ < 300; }

bool is_ssl_supported()
{
    const auto version = ::curl_version_info(CURLVERSION_NOW);
    return version->features & CURL_VERSION_SSL;
}

void Client::_common(const char* url_, const duration timeout_, const bool verify_)
{
    NSTL2_THROW_EXCEPTION_IF(!url_, "URL pointer is nullptr!");
    _error[0] = '\0';
    if (duration::zero() < timeout_)
    {
        NSTL_CURL_CHECK(
            ::curl_easy_setopt(_curl.get(), CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(timeout_.count())););
        NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_TIMEOUT_MS, static_cast<long>(timeout_.count())););
    }

    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_LOW_SPEED_TIME, _bw_period));
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_LOW_SPEED_LIMIT, _bw_speed));

    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_TCP_KEEPALIVE, 1L));
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_TCP_KEEPIDLE, 30L));
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_TCP_KEEPINTVL, 10L));
#if (8 < LIBCURL_VERSION_MAJOR) || ((8 == LIBCURL_VERSION_MAJOR) && (9 <= LIBCURL_VERSION_MINOR))
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_TCP_KEEPCNT, 3L));
#endif
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_WRITEFUNCTION, writeFunction));
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_HEADERFUNCTION, headerFunction));
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_URL, url_));
    if (_headers)
    {
        NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_HTTPHEADER, _headers.get()));
    }
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_SSL_VERIFYPEER, verify_ ? 1L : 0L));
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_FOLLOWLOCATION, 1L));
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_MAXREDIRS, 10L));
}

std::pair<std::int32_t, std::string> Client::get(const char* url_, const duration timeout_, const bool verify_)
{
    this->_common(url_, timeout_, verify_);
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_HTTPGET, 1L));
    std::string retval;
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_WRITEDATA, &retval));
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_HEADERDATA, &_cbs));
    long http_code = 0;
    NSTL_CURL_CHECK(::curl_easy_perform(_curl.get()));
    NSTL_CURL_CHECK(::curl_easy_getinfo(_curl.get(), CURLINFO_RESPONSE_CODE, &http_code));
    return std::make_pair(static_cast<std::int32_t>(http_code), std::move(retval));
}

std::pair<std::int32_t, std::string> Client::post(const char* url_, const duration timeout_, const bool verify_)
{
    return this->post(url_, std::string_view{}, timeout_, verify_);
}

std::pair<std::int32_t, std::string> Client::post(const char* url_, const std::string_view data_,
                                                  const duration timeout_, const bool verify_)
{
    this->_common(url_, timeout_, verify_);
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_POST, 1L));
    if (!data_.empty())
    {
        NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_POSTFIELDSIZE, data_.size()));
        NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_POSTFIELDS, data_.data()));
    }
    std::string retval;
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_WRITEDATA, &retval));
    NSTL_CURL_CHECK(::curl_easy_setopt(_curl.get(), CURLOPT_HEADERDATA, &_cbs));

    long http_code = 0;
    NSTL_CURL_CHECK(::curl_easy_perform(_curl.get()));
    NSTL_CURL_CHECK(::curl_easy_getinfo(_curl.get(), CURLINFO_RESPONSE_CODE, &http_code));
    return std::make_pair(static_cast<std::int32_t>(http_code), std::move(retval));
}

bool Client::add_header(const char* header_)
{
    NSTL2_THROW_EXCEPTION_IF(!header_, "header is nullptr");
    auto chunk = ::curl_slist_append(_headers.get(), header_);
    NSTL2_THROW_EXCEPTION_IF(!chunk, "curl_slist_append failed");
    _headers.release();
    _headers.reset(std::exchange(chunk, nullptr));
    return true;
}

std::string Client::url_encode(const std::string_view data_) const
{
    if (data_.empty())
    {
        return std::string{};
    }
    const auto encoded = ::curl_easy_escape(_curl.get(), data_.data(), static_cast<int>(data_.size()));
    NSTL2_THROW_EXCEPTION_IF(!encoded, data_ << " failed to be encoded by curl_easy_escape");
    const auto cleaner = on_scope_exit(
        [encoded]()
        {
            if (encoded)
            {
                ::curl_free(encoded);
            }
        });
    return std::string{ encoded };
}

std::string Client::url_decode(const std::string_view data_) const
{
    if (data_.empty())
    {
        return std::string{};
    }
    int out_len = 0;
    const auto decoded = ::curl_easy_unescape(_curl.get(), data_.data(), static_cast<int>(data_.size()), &out_len);
    NSTL2_THROW_EXCEPTION_IF(!decoded, data_ << " failed to be decoded by curl_easy_unescape");
    const auto cleaner = on_scope_exit(
        [decoded]()
        {
            if (decoded)
            {
                ::curl_free(decoded);
            }
        });
    return std::string{ decoded, static_cast<size_t>(out_len) };
}

std::string_view Client::error_view() const
{
    return std::string_view{ _error.data(), strnlen(_error.data(), _error.size()) };
}

void Client::reset() { ::curl_easy_reset(_curl.get()); }
void Client::reset_hdrs() { _headers.reset(); }

HeaderLine Client::setHdrCb(HeaderLine cb_) { return std::exchange(_cbs.line_cb, std::move(cb_)); }

void Client::minimumBandwidth(const std::chrono::seconds check_period_, const std::uint32_t bandwidth_)
{
    NSTL2_THROW_EXCEPTION_IF(check_period_ < std::chrono::seconds::zero(), "Check period cannot be negative");
    if (check_period_ == std::chrono::seconds::zero() || bandwidth_ == 0)
    {
        _bw_period = 0;
        _bw_speed = 0;
    }
    else
    {
        _bw_period = static_cast<long>(check_period_.count());
        _bw_speed = static_cast<long>(bandwidth_);
    }
}

} // namespace nstl::http

namespace nstl::url
{
namespace
{
const std::regex& reg_item()
{
    static const std::regex url_reg{ R"(^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)" };
    return url_reg;
}
} // namespace

bool is_valid_url(const std::string_view url_) { return std::regex_match(url_.cbegin(), url_.cend(), reg_item()); }

bool is_valid_url(std::string_view url_, view_results& result_)
{
    return std::regex_match(url_.cbegin(), url_.cend(), result_, reg_item());
}

} // namespace nstl::url
