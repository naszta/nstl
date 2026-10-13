#include <gtest/gtest.h>

#include <nstl/datahash.hpp>

#include <array>
#include <filesystem>
#include <span>

namespace fs = std::filesystem;

TEST(HashTest, SimpleDefault)
{
    const auto hasher = nstl::Hasher::factory();
    hasher->add("Example payload");
    const auto value = hasher->finish();
    const auto readable = nstl::hash_to_hex(value);
    EXPECT_EQ(readable, "E2CFEB333F551852EFE9783F514B9420D4EA77DAFC9B34BE677075818B5366BE");
}

TEST(HashTest, SimpleMD5)
{
    const auto hasher = nstl::Hasher::factory(nstl::HashType::MD5);
    hasher->add("Example payload");
    const auto value = hasher->finish();
    const auto readable = nstl::hash_to_hex(value);
    EXPECT_EQ(readable, "4A11CA54F9E6CD7A639058DACDA62724");
}

TEST(HashTest, SimpleSHA1)
{
    const auto hasher = nstl::Hasher::factory(nstl::HashType::SHA1);
    hasher->add("Example payload");
    const auto value = hasher->finish();
    const auto readable = nstl::hash_to_hex(value);
    EXPECT_EQ(readable, "881CFA4411D417C8039A8E436991F6DCFA53AFE9");
}

TEST(HashTest, SimpleSHA256)
{
    const auto hasher = nstl::Hasher::factory(nstl::HashType::SHA256);
    hasher->add("Example payload");
    const auto value = hasher->finish();
    const auto readable = nstl::hash_to_hex(value);
    EXPECT_EQ(readable, "E2CFEB333F551852EFE9783F514B9420D4EA77DAFC9B34BE677075818B5366BE");
}

TEST(HashTest, SimpleSHA512)
{
    const auto hasher = nstl::Hasher::factory(nstl::HashType::SHA512);
    hasher->add("Example payload");
    const auto value = hasher->finish();
    const auto readable = nstl::hash_to_hex(value);
    EXPECT_EQ(readable, "321D982DD230977BC36BCC2D3CB9A1496C707A3544B43BBC9A95D64DDD55BB08EA0BDC9D59E0D541CDF25FEF230C19"
                        "55B065AABBE0BF39F1B83B747134599EBB");
}

TEST(HashTest, ResetTest)
{
    constexpr std::string_view test_payload{ "Other payload" };
    const auto hasher0 = nstl::Hasher::factory();
    hasher0->add("Example payload");
    hasher0->reset();
    hasher0->add(test_payload);
    const auto hash0 = hasher0->finish();

    const auto hasher1 = nstl::Hasher::factory();
    hasher1->add(test_payload);
    const auto hash1 = hasher1->finish();

    EXPECT_EQ(hash0, hash1) << "Reset failed!";
}

namespace
{
fs::path test_data_path()
{
    const fs::path src_file{ __FILE__ };
    return src_file.parent_path() / "data";
}
} // namespace

TEST(HashTest, FileTest)
{
    const auto test_file = test_data_path() / "IMG_9999.jpeg";
    const auto file_hash = nstl::hash_file(test_file);
    const auto readable = nstl::hash_to_hex(file_hash);
    EXPECT_EQ(readable, "2A93D4C9ED0E96E7C10413AB4FF075ABB22EC72D8B95B602AE64A7FEC5DC9B2D");

    const auto file_hash2 = nstl::hash_file(test_file, nstl::HashType::SHA256, 1024);
    const auto readable2 = nstl::hash_to_hex(file_hash2);
    EXPECT_EQ(readable, readable2);
}

#ifdef __cpp_lib_span
TEST(HashTest, FileTestSpanBuffer)
{
    const auto test_file = test_data_path() / "IMG_9999.jpeg";
    std::array<char, 1024> buffer;
    const auto file_hash = nstl::hash_file(test_file, std::span{ buffer }, nstl::HashType::SHA256);
    const auto readable = nstl::hash_to_hex(file_hash);
    EXPECT_EQ(readable, "2A93D4C9ED0E96E7C10413AB4FF075ABB22EC72D8B95B602AE64A7FEC5DC9B2D");
}

TEST(HashTest, FileTestSpanBufferEmptyThrows)
{
    const auto test_file = test_data_path() / "IMG_9999.jpeg";
    EXPECT_THROW(nstl::hash_file(test_file, std::span<char>{}, nstl::HashType::SHA256), std::exception);
}
#endif

TEST(HashTest, FileTestEmptyPathThrows) { EXPECT_THROW(nstl::hash_file(fs::path{}), std::exception); }

TEST(HashTest, FileTestMissingFileThrows)
{
    EXPECT_THROW(nstl::hash_file(test_data_path() / "does_not_exist.bin"), std::exception);
}

TEST(HashTest, FileTestZeroBufferSizeThrows)
{
    const auto test_file = test_data_path() / "IMG_9999.jpeg";
    EXPECT_THROW(nstl::hash_file(test_file, nstl::HashType::SHA256, 0), std::exception);
}

TEST(HashTest, ParseHashType)
{
    EXPECT_EQ(nstl::parseHashType("MD5"), nstl::HashType::MD5);
    EXPECT_EQ(nstl::parseHashType("SHA"), nstl::HashType::SHA1);
    EXPECT_EQ(nstl::parseHashType("SHA1"), nstl::HashType::SHA1);
    EXPECT_EQ(nstl::parseHashType("SHA256"), nstl::HashType::SHA256);
    EXPECT_EQ(nstl::parseHashType("SHA512"), nstl::HashType::SHA512);
    EXPECT_EQ(nstl::parseHashType("Default"), nstl::HashType::Default);
    EXPECT_FALSE(nstl::parseHashType("").has_value());
    EXPECT_THROW(nstl::parseHashType("not-a-hash-type"), std::exception);
}

TEST(HashTest, FactoryInvalidTypeThrows)
{
    EXPECT_THROW(nstl::Hasher::factory(static_cast<nstl::HashType>(0)), std::exception);
}

TEST(HashTest, AddSpanOverload)
{
    constexpr std::string_view payload{ "Example payload" };
    const std::span<const char> view{ payload.data(), payload.size() };

    const auto hasher0 = nstl::Hasher::factory();
    hasher0->add(view);
    const auto hash0 = nstl::hash_to_hex(hasher0->finish());

    const auto hasher1 = nstl::Hasher::factory();
    hasher1->add(payload.data(), payload.size());
    const auto hash1 = nstl::hash_to_hex(hasher1->finish());

    EXPECT_EQ(hash0, hash1);
}

TEST(HashTest, AddWideString)
{
    const auto hasher0 = nstl::Hasher::factory();
    hasher0->add(L"Example payload");
    const auto hash0 = nstl::hash_to_hex(hasher0->finish());
    EXPECT_FALSE(hash0.empty());
}

TEST(HashTest, AddNullptrIsNoop)
{
    const auto hasher0 = nstl::Hasher::factory();
    hasher0->add(static_cast<const char*>(nullptr));
    const auto hash0 = nstl::hash_to_hex(hasher0->finish());

    const auto hasher1 = nstl::Hasher::factory();
    const auto hash1 = nstl::hash_to_hex(hasher1->finish());

    EXPECT_EQ(hash0, hash1);

    const auto hasher2 = nstl::Hasher::factory();
    hasher2->add(static_cast<const wchar_t*>(nullptr));
    const auto hash2 = nstl::hash_to_hex(hasher2->finish());

    EXPECT_EQ(hash0, hash2);
}
