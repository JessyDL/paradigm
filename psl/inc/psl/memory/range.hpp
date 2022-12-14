#pragma once
#include <memory>

namespace memory {
/// \brief defines a begin/end location in (virtual) memory (in respect to the region, or segment).
class range_t {
  public:
	range_t() = default;
	range_t(std::uintptr_t begin, std::uintptr_t end);

	range_t(range_t&& other)				 = default;
	range_t(const range_t& other)			 = default;
	range_t& operator=(range_t&& other)		 = default;
	range_t& operator=(const range_t& other) = default;

	bool operator==(const range_t& other) const;
	bool operator!=(const range_t& other) const;
	bool operator<(const range_t& other) const;
	bool operator>(const range_t& other) const;
	bool operator<=(const range_t& other) const;
	bool operator>=(const range_t& other) const;

	bool contains(const range_t& other) const;
	bool is_contained_by(const range_t& other) const;
	bool overlaps(const range_t& other) const;
	bool touches(const range_t& other) const;

	size_t size() const noexcept;

	std::uintptr_t begin;
	std::uintptr_t end;
};
}	 // namespace memory
