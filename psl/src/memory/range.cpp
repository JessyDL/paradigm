#include "psl/memory/range.h"
#include "psl/assertions.h"

using namespace memory;

range::range(std::uintptr_t begin, std::uintptr_t end) : begin(begin), end(end) { assert(begin <= end); };

bool range::operator==(const range& other) const { return (other.begin == begin && other.end == end); }


bool range::operator!=(const range& other) const { return (other.begin != begin || other.end != end); }

bool range::operator<(const range& other) const { return (begin < other.begin); }

bool range::operator>(const range& other) const { return (begin > other.begin); }
bool range::operator<=(const range& other) const { return (begin <= other.begin); }

bool range::operator>=(const range& other) const { return (begin >= other.begin); }

bool range::contains(const range& other) const { return other.begin >= begin && other.end <= end; }

bool range::is_contained_by(const range& other) const { return begin >= other.begin && end <= other.end; }

bool range::overlaps(const range& other) const
{
	return (other.begin >= begin && other.begin <= end) || (other.end >= begin && other.end <= end);
}

bool range::touches(const range& other) const { return other.begin == end || other.end == begin; }

size_t range::size() const noexcept { return end - begin; };
