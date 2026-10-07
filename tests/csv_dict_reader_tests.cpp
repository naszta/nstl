#include <nstl/csv_dict_reader.hpp>
#include <nstl/exception.hpp>

#include <gtest/gtest.h>

#include <sstream>

using nstl::csv::csv_dict_reader;
using nstl::csv::csv_parse_config;
using nstl::csv::values_mapping;

TEST(CsvReader, BasicTest)
{
    const std::vector<std::string> expected{ "one", "two", "three" };
    std::vector<std::string> results;
    constexpr std::string_view input{ "one,\"two\",three" };
    EXPECT_EQ(nstl::csv::parse_csv_line(input,
        [&results](const std::string& val_) {
            results.push_back(val_);
        }), 3U);
    EXPECT_EQ(results, expected);
}

TEST(CsvReader, ParseLineQuotedComaAndEscape)
{
    const std::vector<std::string> expected{ "a,b", "say \"hi\"", "back\\slash", "" };
    std::vector<std::string> results;
    constexpr std::string_view input{ R"("a,b","say \"hi\"","back\\slash",)" };
    EXPECT_EQ(nstl::csv::parse_csv_line(input, [&results](const std::string& val_) { results.push_back(val_); }), 4U);
    EXPECT_EQ(results, expected);
}

TEST(CsvReader, ParseLineCustomSeparator)
{
    const std::vector<std::string> expected{ "a", "b;c", "d" };
    std::vector<std::string> results;
    const nstl::csv::csv_separator sep{ '\\', ';', '\'' };
    EXPECT_EQ(nstl::csv::parse_csv_line("a;'b;c';d", [&results](const std::string& val_) { results.push_back(val_); },
                                        sep),
              3U);
    EXPECT_EQ(results, expected);
}

TEST(CsvReader, ParseLineEmptyFunctorThrows)
{
    EXPECT_THROW(nstl::csv::parse_csv_line("a,b", nstl::csv::result_func{}), nstl::exception);
}

TEST(CsvReader, HeaderParsed)
{
    std::istringstream iss{ "name,age,city\n" };
    const csv_dict_reader reader{ iss };

    const auto header = reader.header();
    ASSERT_TRUE(header);
    ASSERT_EQ(header->size(), 3U);
    EXPECT_EQ(header->at("name"), 0U);
    EXPECT_EQ(header->at("age"), 1U);
    EXPECT_EQ(header->at("city"), 2U);
    EXPECT_FALSE(reader.row());
}

TEST(CsvReader, NoHeaderWhenDisabled)
{
    std::istringstream iss{ "a,b\nc,d\n" };
    csv_dict_reader reader{ iss, csv_parse_config{ .parse_headers = false } };

    EXPECT_FALSE(reader.header());
    ASSERT_TRUE(reader.next());
    ASSERT_TRUE(reader.row());
    EXPECT_EQ(*reader.row(), (std::vector<std::string>{ "a", "b" }));
    EXPECT_FALSE(reader.get_field("a").has_value());
    ASSERT_TRUE(reader.next());
    EXPECT_EQ(*reader.row(), (std::vector<std::string>{ "c", "d" }));
    EXPECT_FALSE(reader.next());
    EXPECT_FALSE(reader.row());
}

TEST(CsvReader, EmptyStreamThrows)
{
    std::istringstream iss{ "" };
    EXPECT_THROW(csv_dict_reader{ iss }, nstl::exception);
}

TEST(CsvReader, EmptyStreamWithoutHeaders)
{
    std::istringstream iss{ "" };
    csv_dict_reader reader{ iss, csv_parse_config{ .parse_headers = false } };
    EXPECT_FALSE(reader.next());
}

TEST(CsvReader, DuplicatedColumnThrows)
{
    std::istringstream iss{ "a,b,a\n1,2,3\n" };
    EXPECT_THROW(csv_dict_reader{ iss }, nstl::exception);
}

TEST(CsvReader, IterateRowsAndFields)
{
    std::istringstream iss{ "name,age\nalice,30\n\"bob, jr\",41\n" };
    csv_dict_reader reader{ iss };

    EXPECT_FALSE(reader.get_field("name").has_value());

    ASSERT_TRUE(reader.next());
    EXPECT_EQ(reader.get_field("name"), "alice");
    EXPECT_EQ(reader.get_field("age"), "30");
    EXPECT_FALSE(reader.get_field("missing").has_value());

    ASSERT_TRUE(reader.next());
    EXPECT_EQ(reader.get_field("name"), "bob, jr");
    EXPECT_EQ(reader.get_field("age"), "41");

    EXPECT_FALSE(reader.next());
    EXPECT_FALSE(reader.get_field("name").has_value());
}

