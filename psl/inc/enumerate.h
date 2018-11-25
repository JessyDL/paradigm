#pragma once
#include <iterator>

namespace psl
{
	template <typename T, typename index_type = size_t, typename TIterator = decltype(std::begin(std::declval<T>())) >
	class enumerator
	{
	  public:
		class iterator
		{
		  public:
			iterator(index_type index, TIterator iterator) noexcept : index(index), it(iterator) {}
			~iterator() = default;
			iterator(const iterator& other) noexcept = default;
			iterator(iterator&& other) noexcept = default;
			iterator& operator=(const iterator& other) noexcept = default;
			iterator& operator=(iterator&& other) noexcept = default;

			iterator& operator++()
			{
				++index;
				++it;
				return *this;
			}

			iterator& operator--()
			{
				--index;
				--it;
				return *this;
			}

			iterator operator++() const
			{
				auto copy{*this};
				++copy;
				return copy;
			}
			iterator operator--() const
			{
				auto copy{*this};
				--copy;
				return copy;
			}

			bool operator!=(const iterator &other) const noexcept { return it != other.it; }

			bool operator==(const iterator &other) const noexcept { return it == other.it; }

			auto operator*() -> std::pair<index_type, decltype(*std::declval<TIterator>())>
			{ return {index, *it}; }


			auto operator*() const -> std::pair<index_type, const decltype(*std::declval<TIterator>())>
			{
				return {index, *it};
			}

		  private:
			  index_type index;
			  TIterator it;
		};
		enumerator(T &container)	:	first(std::begin(container)), last(std::end(container)), index(0), count(std::size(container)) {}
		enumerator(TIterator first, TIterator last, index_type index = 0, index_type count = 0) : first(first), last(last), index(index), count(count) {}

		iterator begin() const { return iterator(index, first); }

		iterator end() const { return iterator(count, last); }

	  private:
		  TIterator first;
		  TIterator last;
		  index_type index;
		  index_type count;
	};

	template <class T>
	auto enumerate(T first, T last, typename enumerator<T>::index_t index = 0) -> enumerator<T>
	{
		return enumerator<T>(first, last, index, std::distance(first, last));
	}

	template <class T>
	auto enumerate(T &content) -> enumerator<T>
	{
		return enumerator<T>(content);
	}
} // namespace psl