#include <nstl/http_client.hpp>

#include <gtest/gtest.h>

TEST(HttpClient, ClientTest)
{
    nstl::HttpClient client;
    ASSERT_TRUE(client.is_ssl_supported());
    const auto [code, text] = client.get("https://google.com");
    EXPECT_TRUE(client.is_http_success(code)) << code << " HTTP code is invalid";
    EXPECT_FALSE(text.empty());
    EXPECT_TRUE(client.error_view().empty());
}
