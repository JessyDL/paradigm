#pragma once
#include <memory>

namespace memory
{
	/// \brief defines a begin/end location in (virtual) memory (in respect to the region, or segment).
	class range
	{
	  public:
		range() = default;
		range(std::uintptr_t begin, std::uintptr_t end);

		range(range&& other)	  = default;
		range(const range& other) = default;
		range& operator=(range&& other) = default;
		range& operator=(const range& other) = default;

		bool operator==(const range& other) const;
		bool operator!=(const range& other) const;
		bool operator<(const range& other) const;
		bool operator>(const range& other) const;
		bool operator<=(const range& other) const;
		bool operator>=(const range& other) const;

		bool contains(const range& other) const;
		bool is_contained_by(const range& other) const;
		bool overlaps(const range& other) const;
		bool touches(const range& other) const;

		size_t size() const noexcept;

		std::uintptr_t begin;
		std::uintptr_t end;
	};
}	 // namespace memory
