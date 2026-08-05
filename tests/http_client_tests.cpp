#include <nstl/http_client.hpp>
#include <nstl/range_print.hpp>

#include <gtest/gtest.h>

TEST(HttpClient, ClientTest)
{
    ASSERT_TRUE(nstl::http::is_ssl_supported());
    nstl::http::Client client{ true };
    const auto [code, text] = client.get("https://google.com");
    EXPECT_TRUE(nstl::http::is_http_success(code)) << code << " HTTP code is invalid";
    EXPECT_FALSE(text.empty());
    EXPECT_TRUE(client.error_view().empty());
}

TEST(HttpClient, UrlTest)
{
    const std::string_view my_url{ "https://naszta.london/path/file?key=value&key2=value" };
    nstl::url::view_results results;
    ASSERT_TRUE(nstl::url::is_valid_url(my_url, results));
    ASSERT_GT(results.size(), nstl::url::Params);
    EXPECT_EQ(results[nstl::url::Protocol].compare("https"), 0) << results[nstl::url::Protocol].str() << " received";
    EXPECT_EQ(results[nstl::url::Hostname].compare("naszta.london"), 0)
        << results[nstl::url::Hostname].str() << " received";
    EXPECT_EQ(results[nstl::url::Path].compare("/path/file"), 0) << results[nstl::url::Path].str() << " received";
    EXPECT_EQ(results[nstl::url::Params].compare("key=value&key2=value"), 0)
        << results[nstl::url::Params].str() << " received";
}

TEST(HttpClient, UrlTestNoQueryString)
{
    const std::string_view my_url{ "https://naszta.london/path/file" };
    nstl::url::view_results results;
    ASSERT_TRUE(nstl::url::is_valid_url(my_url, results));
    ASSERT_GT(results.size(), nstl::url::Path);
    EXPECT_EQ(results[nstl::url::Protocol].compare("https"), 0);
    EXPECT_EQ(results[nstl::url::Hostname].compare("naszta.london"), 0);
    EXPECT_EQ(results[nstl::url::Path].compare("/path/file"), 0);
    EXPECT_FALSE(results[nstl::url::Params].matched);
}

TEST(HttpClient, UrlTestSingleArgOverload) { EXPECT_TRUE(nstl::url::is_valid_url("https://naszta.london/path")); }

TEST(HttpClient, UrlEncodeDecode)
{
    nstl::http::Client client;
    const std::string_view raw{ "a b/c?d=e" };
    const auto encoded = client.url_encode(raw);
    EXPECT_NE(encoded, raw);
    const auto decoded = client.url_decode(encoded);
    EXPECT_EQ(decoded, raw);

    EXPECT_TRUE(client.url_encode(std::string_view{}).empty());
    EXPECT_TRUE(client.url_decode(std::string_view{}).empty());
}

TEST(HttpClient, AddHeaderAndReset)
{
    nstl::http::Client client;
    EXPECT_TRUE(client.add_header("X-Test: value"));
    EXPECT_NO_THROW(client.reset());
}

TEST(HttpClient, IsHttpSuccessBoundaries)
{
    EXPECT_FALSE(nstl::http::is_http_success(199));
    EXPECT_TRUE(nstl::http::is_http_success(200));
    EXPECT_TRUE(nstl::http::is_http_success(299));
    EXPECT_FALSE(nstl::http::is_http_success(300));
}

TEST(HttpClient, GetNullptrUrlThrows)
{
    nstl::http::Client client;
    EXPECT_THROW(client.get(nullptr), std::exception);
}
