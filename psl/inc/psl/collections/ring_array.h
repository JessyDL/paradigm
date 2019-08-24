#pragma once
#include "psl/static_array.h"

namespace psl
{
	template <typename T>
	class ring_array
	{
	  public:
		class iterator
		{
			friend class ring_array<T>;

		  public:
			using difference_type   = std::ptrdiff_t;
			using value_type		= T;
			using pointer			= value_type*;
			using reference			= value_type&;
			using iterator_category = std::random_access_iterator_tag;

			iterator(T* data, size_t begin = 0, size_t capacity = 0, size_t index = 0) noexcept
				: m_Data(data), m_Begin(begin), m_Capacity(capacity), m_Index(index){};
			iterator() noexcept					   = default;
			iterator(const iterator& rhs) noexcept = default;
			iterator(iterator&& rhs) noexcept	  = default;
			iterator& operator=(const iterator& rhs) noexcept = default;
			iterator& operator=(iterator&& rhs) noexcept = default;
			void swap(iterator& iter)
			{
				std::swap(m_Data + offset_of(m_Index), iter.m_Data + iter.offset_of(iter.m_Index));
			};

			bool operator==(const iterator& rhs) { return m_Data == rhs.m_Data && m_Index == rhs.m_Index; }
			bool operator!=(const iterator& rhs) const noexcept
			{
				return m_Data != rhs.m_Data || m_Index != rhs.m_Index;
			}
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
				auto copy{*this};
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
				auto copy{*this};
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
				auto copy{*this};
				--copy;
				return copy;
			}
			iterator& operator-=(difference_type n)
			{
				assert(n <= m_Index);
				m_Index -= n;
				return *this;
			}
			iterator operator-(difference_type n) const
			{
				auto copy{*this};
				copy -= n;
				return copy;
			}

		  private:
			inline size_t offset_of(size_t index) const noexcept { return ((m_Begin + index) % m_Capacity); };

			pointer m_Data{nullptr};
			size_t m_Begin{0u};
			size_t m_Capacity{0u};
			size_t m_Index{0u};
		};


		ring_array(size_t size = 4) : m_Data(new T[size]), m_Begin(m_Data), m_Count(0), m_Capacity(size) {}

		~ring_array() { delete[](m_Data); }


		void reserve(size_t size)
		{
			if(size == m_Capacity) return;

			T* newLoc = new T[size];
			if(newLoc == nullptr) throw std::runtime_error("could not allocate");

			for(auto it = begin(); it != end(); ++it)
			{
				*newLoc = *it;
				++newLoc;
			}
			delete[](m_Data);
			newLoc -= m_Count;
			m_Data	 = newLoc;
			m_Begin	= newLoc;
			m_Capacity = size;
		}
		void resize(size_t size)
		{
			if(size == m_Capacity) return;

			T* newLoc = new T[size];
			if(newLoc == nullptr) throw std::runtime_error("could not allocate");

			for(auto it = begin(); it != end(); ++it)
			{
				*newLoc = *it;
				++newLoc;
			}
			delete[](m_Data);
			newLoc -= m_Count;

			/*for(auto i = m_Count; i < size; ++i)
			{
				*(newLoc + i).T();
			}*/

			m_Data	 = newLoc;
			m_Begin	= newLoc;
			m_Capacity = size;
			m_Count	= size;
		}

		void erase(iterator location) noexcept
		{
			if(location.m_Index == 0)
			{
				pop_front();
			}
			else if(location.m_Index == m_Count)
			{
				pop_back();
			}
			else if(location.m_Index > m_Count / 2)
			{
				for(auto it = location + 1, current = location; it != end(); ++it, ++current)
				{
					*current = *it;
				}
				--m_Count;
			}
			else
			{
				for(auto it = location - 1, current = location; current != begin(); --it, --current)
				{
					*current = *it;
				}
				++m_Begin;
				--m_Count;
			}
		}

