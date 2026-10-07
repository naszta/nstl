#include "csv_dict_reader.hpp"
#include "exception.hpp"

#include <boost/tokenizer.hpp>

#include <array>
#include <istream>
#include <ostream>

using tokenizer_view_type = boost::tokenizer<nstl::csv::csv_separator, std::string_view::const_iterator>;
using tokenizer_str_type = boost::tokenizer<nstl::csv::csv_separator, std::string::const_iterator>;

namespace nstl::csv
{
namespace
{
std::optional<column_mapping> parse_columns(std::istream& iss_, std::string& data_, const csv_separator& sep_,
                                            bool parse_headers_)
{
    std::optional<column_mapping> columns;
    if (!parse_headers_)
    {
        return columns;
    }
    if (std::getline(iss_, data_))
    {
        columns.emplace();

        tokenizer_str_type tok{ data_, sep_ };
        size_t idx = 0;
        for (auto itr = tok.begin(); itr != tok.end(); ++itr, ++idx)
        {
            const auto helper = columns->emplace(*itr, idx);
            NSTL2_THROW_EXCEPTION_IF(!helper.second, helper.first->first << " column duplicated ("
                                                                         << helper.first->second << " vs " << idx
                                                                         << ')');
        }

        return columns;
    }
    NSTL2_THROW_EXCEPTION("failed to read from istream");
}
} // namespace

csv_dict_reader::csv_dict_reader(std::istream& iss_, const csv_parse_config& config_)
    : _config{ config_ }, _sep{ _config.escape, _config.coma, _config.quote }, _iss{ iss_ },
      _columns{ parse_columns(_iss, _data, _sep, _config.parse_headers) }
{
}

bool csv_dict_reader::next()
{
    for (; std::getline(_iss, _data); ++_line)
    {
        // handle empty line
        if (_config.skip_empty_lines && _data.empty())
        {
            continue;
        }
        // handle comment
        if (_config.comment != '\0' && !_data.empty() && _data.front() == _config.comment)
        {
            continue;
        }
        // process the values
        if (_values.has_value())
        {
            _values->clear();
        }
        else
        {
            _values.emplace();
            _values->reserve(_columns ? _columns->size() : 0);
        }
        tokenizer_str_type tok{ _data, _sep };
        for (auto itr = tok.begin(); itr != tok.end(); ++itr)
        {
            _values->push_back(*itr);
        }
        if (_columns && _config.check_columns_size && _values->size() != _columns->size())
        {
            NSTL2_THROW_EXCEPTION("columns not in line with the number of fields (line: " << _line << ')');
        }
        return true;
    }
    _values.reset();
    return false;
}

observer_ptr<const column_mapping> csv_dict_reader::header() const
{
    if (_columns.has_value())
    {
        const auto& columns = _columns.value();
        return make_observer(&columns);
    }
    return nullptr;
}

observer_ptr<const std::vector<std::string>> csv_dict_reader::row() const
{
    if (_values.has_value())
    {
        const auto& values = _values.value();
        return make_observer(&values);
    }
    return nullptr;
}

std::optional<std::string> csv_dict_reader::get_field(const column_mapping& columns_, const std::string_view col_) const
{
    if (!_values.has_value())
    {
        return std::nullopt;
    }
    const auto& values = _values.value();

    if (const auto itr = columns_.find(col_); itr != columns_.end())
    {
        if (values.size() <= itr->second)
        {
            return std::nullopt;
        }
        return values[itr->second];
    }
    return std::nullopt;
}

std::optional<std::string> csv_dict_reader::get_field(const std::string_view col_) const
{
    if (_columns.has_value())
    {
        return this->get_field(_columns.value(), col_);
    }
    return std::nullopt;
}

bool csv_dict_reader::get_data(values_mapping& target_)
{
    NSTL2_THROW_EXCEPTION_IF(!_columns.has_value(), "this API can be used only when headers parsed");
    if (this->next())
    {
        NSTL2_THROW_EXCEPTION_IF(!_values.has_value(), "This is a massive bug");
        const auto& values = _values.value();
        target_.clear();
        target_.reserve(_columns->size());

        auto helper = target_.begin();
        for (const auto& [column, idx] : _columns.value())
        {
            if (idx < values.size())
            {
                helper = target_.emplace_hint(helper, column, values[idx]);
            }
        }

        return true;
    }
    return false;
}

std::optional<values_mapping> csv_dict_reader::get_data()
{
    std::optional<values_mapping> values;
    values.emplace();
    if (!this->get_data(values.value()))
    {
        values.reset();
    }
    return values;
}

size_t parse_csv_line(const std::string_view line_, const result_func& result_, const csv_separator& sep_)
{
    NSTL2_THROW_EXCEPTION_IF(!result_, "Functor not set");
    tokenizer_view_type tok{ line_.cbegin(), line_.cend(), sep_ };
    size_t retval = 0;
    for (auto itr = tok.begin(); itr != tok.end(); ++itr, ++retval)
    {
        result_(*itr);
    }
    return retval;
}

std::ostream& write_field(std::ostream& os_, const std::string_view data_, const csv_parse_config& config_)
{
    const std::array<char, 5> special_chars{ config_.escape, config_.coma, config_.quote, '\n', '\0' };

    if (data_.find_first_of(special_chars.data()) == std::string_view::npos)
    {
        return os_.write(data_.data(), data_.size());
    }
    os_.put(config_.quote);
    for (const char ch : data_)
    {
        if (ch == config_.escape || ch == config_.quote)
        {
            os_.put(config_.escape);
            os_.put(ch);
        }
        else if (ch == '\n')
        {
            os_.put(config_.escape);
            os_.put('n');
        }
        else
        {
            os_.put(ch);
        }
    }
    return os_.put(config_.quote);
}

std::ostream& write_row(std::ostream& os_, const std::span<const std::string> row_, const csv_parse_config& config_)
{
    auto itr = row_.begin();
    if (itr == row_.end())
    {
        return os_;
    }
    write_field(os_, *itr, config_);
    while (++itr != row_.end())
    {
        os_.put(config_.coma);
        write_field(os_, *itr, config_);
    }
    return os_;
}

} // namespace nstl::csv
