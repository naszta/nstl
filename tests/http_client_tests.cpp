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
