#ifndef _NSTL_CSV_DICT_READER
#define _NSTL_CSV_DICT_READER 1

#include <nstl/memory.hpp>

#include <boost/container/flat_map.hpp>
#include <boost/token_functions.hpp>

#include <concepts>
#include <functional>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <cstdint>

namespace nstl::csv
{
constexpr char default_escape = '\\';
constexpr char default_coma = ',';
constexpr char default_quote = '\"';

using csv_separator = boost::escaped_list_separator<char>;
using result_func = std::function<void(const std::string& item_)>;
using column_mapping = boost::container::flat_map<std::string, size_t, std::less<>>;
using values_mapping = boost::container::flat_map<std::string, std::string, std::less<>>;

struct csv_parse_config
{
    bool parse_headers{ true };
    bool skip_empty_lines{ true };
    bool check_columns_size{ true };
    char comment{ '#' }; // '\0' means no commenting in csv
    char escape{ default_escape };
    char coma{ default_coma };
    char quote{ default_quote };
};

class csv_dict_reader
{
    const csv_parse_config _config;
    const csv_separator _sep;
    std::istream& _iss;
    std::string _data;
    std::uint32_t _line{ 1 };
    std::optional<column_mapping> _columns;
    std::optional<std::vector<std::string>> _values;

public:
    explicit csv_dict_reader(std::istream& iss_, const csv_parse_config& config_ = csv_parse_config{});

    bool next();

    observer_ptr<const column_mapping> header() const;
    observer_ptr<const std::vector<std::string>> row() const;
    std::optional<std::string> get_field(const column_mapping& columns_, std::string_view col_) const;
    std::optional<std::string> get_field(std::string_view col_) const;

    bool get_data(values_mapping& target_);
    std::optional<values_mapping> get_data();
};

size_t parse_csv_line(std::string_view line_, const result_func& result_, const csv_separator& sep_ = csv_separator{});
std::ostream& write_field(std::ostream& os_, std::string_view data_,
                          const csv_parse_config& config_ = csv_parse_config{});
std::ostream& write_row(std::ostream& os_, std::span<const std::string> row_,
                        const csv_parse_config& config_ = csv_parse_config{});
} // namespace nstl::csv

#endif