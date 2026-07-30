#include "exception.hpp"

#include <ostream>

namespace nstl
{
exception::exception(const std::string& val_, const char* file_, int line_)
    : std::runtime_error{ val_ }, _file{ file_ }, _line{ line_ }
{
}

exception::exception(const char* val_, const char* file_, int line_)
    : std::runtime_error{ val_ }, _file{ file_ }, _line{ line_ }
{
}

std::string_view exception::file() const { return _file ? std::string_view{ _file } : std::string_view{}; }

int exception::line() const { return _line; }

namespace
{
std::ostream& print_exception(std::ostream& os_, const std::exception& exc_, const unsigned int level = 0)
{
    for (unsigned int idx = 0; idx < level; ++idx)
    {
        os_ << ' ';
    }
    os_ << "exception: " << exc_.what();
    try
    {
        std::rethrow_if_nested(exc_);
        return os_;
    }
    catch (const std::exception& nested_)
    {
        os_ << "\n";
        return print_exception(os_, nested_, level + 1);
    }
}
} // namespace

std::ostream& operator<<(std::ostream& os_, const exception& exc_) { return print_exception(os_, exc_); }
} // namespace nstl