		void erase(iterator beginIt, iterator endIt) noexcept
		{
			if(beginIt.m_Index == endIt.m_Index) return;

			auto beginSize = beginIt.m_Index - begin().m_Index;
			auto endSize   = end().m_Index - endIt.m_Index;
			auto count	 = (endIt.m_Index) - beginIt.m_Index;

			if(endSize == beginSize && endSize == 0)
			{
				clear();
				return;
			}

			if(beginSize > endSize) // move end to begin
			{
				for(auto it = endIt, current = beginIt; it != end(); ++it, ++current)
				{
					*current = *it;
				}
				m_Count -= count;
			}
			else
			{
				for(auto it = beginIt - 1, current = endIt - 1; it != begin(); --it, --current)
				{
					*current = *it;
				}
				m_Begin += count;
				m_Count -= count;
			}
		}

		void shrink_to_fit() { resize(m_Count); }

		void clear() noexcept
		{
			m_Count = 0;
			m_Begin = m_Data;
		}

		T& operator[](size_t index) { return *(m_Data + offset_of(index)); }
		T& at(size_t index) noexcept { return *(m_Data + offset_of(index)); }


		const T& operator[](size_t index) const noexcept { return *(m_Data + offset_of(index)); }
		const T& at(size_t index) const noexcept { return *(m_Data + offset_of(index)); }

		const T& at(int64_t index) const noexcept { return *(m_Data + offset_of(index)); }

		void push_back(T&& value)
		{
			if(m_Count == m_Capacity)
			{
				reserve(m_Capacity * 2);
			}

			*(m_Data + end_of()) = std::forward<T>(value);
			++m_Count;
		}

		template <typename... Ts>
		void emplace_back(Ts&&... args)
		{
			if(m_Count == m_Capacity)
			{
				reserve(m_Capacity * 2);
			}

			*(m_Data + end_of()) = T(std::forward<Ts>(args)...);
			++m_Count;
		}

		void push_front(T&& value)
		{
			if(m_Count == m_Capacity)
			{
				reserve(m_Capacity * 2);
			}

			if(m_Begin == m_Data)
			{
				m_Begin = m_Data + m_Capacity;
			}
			--m_Begin;
			++m_Count;
			*m_Begin = std::forward<T>(value);
		}
		template <typename... Ts>
		void emplace_front(Ts&&... args)
		{
			if(m_Count == m_Capacity)
			{
				reserve(m_Capacity * 2);
			}

			if(m_Begin == m_Data)
			{
				m_Begin = m_Data + m_Capacity;
			}
			--m_Begin;
			++m_Count;

			*m_Begin = T(std::forward<Ts>(args)...);
		}

		void pop_back()
		{
			if(m_Count == 0) throw std::runtime_error("no elements to pop_back");

			// back().~T();
			--m_Count;
		}

		void pop_front()
		{
			if(m_Count == 0) throw std::runtime_error("no elements to pop_front");
			// front().~T();
			++m_Begin;
			--m_Count;
		}

		T& back() { return *(m_Data + last_of()); };
		T& front() { return *m_Begin; };

		size_t size() const noexcept { return m_Count; }
		int64_t ssize() const noexcept { return static_cast<int64_t>(m_Count); }
		size_t capacity() const noexcept { return m_Capacity; }


		iterator begin() noexcept { return iterator{m_Data, start_of(), m_Capacity, 0}; }
		iterator end() noexcept { return iterator{m_Data, end_of(), m_Capacity, m_Count}; }

	  private:
		inline auto offset_of(size_t index) const noexcept { return (start_of() + index) & (m_Capacity - 1); }
		inline auto offset_of(int64_t index) const noexcept
		{
			return (static_cast<int64_t>(start_of()) + index) & (m_Capacity - 1);
		}
		inline size_t start_of() const noexcept { return static_cast<size_t>(m_Begin - m_Data); }
		inline size_t end_of() const noexcept { return offset_of(m_Count); }
		inline size_t last_of() const noexcept { return offset_of(m_Count - 1); }

		T* m_Data;
		T* m_Begin;
		size_t m_Count;
		size_t m_Capacity;
	};
} // namespace psl