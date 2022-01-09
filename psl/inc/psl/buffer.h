#pragma once
#include "psl/memory/allocator.h"
#include "psl/memory/segment.h"
#include "psl/template_utils.h"
#include <algorithm>
#include <optional>


namespace psl
{
	template <size_t SIZE = 8>
	struct local_storage
	{
		static constexpr size_t size {SIZE};
		void* storage() const { return (void*)&_storage[0]; }

		char _storage[SIZE];
	};

	template <>
	struct local_storage<0u>
	{
		static constexpr size_t size {0u};
		void* storage() const { return nullptr; }
	};

	template <typename T, typename Storage = local_storage<8>>
	struct buffer
	{
	  private:
		using iterator					 = T*;
		using const_iterator			 = const T*;
		static constexpr size_t SBO_Size = Storage::size * sizeof(char);

		memory::allocator_base* m_Allocator;
		std::optional<memory::segment> m_Segment;
		Storage m_Storage;
		T* first;
		T* last;
		T* m_Capacity;

		static size_t grow(size_t size)
		{
			auto current = size;
			size_t next	 = 0;
			if(current < 1024)
			{
				next = (size_t)(pow(2, ceil(log(current) / log(2))) * 2);
			}
			else
			{
				next = (size_t)(pow(2, ceil(log(current) / log(2))) * 1.5);
			}
			return next;
		}

		void fill(T* begin, T* end)
		{
			if constexpr(std::is_pod<T>::value)
			{
				for(; begin < end; ++begin) *begin = T();
			}
			else
			{
				for(; begin < end; ++begin) new(begin) T();
			}
		}


		void fill(T* begin, T* end, const T& value)
		{
			if constexpr(std::is_pod<T>::value)
			{
				static_assert(std::is_copy_assignable<T>::value, "No copy assignment operator available");
				for(; begin < end; ++begin) *begin = value;
			}
			else
			{
				static_assert(std::is_copy_constructible<T>::value, "No copy constructor available");
				for(; begin < end; ++begin) new(begin) T(value);
			}
		}

		void move_range(T* destination, T* begin, T* end)
		{
			if constexpr(std::is_pod<T>::value)
			{
				for(; begin != end; ++begin, ++destination)
				{
					*destination = *begin;
				}
			}
			else
			{
				for(; begin != end; ++begin, ++destination) *destination = std::move(*begin);
			}
		}


	  public:
		buffer() : first(nullptr), last(nullptr), m_Capacity(nullptr) {};
		explicit buffer(size_t reserve, memory::region* region) noexcept :
			m_Allocator(region->allocator()),
			m_Segment((reserve * sizeof(T) < SBO_Size) ? std::nullopt : m_Allocator->allocate(sizeof(T) * reserve)),
			first((!m_Segment) ? (T*)m_Storage.storage() : (T*)m_Segment.value().range().begin),
			last(first + (reserve)), m_Capacity(first + reserve)
		{
			fill(first, last);
		};

		buffer(std::initializer_list<T> values, memory::region* region) :
			m_Allocator(region->allocator()),
			m_Segment((values.size() * sizeof(T) < SBO_Size) ? std::nullopt
															 : m_Allocator->allocate(sizeof(T) * values.size())),
			first((!m_Segment) ? (T*)m_Storage.storage() : (T*)m_Segment.value().range().begin), last(first),
			m_Capacity(first + values.size())
		{
			emplace_back(values);
		}

		buffer(const buffer& other) noexcept :
			m_Allocator(other.m_Allocator),
			m_Segment((other.capacity() * sizeof(T) < SBO_Size) ? std::nullopt
																: m_Allocator->allocate(sizeof(T) * other.capacity())),
			first((!m_Segment) ? (T*)m_Storage.storage() : (T*)m_Segment.value().range().begin),
			last(first + other.size()), m_Capacity(first + other.capacity())
		{
			for(size_t i = 0, count = other.size(); i < count; ++i) (*this)[i] = other[i];
		};

