#ifndef __NSTL_RANGE_PRINT
#define __NSTL_RANGE_PRINT 1

#include <ostream>

namespace nstl
{
	namespace detail {
		template <class IterType, class DelimType>
		class Streamer {
			IterType _beg;
			IterType _end;
			DelimType _delim;

		public:
			Streamer(IterType&& beg_, IterType&& end_, DelimType&& delim_)
				: _beg{ std::forward<IterType>(beg_) }
				, _end{ std::forward<IterType>(end_) }
				, _delim{ std::forward<DelimType>(delim_) }
			{}

			template <class CharT, class TraitsT = std::char_traits<CharT>>
			friend std::basic_ostream<CharT, TraitsT>& operator<< (std::basic_ostream<CharT, TraitsT>& os_, const Streamer& str_)
			{
				if (str_._beg == str_._end) {
					return os_;
				}
				auto itr = str_._beg;
				os_ << (*itr);
				while (++itr != str_._end) {
					os_ << str_._delim;
					os_ << *itr;
				}
				return os_;
			}
		};

		template <class IterType, class DelimType, class EqDelimType>
		class MapStreamer {
			IterType _beg;
			IterType _end;
			DelimType _delim;
			EqDelimType _eq_delim;

		public:
			MapStreamer(IterType&& beg_, IterType&& end_, DelimType&& delim_, EqDelimType&& eq_delim_)
				: _beg{ std::forward<IterType>(beg_) }
				, _end{ std::forward<IterType>(end_) }
				, _delim{ std::forward<DelimType>(delim_) }
				, _eq_delim{ std::forward<EqDelimType>(eq_delim_) }
			{}

			template <class CharT, class TraitsT = std::char_traits<CharT>>
			friend std::basic_ostream<CharT, TraitsT>& operator<< (std::basic_ostream<CharT, TraitsT>& os_, const MapStreamer& str_)
			{
				if (str_._beg == str_._end) {
					return os_;
				}
				auto itr = str_._beg;
				os_ << itr->first;
				os_ << str_._eq_delim;
				os_ << itr->second;
				while (++itr != str_._end) {
					os_ << str_._delim;
					os_ << itr->first;
					os_ << str_._eq_delim;
					os_ << itr->second;
				}
				return os_;
			}
		};
	}

	template <class IterType, class DelimType>
	auto range_print_iter(IterType beg_, IterType end_, DelimType delim_) {
		return ::nstl::detail::Streamer{ std::forward<IterType>(beg_), std::forward<IterType>(end_), std::forward<DelimType>(delim_) };
	}

	template <class RangeType, class DelimType>
	auto range_print(const RangeType& range_, DelimType delim_) {
		return ::nstl::range_print_iter(range_.begin(), range_.end(), std::forward<DelimType>(delim_));
	}

	template <class IterType, class DelimType, class EqDelimType>
	auto range_map_print_iter(IterType beg_, IterType end_, DelimType delim_, EqDelimType eq_delim_) {
		return ::nstl::detail::MapStreamer{ std::forward<IterType>(beg_), std::forward<IterType>(end_), std::forward<DelimType>(delim_), std::forward<EqDelimType>(eq_delim_) };
	}

	template <class RangeType, class DelimType, class EqDelimType>
	auto range_map_print(const RangeType& range_, DelimType delim_, EqDelimType eq_delim_) {
		return ::nstl::range_map_print_iter(range_.begin(), range_.end(), std::forward<DelimType>(delim_), std::forward<EqDelimType>(eq_delim_));
	}
}

#endif
