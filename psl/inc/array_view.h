#pragma once
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
			using value_type = typename std::remove_reference<T>::type;
			using reference = value_type&;
			using pointer = value_type*;
			using difference_type = std::ptrdiff_t;
			typedef std::random_access_iterator_tag iterator_category;

			iterator() noexcept : it(iterator) {}
			iterator(pointer iterator = nullptr) noexcept : it(iterator) {}
			~iterator()								 = default;
			iterator(const iterator& other) noexcept = default;
			iterator(iterator&& other) noexcept		 = default;
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

			iterator operator++(int) const
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

			iterator& operator+=(difference_type offset)
			{
				it += offset;
				return *this;
			}

			iterator& operator-=(difference_type offset)
			{
				it -= offset;
				return *this;
			}

			bool operator!=(const iterator& other) const noexcept { return it != other.it; }

			bool operator==(const iterator& other) const noexcept { return it == other.it; }

			reference operator*() { return *it; }
			const reference operator*() const { return *it; }

			reference value() { return *it; }

			const reference cvalue() const { return *it; }

		  private:
			pointer it;
		};

		using value_type = typename std::remove_reference<typename std::remove_const<T>::type>::type;
		using reference = value_type&;
		using pointer = value_type*;
		using difference_type = std::ptrdiff_t;

		array_view() : first(nullptr), last(nullptr){};
		// template<typename container_t>
		// array_view(container_t& container) : first(container.data()), last(container.data() + container.size()) {};
		array_view(pointer first, pointer last) : first(first), last(last){};

		template <typename IT>
		array_view(IT first, IT last) : first(&(*first)), last(&(*last)){};


		array_view(const std::vector<value_type>& container)
			: first((pointer)container.data()), last((pointer)container.data() + container.size()){};

		template<size_t N>
		array_view(const std::array<value_type, N>& container) : first((pointer)container.data()), last((pointer)container.data() + N)
		{};

		~array_view()								 = default;
		array_view(const array_view& other) noexcept = default;
		array_view(array_view&& other) noexcept		 = default;
		array_view& operator=(const array_view& other) noexcept = default;
		array_view& operator=(array_view&& other) noexcept = default;

		reference operator[](size_t index) { return *(first + index); }

		const reference operator[](size_t index) const { return *(first + index); }

		operator array_view<const value_type>&() const noexcept { return *(array_view<const T>*)(this); }

		iterator begin() const { return iterator(first); }

		iterator end() const { return iterator(last); }

		size_t size() const { return last - first; }

		pointer& internal_data() { return first; };

	  private:
		  pointer first;
		  pointer last;
	};
} // namespace psl