#pragma once
#include <iterator>

namespace psl
{
	template<typename T>
	class array_view
	{
	public:
		using element_type = T;
		class iterator
		{
		public:
			typedef iterator self_type;
			typedef T value_type;
			typedef T& reference;
			typedef T* pointer;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

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
		//template<typename container_t>
		//array_view(container_t& container) : first(container.data()), last(container.data() + container.size()) {};
		array_view(T* first, T* last) : first(first), last(last) {};

		template<typename IT>
		array_view(IT& first, IT& last) : first(&(*first)), last(&(*last)) {};

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

		T*& internal_data() { return first; };
	private:
		T* first;
		T* last;
	};
} // namespace psl