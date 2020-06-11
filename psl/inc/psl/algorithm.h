#pragma once
#include <algorithm>

namespace psl
{
	enum class sorter
	{
		hybrid,
		quick_3way,
		insertion,
		heap,
		merge
	};

	class sorter_quick
	{};
	class sorter_insertion
	{};
	class sorter_merge
	{};
	namespace sorting
	{
		namespace details
		{
			template <typename It, class Pred>
			It partition(It first, It last, Pred&& pred) noexcept
			{
				using std::iter_swap, std::prev, std::next, std::distance;

				while(pred(*first, *next(first)))
				{
					first = next(first);

					if(next(first) == last) return last;
				}

				for(auto i = next(first); i != last; i = next(i))
				{
					if(pred(*i, *first))
					{
						iter_swap(i, first);
						first = next(first);
					}
				}
				return first;
			}
		} // namespace details

		template <typename It, class Pred>
		void quick(const It first, const It last, Pred&& pred) noexcept
		{
			using std::distance, std::next, std::prev;
			if(distance(first, last) > 1)
			{
				auto pivot = ::psl::sorting::details::partition(first, last, std::forward<Pred>(pred));
				if (pivot == last) return;
				quick(first, pivot, std::forward<Pred>(pred));
				quick(next(pivot), last, std::forward<Pred>(pred));
			}
		}

		/// \details sorting algorithm best used for data with few unique items
		template <typename It>
		void quick(const It first, const It last) noexcept
		{
			quick(first, last, std::less<typename std::iterator_traits<It>::value_type>());
		}

		template <typename It, class Pred>
		void insertion(const It first, const It last, Pred&& pred) noexcept
		{
			using std::next, std::prev, std::iter_swap;
			for(auto i = first; i != last; i = next(i))
			{
				for(auto j = i; j != first; j = prev(j))
				{
					if(pred(*j, *prev(j)))
						iter_swap(j, prev(j));
					else
						break;
				}
			}
		}

		template <typename It>
		void insertion(const It first, const It last) noexcept
		{
			insertion(first, last, std::less<typename std::iterator_traits<It>::value_type>());
		}

	} // namespace sorting
	template <typename Sorter, typename It, typename Pred>
	void sort(Sorter&&, const It first, const It last, Pred&& pred) noexcept
	{
		using type = std::remove_cv_t<std::remove_const_t<Sorter>>;
		if constexpr(std::is_same_v<type, sorter_quick>)
		{
			::psl::sorting::quick(first, last, std::forward<Pred>(pred));
		}
		else if constexpr(std::is_same_v<type, sorter_insertion>)
		{
			::psl::sorting::insertion(first, last, std::forward<Pred>(pred));
		}
		else if constexpr(std::is_same_v<type, sorter_merge>)
		{
			::psl::sorting::quick(first, last, std::forward<Pred>(pred));
		}
	}

	template <typename Sorter, typename It>
	void sort(Sorter&&, const It first, const It last) noexcept
	{
		using type = std::remove_cv_t<std::remove_const_t<Sorter>>;
		if constexpr(std::is_same_v<type, sorter_quick>)
		{
			::psl::sorting::quick(first, last);
		}
		else if constexpr(std::is_same_v<type, sorter_insertion>)
		{
			::psl::sorting::insertion(first, last);
		}
		else if constexpr(std::is_same_v<type, sorter_merge>)
		{
			::psl::sorting::quick(first, last);
		}
	}

	template <class InputIt1, class InputIt2, class Compare>
	InputIt1 inplace_set_difference(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
							Compare comp)
	{
		using std::rotate, std::next, std::prev;

		while(first1 != last1)
		{
			if (first2 == last2)
			{
				return last1;
			}

			if(comp(*first1, *first2))
			{
				first1 = next(first1);
			}
			else
			{
				if(!comp(*first2, *first1))
				{
					rotate(first1, next(first1), last1);
					last1 = prev(last1);
				}
				++first2;
			}
		}
		return last1;
	}

	template<class InputIt1, class InputIt2>
	InputIt1 inplace_set_difference(InputIt1 first1, InputIt1 last1,
		InputIt2 first2, InputIt2 last2)
	{
		return inplace_set_difference(first1, last1, first2, last2, std::less<typename std::iterator_traits<InputIt1>::value_type>());
	}

	// template <typename It, typename Pred>
	// void sort(const It first, const It last, Pred&& pred)
	//{
	//	using std::sort;
	//	sort(first, last, std::forward<Pred>(pred));
	//}

	// template <typename It>
	// void sort(const It first, const It last)
	//{
	//	using std::sort;
	//	sort(first, last);
	//}

	// template <typename Policy, typename It, typename Pred>
	// void sort(Policy&& policy, const It first, const It last, Pred&& pred)
	//{
	//	using std::sort;
	//	sort(std::forward<Policy>(policy), first, last, std::forward<Pred>(pred));
	//}
} // namespace psl