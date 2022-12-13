#pragma once
#include "psl/static_array.hpp"

namespace psl
{
/// \brief a deque-like interface implemented as an std::array<T, N> under the hood
///
/// \details When you need a deque like
template <typename T, size_t N>
class static_ring_array
{
  public:
	class iterator
	{
	  public:
		using difference_type	= std::ptrdiff_t;
		using value_type		= T;
		using pointer			= value_type*;
		using reference			= value_type&;
		using iterator_category = std::random_access_iterator_tag;

		iterator(T* data, size_t begin = 0, size_t index = 0) noexcept :
			m_Data(data), m_Begin(begin), m_Index(index) {};
		iterator() noexcept								  = default;
		iterator(const iterator& rhs) noexcept			  = default;
		iterator(iterator&& rhs) noexcept				  = default;
		iterator& operator=(const iterator& rhs) noexcept = default;
		iterator& operator=(iterator&& rhs) noexcept	  = default;
		void swap(iterator& iter)
		{
			std::swap(m_Data + offset_of(m_Index), iter.m_Data + iter.offset_of(iter.m_Index));
		};

		bool operator==(const iterator& rhs) { return m_Data == rhs.m_Data && m_Index == rhs.m_Index; }
		bool operator!=(const iterator& rhs) const noexcept { return m_Data != rhs.m_Data || m_Index != rhs.m_Index; }
		bool operator<(const iterator& rhs) const noexcept
		{
			return (m_Data < rhs.m_Data) ? true : m_Index < rhs.m_Index;
		}
		bool operator<=(const iterator& rhs) const noexcept
		{
			return (m_Data <= rhs.m_Data) ? true : m_Index <= rhs.m_Index;
		}
		bool operator>(const iterator& rhs) const noexcept
		{
			return (m_Data > rhs.m_Data) ? true : m_Index > rhs.m_Index;
		}
		bool operator>=(const iterator& rhs) const noexcept
		{
			return (m_Data >= rhs.m_Data) ? true : m_Index >= rhs.m_Index;
		}

		reference operator*() { return *(m_Data + offset_of(m_Index)); }
		pointer operator->() { return m_Data + offset_of(m_Index); }
		reference operator[](difference_type n) { return *(m_Data + offset_of(m_Index + n)); }

		iterator& operator++()
		{
			++m_Index;
			return *this;
		}
		iterator operator++(int) const
		{
			auto copy {*this};
			++copy;
			return copy;
		}
		iterator& operator+=(difference_type n)
		{
			m_Index += n;
			return *this;
		}
		iterator operator+(difference_type n) const
		{
			auto copy {*this};
			copy += n;
			return copy;
		}
		iterator& operator--()
		{
			--m_Index;
			return *this;
		}
		iterator operator--(int) const
		{
			auto copy {*this};
			--copy;
			return copy;
		}
		iterator& operator-=(difference_type n)
		{
			psl_assert(n <= m_Index, "expected index: {} to be larger than {}", m_Index, n);
			m_Index -= n;
			return *this;
		}
		iterator operator-(difference_type n) const
		{
			auto copy {*this};
			copy -= n;
			return copy;
		}

	  private:
		inline size_t offset_of(size_t index) const noexcept { return ((m_Begin + index) % N); };

		pointer m_Data {nullptr};
		size_t m_Begin {0u};
		size_t m_Index {0u};
	};

	static_ring_array() : m_Data((T*)std::malloc(sizeof(T) * N)), m_Begin(m_Data), m_Count(0) {}

	~static_ring_array() { std::free(m_Data); }

	T& operator[](size_t index) { return *(m_Data + offset_of(index)); }

	void push_back(T&& value)
	{
		if(m_Count == N) throw std::runtime_error("out of space");

		*(m_Data + end_of()) = std::forward<T>(value);
		++m_Count;
	}

	void push_front(T&& value)
	{
		if(m_Count == N) throw std::runtime_error("out of space");

		if(m_Begin == m_Data)
		{
			m_Begin = m_Data + N;
		}
		--m_Begin;
		++m_Count;
		*m_Begin = std::forward<T>(value);
	}

	void pop_back()
	{
		if(m_Count == 0) throw std::runtime_error("no elements to pop_back");

		back().~T();
		--m_Count;
	}

	void pop_front()
	{
		if(m_Count == 0) throw std::runtime_error("no elements to pop_front");
		front().~T();
		++m_Begin;
		--m_Count;
	}

	T& back() { return *(m_Data + last_of()); };
	T& front() { return *m_Begin; };

	size_t size() const noexcept { return m_Count; }
	size_t capacity() const noexcept { return N; }


	iterator begin() { return iterator {m_Data, start_of(), 0}; }
	iterator end() { return iterator {m_Data, end_of(), m_Count}; }

  private:
	inline auto offset_of(size_t index) const noexcept { return (start_of() + index) % N; }
	inline size_t start_of() const noexcept { return (size_t)(m_Begin - m_Data); }
	inline size_t end_of() const noexcept { return offset_of(m_Count); }
	inline size_t last_of() const noexcept { return offset_of(m_Count - 1); }

	T* m_Data;
	T* m_Begin;
	size_t m_Count;
};
}	 // namespace psl
