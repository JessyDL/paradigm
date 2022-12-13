#pragma once
#include <cstdint>

namespace psl::async
{
enum class barrier_type : uint8_t
{
	READ  = 0,
	WRITE = 1
};

enum class launch : uint8_t
{
	IMMEDIATE = 0,
	ASYNC	  = 1,
	DEFERRED  = 2
};

class barrier
{
  public:
	using location_t   = std::uintptr_t;
	barrier() noexcept = default;
	barrier(location_t begin, location_t end, barrier_type type = barrier_type::READ) :
		m_Begin(begin), m_End(end), m_Type(type) {};

	bool operator==(const barrier& other) const noexcept
	{
		return m_Begin == other.m_Begin && m_End == other.m_End && m_Type == other.m_Type;
	}
	bool operator!=(const barrier& other) const noexcept
	{
		return m_Begin != other.m_Begin || m_End != other.m_End || m_Type != other.m_Type;
	}
	location_t begin() const noexcept { return m_Begin; }
	location_t end() const noexcept { return m_End; }
	location_t size() const noexcept { return m_End - m_Begin; }
	barrier_type type() const noexcept { return m_Type; }

	void begin(location_t location) noexcept { m_Begin = location; };
	void end(location_t location) noexcept { m_End = location; };
	void type(barrier_type type) noexcept { m_Type = type; }

	void move(location_t location) noexcept
	{
		m_End	= size() + location;
		m_Begin = location;
	}
	void resize(location_t new_size) noexcept { m_End = m_Begin + new_size; };

	bool conflicts(const barrier& other) const noexcept
	{
		if((m_Type == barrier_type::READ && m_Type == other.m_Type) || !overlaps(other)) return false;
		return true;
	}

	bool overlaps(const barrier& other) const noexcept
	{
		return (other.m_Begin >= m_Begin && other.m_Begin < m_End) || (other.m_End > m_Begin && other.m_End <= m_End);
	}

  private:
	location_t m_Begin {0};
	location_t m_End {0};
	barrier_type m_Type;
};

}	 // namespace psl::async