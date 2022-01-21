#pragma once
#include "psl/array.hpp"
#include "psl/static_array.hpp"
#include <iterator>

namespace psl
{
	template <typename T>
	class array_view
	{
	  public:
		class iterator
		{
		  public:
			typedef iterator self_type;
			using value_type		= typename std::remove_reference<T>::type;
			using reference			= value_type&;
			using const_reference	= reference;
			using pointer			= value_type*;
			using difference_type	= std::ptrdiff_t;
			using iterator_category = std::random_access_iterator_tag;

			iterator() noexcept : it(nullptr) {}
			iterator(pointer ptr) noexcept : it(ptr) {}
			~iterator() = default;
			iterator(const iterator& other) noexcept : it(other.it) {};
			iterator(iterator&& other) noexcept : it(other.it) {};
			iterator& operator=(const iterator& other) noexcept
			{
				if(this != &other)
				{
					it = other.it;
				}
				return *this;
			};
			iterator& operator=(iterator&& other) noexcept
			{
				if(this != &other)
				{
					it = other.it;
				}
				return *this;
			};

			// iterator(std::vector<T>::iterator it) : it(&(*it)){};

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

			iterator operator++(int) const
			{
				auto copy {*this};
				++copy;
				return copy;
			}
			iterator operator--() const
			{
				auto copy {*this};
				--copy;
				return copy;
			}

			iterator& operator+=(difference_type offset) noexcept
			{
				it += offset;
				return *this;
			}

			iterator& operator-=(difference_type offset) noexcept
			{
				it -= offset;
				return *this;
			}

			iterator operator+(difference_type offset) const
			{
				auto copy {*this};
				copy += offset;
				return copy;
			}

			iterator operator-(difference_type offset) const
			{
				auto copy {*this};
				copy -= offset;
				return copy;
			}

			difference_type operator-(iterator offset) const { return difference_type {it - offset.it}; }

			difference_type operator+(iterator offset) const { return difference_type {it + offset.it}; }

			bool operator!=(const iterator& other) const noexcept { return it != other.it; }

			bool operator==(const iterator& other) const noexcept { return it == other.it; }

			bool operator<(const iterator& other) const noexcept { return it < other.it; }
			bool operator<=(const iterator& other) const noexcept { return it <= other.it; }
			bool operator>(const iterator& other) const noexcept { return it > other.it; }
			bool operator>=(const iterator& other) const noexcept { return it >= other.it; }

			reference operator*() noexcept { return *it; }
			reference operator*() const noexcept { return *it; }
			pointer operator->() noexcept { return it; }
			pointer operator->() const noexcept { return it; }

			reference value() noexcept { return *it; }

			reference cvalue() const noexcept { return *it; }

			operator reference() noexcept { return *it; }

			operator const_reference() const noexcept { return *it; }

			// void swap(iterator& other) { std::swap(it, other.it); }

		  private:
			pointer it;
		};

		using value_type	  = typename std::remove_reference<typename std::remove_const<T>::type>::type;
		using reference		  = typename std::remove_reference<T>::type&;
		using const_reference = const value_type&;
		using pointer		  = value_type*;
		using difference_type = std::ptrdiff_t;

		array_view() : first(nullptr), last(nullptr) {};
		// template<typename container_t>
		// array_view(container_t& container) : first(container.data()), last(container.data() + container.size()) {};
		array_view(pointer first, pointer last) : first(first), last(last) {};

		template <typename IT>
		array_view(IT first, size_t count) :
			first((count > 0) ? std::addressof(*first) : nullptr),
			last((count > 0) ? (pointer)((std::uintptr_t)std::addressof(*first) + count * sizeof(T)) : nullptr) {};

		template <typename IT>
		array_view(IT first, IT last)
		{
			if(first == last)
			{
				this->first = nullptr;
				this->last	= nullptr;
			}
			else
			{
				this->first = const_cast<pointer>(std::addressof(*first));
				this->last	= const_cast<pointer>(std::addressof(*std::prev(last)) + 1);
			}
		};


		array_view(const std::vector<value_type>& container) :
			first((pointer)container.data()), last((pointer)container.data() + container.size()) {};

		template <size_t N>
		array_view(const std::array<value_type, N>& container) :
			first((pointer)container.data()), last((pointer)container.data() + N) {};

		~array_view()								 = default;
		array_view(const array_view& other) noexcept = default;
		array_view(array_view&& other) noexcept		 = default;
		array_view& operator=(const array_view& other) noexcept = default;
		array_view& operator=(array_view&& other) noexcept = default;

		reference operator[](size_t index) { return *(first + index); }

		const_reference operator[](size_t index) const { return *(first + index); }

		operator array_view<const value_type>&() const noexcept { return *(array_view<const T>*)(this); }


		explicit operator std::vector<value_type>() const noexcept { return std::vector<value_type> {first, last}; }

		iterator begin() const { return iterator(first); }

		iterator end() const { return iterator(last); }

		size_t size() const { return last - first; }

		pointer& internal_data() { return first; };

		pointer data() const noexcept { return first; };

		array_view slice(size_t begin, size_t end) { return array_view<T> {first + begin, first + end}; }

	  private:
		pointer first;
		pointer last;
	};

	template <typename IT>
	array_view(IT first, IT last) -> array_view<typename std::iterator_traits<IT>::value_type>;


	template <typename IT>
	array_view(IT first, size_t count) -> array_view<typename std::iterator_traits<IT>::value_type>;
}	 // namespace psl