TEST(CsvReader, NoTrailingNewline)
{
    std::istringstream iss{ "a\n1" };
    csv_dict_reader reader{ iss };
    ASSERT_TRUE(reader.next());
    EXPECT_EQ(reader.get_field("a"), "1");
    EXPECT_FALSE(reader.next());
}

TEST(CsvReader, GetFieldWithExternalColumns)
{
    std::istringstream iss{ "x,y,z\n" };
    csv_dict_reader reader{ iss, csv_parse_config{ .parse_headers = false } };
    ASSERT_TRUE(reader.next());

    const nstl::csv::column_mapping columns{ { "first", 0 }, { "third", 2 }, { "out_of_range", 5 } };
    EXPECT_EQ(reader.get_field(columns, "first"), "x");
    EXPECT_EQ(reader.get_field(columns, "third"), "z");
    EXPECT_FALSE(reader.get_field(columns, "out_of_range").has_value());
    EXPECT_FALSE(reader.get_field(columns, "unknown").has_value());
}

TEST(CsvReader, SkipsEmptyLinesAndComments)
{
    std::istringstream iss{ "a,b\n\n# comment line\n1,2\n\n#another\n3,4\n" };
    csv_dict_reader reader{ iss };

    ASSERT_TRUE(reader.next());
    EXPECT_EQ(*reader.row(), (std::vector<std::string>{ "1", "2" }));
    ASSERT_TRUE(reader.next());
    EXPECT_EQ(*reader.row(), (std::vector<std::string>{ "3", "4" }));
    EXPECT_FALSE(reader.next());
}

TEST(CsvReader, CommentsDisabled)
{
    std::istringstream iss{ "a,b\n#1,2\n" };
    csv_dict_reader reader{ iss, csv_parse_config{ .comment = '\0' } };

    ASSERT_TRUE(reader.next());
    EXPECT_EQ(reader.get_field("a"), "#1");
    EXPECT_EQ(reader.get_field("b"), "2");
    EXPECT_FALSE(reader.next());
}

TEST(CsvReader, CustomCommentChar)
{
    std::istringstream iss{ "a\n;skipped\n#kept\n" };
    csv_dict_reader reader{ iss, csv_parse_config{ .comment = ';' } };

    ASSERT_TRUE(reader.next());
    EXPECT_EQ(reader.get_field("a"), "#kept");
    EXPECT_FALSE(reader.next());
}

TEST(CsvReader, EmptyLinesKeptWhenNotSkipped)
{
    std::istringstream iss{ "1\n\n2\n" };
    csv_dict_reader reader{ iss, csv_parse_config{ .parse_headers = false, .skip_empty_lines = false } };

    ASSERT_TRUE(reader.next());
    EXPECT_EQ(*reader.row(), (std::vector<std::string>{ "1" }));
    ASSERT_TRUE(reader.next());
    EXPECT_TRUE(reader.row()->empty());
    ASSERT_TRUE(reader.next());
    EXPECT_EQ(*reader.row(), (std::vector<std::string>{ "2" }));
    EXPECT_FALSE(reader.next());
}

TEST(CsvReader, ColumnCountMismatchThrows)
{
    std::istringstream iss{ "a,b\n1,2,3\n" };
    csv_dict_reader reader{ iss };
    EXPECT_THROW(reader.next(), nstl::exception);
}

TEST(CsvReader, ColumnCountMismatchAllowed)
{
    std::istringstream iss{ "a,b,c\n1,2\n1,2,3,4\n" };
    csv_dict_reader reader{ iss, csv_parse_config{ .check_columns_size = false } };

    ASSERT_TRUE(reader.next());
    EXPECT_EQ(reader.get_field("a"), "1");
    EXPECT_EQ(reader.get_field("b"), "2");
    EXPECT_FALSE(reader.get_field("c").has_value());

    ASSERT_TRUE(reader.next());
    EXPECT_EQ(reader.row()->size(), 4U);
    EXPECT_EQ(reader.get_field("c"), "3");
    EXPECT_FALSE(reader.next());
}

