#ifndef _NSTL_BASE64_H
#define _NSTL_BASE64_H 1

#include <nstl/exception.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <algorithm>
#include <iterator>
#include <ranges>

namespace nstl
{
template <class InItr, class OutIt>
    requires(std::random_access_iterator<InItr> && std::output_iterator<OutIt, char>)
OutIt to_base64(InItr beg_, InItr end_, OutIt out_)
{
    using namespace boost::archive::iterators;
    using Base64Itr = base64_from_binary<transform_width<InItr, 6, 8>>;

    if (beg_ == end_)
    {
        return out_;
    }

    const auto size = std::distance(beg_, end_);
    NSTL2_THROW_EXCEPTION_IF(size < 0, "Invalid iterators");

    auto out = std::copy(Base64Itr{ beg_ }, Base64Itr{ end_ }, out_);
    return std::fill_n(out, (3 - size % 3) % 3, '=');
}

template <class RangeT, class OutIt>
    requires(std::ranges::random_access_range<RangeT> && std::output_iterator<OutIt, char>)
OutIt to_base64(RangeT&& range_, OutIt out_)
{
    static_assert(std::is_lvalue_reference_v<RangeT>);
    return to_base64(range_.begin(), range_.end(), out_);
}

template <class InItr, class OutIt>
    requires(std::bidirectional_iterator<InItr> && std::output_iterator<OutIt, char>)
OutIt from_base64(InItr beg_, InItr end_, OutIt out_)
{
    using namespace boost::archive::iterators;
    using Base64Itr = transform_width<binary_from_base64<InItr>, 8, 6>;
    if (beg_ == end_)
    {
        return out_;
    }
    while (beg_ != end_ && *std::prev(end_) == '=')
    {
        --end_;
    }
    return std::copy(Base64Itr{ beg_ }, Base64Itr{ end_ }, out_);
}

template <class RangeT, class OutIt>
    requires(std::ranges::bidirectional_range<RangeT> && std::output_iterator<OutIt, char>)
OutIt from_base64(RangeT&& range_, OutIt out_)
{
    static_assert(std::is_lvalue_reference_v<RangeT>);
    return from_base64(range_.begin(), range_.end(), out_);
}
} // namespace nstl

#endif
