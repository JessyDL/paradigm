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

	template<typename T>
	class array_view
	{
	public:
		using element_type = T;
		class iterator
		{
		public:
			iterator() noexcept : it(iterator) {}
			iterator(T* iterator = nullptr) noexcept : it(iterator) {}
			~iterator() = default;
			iterator(const iterator& other) noexcept = default;
			iterator(iterator&& other) noexcept = default;
			iterator& operator=(const iterator& other) noexcept = default;
			iterator& operator=(iterator&& other) noexcept = default;

			iterator& operator++()
			{
				++it;
				return *this;
			}

			iterator& operator--()
			{
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

			T& operator*()
			{
				return *it;
			}
			const T& operator*() const
			{
				return *it;
			}

			T& value()
			{
				return *it;			
			}


			const T& cvalue() const
			{
				return *it;
			}
		private:
			T* it;
		};
		array_view() : first(nullptr), last(nullptr) {};
		template<typename container_t>
		array_view(container_t& container) : first(container.data()), last(container.data() + container.size()) {};
		array_view(T* first, T* last) : first(first), last(last) {};

		~array_view() = default;
		array_view(const array_view& other) noexcept = default;
		array_view(array_view&& other) noexcept = default;
		array_view& operator=(const array_view& other) noexcept = default;
		array_view& operator=(array_view&& other) noexcept = default;

		T& operator[](size_t index)
		{
			return *(first + index);
		}

		const T& operator[](size_t index) const
		{
			return *(first + index);
		}

		iterator begin() const { return iterator(first); }

		iterator end() const { return iterator(last); }

		size_t size() const { return last - first;}
	private:
		T* first;
		T* last;
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