TEST(CsvReader, CustomSeparatorConfig)
{
    std::istringstream iss{ "a;b\n'x;y';z\n" };
    csv_dict_reader reader{ iss, csv_parse_config{ .coma = ';', .quote = '\'' } };

    ASSERT_TRUE(reader.next());
    EXPECT_EQ(reader.get_field("a"), "x;y");
    EXPECT_EQ(reader.get_field("b"), "z");
}

TEST(CsvReader, GetDataMapping)
{
    std::istringstream iss{ "name,age\nalice,30\nbob,41\n" };
    csv_dict_reader reader{ iss };

    values_mapping data;
    ASSERT_TRUE(reader.get_data(data));
    EXPECT_EQ(data, (values_mapping{ { "name", "alice" }, { "age", "30" } }));
    ASSERT_TRUE(reader.get_data(data));
    EXPECT_EQ(data, (values_mapping{ { "name", "bob" }, { "age", "41" } }));
    EXPECT_FALSE(reader.get_data(data));
}

TEST(CsvReader, GetDataOptional)
{
    std::istringstream iss{ "k,v\n1,2\n" };
    csv_dict_reader reader{ iss };

    const auto data = reader.get_data();
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(*data, (values_mapping{ { "k", "1" }, { "v", "2" } }));
    EXPECT_FALSE(reader.get_data().has_value());
}

TEST(CsvReader, GetDataSkipsMissingFields)
{
    std::istringstream iss{ "a,b,c\n1,2\n" };
    csv_dict_reader reader{ iss, csv_parse_config{ .check_columns_size = false } };

    const auto data = reader.get_data();
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(*data, (values_mapping{ { "a", "1" }, { "b", "2" } }));
}

TEST(CsvReader, GetDataWithoutHeadersThrows)
{
    std::istringstream iss{ "1,2\n" };
    csv_dict_reader reader{ iss, csv_parse_config{ .parse_headers = false } };
    values_mapping data;
    EXPECT_THROW(reader.get_data(data), nstl::exception);
    EXPECT_THROW(reader.get_data(), nstl::exception);
}

TEST(CsvWriter, WriteFieldPlain)
{
    std::ostringstream oss;
    nstl::csv::write_field(oss, "plain text");
    EXPECT_EQ(oss.str(), "plain text");
}

TEST(CsvWriter, WriteFieldQuoted)
{
    std::ostringstream oss;
    nstl::csv::write_field(oss, "a,b");
    EXPECT_EQ(oss.str(), "\"a,b\"");
}

TEST(CsvWriter, WriteFieldEscapes)
{
    std::ostringstream oss;
    nstl::csv::write_field(oss, R"(say "hi" \o/)");
    EXPECT_EQ(oss.str(), R"("say \"hi\" \\o/")");
}

TEST(CsvWriter, WriteFieldEmpty)
{
    std::ostringstream oss;
    nstl::csv::write_field(oss, "");
    EXPECT_EQ(oss.str(), "");
}

TEST(CsvWriter, WriteRow)
{
    const std::vector<std::string> row{ "a", "b,c", "" };
    std::ostringstream oss;
    nstl::csv::write_row(oss, row);
    EXPECT_EQ(oss.str(), "a,\"b,c\",");
}

TEST(CsvWriter, WriteRowEmpty)
{
    std::ostringstream oss;
    nstl::csv::write_row(oss, std::span<const std::string>{});
    EXPECT_EQ(oss.str(), "");
}

TEST(CsvWriter, WriteRowCustomConfig)
{
    const std::vector<std::string> row{ "x;y", "z" };
    std::ostringstream oss;
    nstl::csv::write_row(oss, row, csv_parse_config{ .coma = ';', .quote = '\'' });
    EXPECT_EQ(oss.str(), "'x;y';z");
}

TEST(CsvWriter, RoundTrip)
{
    const std::vector<std::string> header{ "name", "note" };
    const std::vector<std::string> row{ "x,y", R"(quote " and \ backslash)" };

    std::stringstream ss;
    nstl::csv::write_row(ss, header) << '\n';
    nstl::csv::write_row(ss, row) << '\n';

    csv_dict_reader reader{ ss };
    ASSERT_TRUE(reader.next());
    EXPECT_EQ(*reader.row(), row);
    EXPECT_FALSE(reader.next());
}