		buffer(buffer&& other) noexcept :
			// if it fits into the SBO, move it there, if not check if it's stored in the other's SBO
			// and if is, allocate a new buffer, otherwise "hijack" the buffer
			m_Allocator(other.m_Allocator),
			m_Segment((other.capacity() * sizeof(T) < SBO_Size) ? std::nullopt
					  : (other.using_sbo())						? m_Allocator->allocate(sizeof(T) * other.capacity())
																: other.m_Segment),
			first((!m_Segment) ? (T*)m_Storage.storage() : (T*)m_Segment.value().range().begin),
			last(first + other.size()), m_Capacity(first + other.capacity())
		{
			other.m_Allocator = nullptr;
			other.m_Segment	  = {};
			other.first		  = nullptr;
			other.last		  = nullptr;
			other.m_Capacity  = nullptr;
		};

		~buffer()
		{
			clear();

			if(m_Segment)
			{
				m_Allocator->deallocate(m_Segment.value());
			}
			m_Segment	= {};
			first		= nullptr;
			last		= nullptr;
			m_Capacity	= nullptr;
			m_Allocator = nullptr;
		}

		T& operator[](size_t index) const noexcept { return *(first + index); }

		iterator begin() { return first; }
		const_iterator begin() const { return first; }
		iterator end() { return last; }
		const_iterator end() const { return last; }

		void reserve(size_t capacity) noexcept
		{
			if(first + capacity <= m_Capacity) return;

			force_reserve(capacity);
		}
		void force_reserve(size_t capacity) noexcept
		{
			if(first == (T*)m_Storage.storage() && capacity * sizeof(T) <= SBO_Size)
			{
				m_Capacity = first + capacity;
				return;
			}

			const size_t old_size = size();
			auto new_segment	  = m_Allocator->allocate(sizeof(T) * capacity);

			T* new_buffer_location = (T*)new_segment.value().range().begin;
			move_range(new_buffer_location, first, last);

			if(first != (T*)m_Storage.storage())
			{
				m_Allocator->deallocate(m_Segment.value());
			}

			m_Segment  = new_segment;
			first	   = new_buffer_location;
			last	   = first + old_size;
			m_Capacity = first + capacity;
		}

		size_t size() const noexcept { return (size_t)(last - first); }
		size_t capacity() const noexcept { return m_Capacity - first; }
		bool using_sbo() const noexcept { return first == (T*)m_Storage.storage(); }

		void clear()
		{
			if constexpr(!std::is_pod<T>::value)
			{
				while(last > first) (--last)->~T();
			}
			last = first;
		}

		void fill()
		{
			if(last == m_Capacity) return;

			if constexpr(std::is_pod<T>::value)
			{
				for(; last < m_Capacity; ++last) *last = T();
			}
			else
			{
				for(; last < m_Capacity; ++last) new(last) T();
			}
		}
		void fill(const T& value)
		{
			if constexpr(std::is_pod<T>::value)
			{
				for(; last < m_Capacity; ++last)
				{
					*last = value;
				}
			}
			else
			{
				for(; last < m_Capacity; ++last)
				{
					new(last) T(value);
				}
			}
		}
		void fill(std::initializer_list<T> values)
		{
			auto it = std::begin(values);
			if constexpr(std::is_pod<T>::value)
			{
				for(; last < m_Capacity; ++last)
				{
					*last = *it;
					it	  = std::next(it, 1);
					if(it == std::end(values)) it = std::begin(values);
				}
			}
			else
			{
				for(; last < m_Capacity; ++last)
				{
					new(last) T(*it);
					it = std::next(it, 1);
					if(it == std::end(values)) it = std::begin(values);
				}
			}
		}

		void shrink_to_fit() noexcept
		{
			if(last == m_Capacity) return;

			force_reserve(size());
		}

		void resize(size_t newSize) noexcept
		{
			if(newSize < size())
			{
				erase(first + newSize, last);
				shrink_to_fit();
			}
			else
			{
				reserve(newSize);
				fill(last, first + newSize);
				last = first + newSize;
			}
		}
		void resize(size_t newSize, const T& value) noexcept
		{
			if(newSize < size())
			{
				erase(first + newSize, last);
				shrink_to_fit();
			}
			else
			{
				reserve(newSize);
				fill(last, first + newSize, value);
				last = first + newSize;
			}
		}

