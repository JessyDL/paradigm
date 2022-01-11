#pragma once
#include "psl/assertions.h"
#include "range.h"
#include <cstring>
#include <optional>

namespace memory
{
	class range_t;
	/// \brief a segment defines a addressable and mapped region of memory.
	class segment
	{
	  public:
		segment();
		segment(range_t& _range, bool physically_backed = true);
		const memory::range_t& range() const noexcept;
		segment(const segment& other) noexcept : m_Range(other.m_Range), m_IsVirtual(other.m_IsVirtual) {};
		segment& operator=(const segment& other) noexcept
		{
			if(this != &other)
			{
				m_Range		= other.m_Range;
				m_IsVirtual = other.m_IsVirtual;
			}
			return *this;
		}
		segment(segment&& other) noexcept : m_Range(other.m_Range), m_IsVirtual(other.m_IsVirtual) {};
		segment& operator=(segment&& other) noexcept
		{
			if(this != &other)
			{
				m_Range		= other.m_Range;
				m_IsVirtual = other.m_IsVirtual;
			}
			return *this;
		};
		// sets the memory range with the value provided, in case it is not backed by physical memory, this turns into a
		// no-op
		template <typename T>
		void set(T&& value)
		{
			static_assert(std::is_trivial<T>::value, "T should be a trivial type such as a POD data container.");
			assert(sizeof(T) <= m_Range->size());
			if(!m_IsVirtual && is_valid()) std::memcpy((void*)m_Range->begin, &value, sizeof(T));
		}

		template <typename T>
		void set(const T& value)
		{
			static_assert(std::is_trivial<T>::value, "T should be a trivial type such as a POD data container.");
			assert(sizeof(T) <= m_Range->size());
			if(!m_IsVirtual && is_valid()) std::memcpy((void*)m_Range->begin, &value, sizeof(T));
		}

		void unsafe_set(void* data, std::optional<size_t> _size = {})
		{
			if(!m_IsVirtual && is_valid()) std::memcpy((void*)m_Range->begin, data, _size.value_or(m_Range->size()));
		}

		bool is_virtual() const;
		bool is_valid() const;

	  private:
		memory::range_t* m_Range;
		bool m_IsVirtual {false};
	};
}	 // namespace memory