		void replace(size_t where, std::initializer_list<T> values) noexcept { replace(first + where, values); }
		void replace(iterator where, std::initializer_list<T> values) noexcept
		{
			if(where < first || where + values.size() > last) return;

			if constexpr(!std::is_pod<T>::value)
			{
				for(auto it = where; it < where + values.size(); ++it)
				{
					it->~T();
				}
			}
			for(auto& it : values)
			{
				*(where++) = std::move(it);
			}
		}

		void erase(iterator it) noexcept { erase(it, it + 1); }
		void erase(iterator first, iterator last) noexcept
		{
			if(first > last || first < this->first || last > this->last) return;

			if constexpr(!std::is_pod<T>::value)
			{
				for(auto it = first; it < last; ++it)
				{
					it->~T();
				}
			}

			size_t new_size = first - this->first + this->last - last;
			move_range(first, last, this->last);
			this->last = this->first + new_size;
		}

		void push_back(const T& value) { push_back_n(1, value); }
		void push_back_n(size_t count, const T& value)
		{
			size_t expected_size = count + size();
			if(expected_size > capacity())
			{
				reserve(grow(expected_size));
			}
			auto new_last = first + expected_size;
			if constexpr(std::is_pod<T>::value)
			{
				for(; last < new_last; ++last)
				{
					*last = value;
				}
			}
			else
			{
				for(; last < new_last; ++last)
				{
					new(last) T(value);
				}
			}
		}
		void push_back(std::initializer_list<T> values)
		{
			size_t expected_size = values.size() + size();
			if(expected_size > capacity())
			{
				reserve(grow(expected_size));
			}
			auto new_last = first + expected_size;
			auto it		  = std::begin(values);
			if constexpr(std::is_pod<T>::value)
			{
				for(; last < new_last; ++last)
				{
					*last = *it;
					it	  = std::next(it, 1);
					if(it == std::end(values)) it = std::begin(values);
				}
			}
			else
			{
				for(; last < new_last; ++last)
				{
					new(last) T(*it);
					it = std::next(it, 1);
					if(it == std::end(values)) it = std::begin(values);
				}
			}
		}

		template <typename... Args>
		void emplace_back(Args&&... args)
		{
			if constexpr(utility::templates::all_same<Args...>::value)
			{
				emplace_back_v(args...);
			}
			else
			{
				emplace_back_n(1u, args...);
			}
		}
		template <typename... Args>
		void emplace_back_n(size_t count, Args&&... args)
		{
			size_t expected_size = count + size();
			if(expected_size > capacity())
			{
				reserve(grow(expected_size));
			}
			auto new_last = first + expected_size;
			if constexpr(std::is_pod<T>::value)
			{
				for(; last < new_last; ++last)
				{
					*last = T(std::forward<Args>(args)...);
				}
			}
			else
			{
				for(; last < new_last; ++last)
				{
					new(last) T(std::forward<Args>(args)...);
				}
			}
		}

		template <typename... V>
		void emplace_back_v(V&&... values)
		{
			size_t expected_size = sizeof...(V) + size();
			if(expected_size > capacity())
			{
				reserve(grow(expected_size));
			}
			auto new_last = first + expected_size;
			if constexpr(std::is_pod<T>::value)
			{
				//(..., *last++ = T(std::forward<V>(values)...));
			}
			else
			{
				(..., new(last++) T(std::forward<V>(values)));
			}
		}

		void emplace_back(std::initializer_list<T> values)
		{
			size_t expected_size = values.size() + size();
			if(expected_size > capacity())
			{
				reserve(grow(expected_size));
			}
			auto new_last = first + expected_size;
			auto it		  = std::begin(values);
			if constexpr(std::is_pod<T>::value)
			{
				for(; last < new_last; ++last)
				{
					*last = *it;
					it	  = std::next(it);
				}
			}
			else
			{
				for(; last < new_last; ++last)
				{
					new(last) T(std::move(*it));
					it = std::next(it);
				}
			}
		}
	};


	template <typename T, typename Storage = local_storage<std::max<size_t>(40u, sizeof(T) * 4)>>
	using vector = buffer<T, Storage>;
}	 // namespace psl
