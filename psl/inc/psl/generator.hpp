#pragma once
#include <stdexcept>	// std::runtime_error

#include <algorithm>
#include <cstdint>	  // uint32_t
#include <cstdio>	 // For printf(). Remove if you don't need the PrintRanges() function (mostly for debugging anyway).
#include <cstdlib>
#include <cstring>
#include <vector>
#undef max
#undef min

namespace psl {
template <typename T = uint32_t>
class generator {
  private:
	struct range {
		range() = default;
		range(T first, T last) : first(first), last(last) {};
		T& begin() noexcept { return first; }
		T& end() noexcept { return last; }
		const T& begin() const noexcept { return first; }
		const T& end() const noexcept { return last; }

		bool operator<=(const range& other) const noexcept { return first <= other.first; }
		bool operator>=(const range& other) const noexcept { return first >= other.first; }
		bool operator<(const range& other) const noexcept { return first < other.first; }
		bool operator>(const range& other) const noexcept { return first > other.first; }
		bool operator==(const range& other) const noexcept { return first == other.first; }
		bool operator!=(const range& other) const noexcept { return first != other.first; }
		bool overlaps(const range& other) const noexcept {
			return (other.first >= first && other.first < last) || (other.last > first && other.last <= last);
		}
		T first {0};
		T last {0};

		T size() const noexcept { return last - first; };
	};

	std::vector<range> m_FreeRanges {};	   // Sorted array of ranges of free IDs
	T m_Max {0};

  public:
	explicit generator(T max) : m_FreeRanges({range {0, max}}), m_Max(max) {}

	generator() : generator(std::numeric_limits<T>::max()) {};

	generator& operator=(const generator&) = default;
	generator(const generator&)			   = default;

	generator(generator&& other) : m_FreeRanges(std::move(other.m_FreeRanges)), m_Max(other.m_Max) {}

	generator& operator=(generator&& other) {
		if(this != &other) {
			m_FreeRanges = std::move(other.m_FreeRanges);
			m_Max		 = other.m_Max;
		}
		return *this;
	}

	T capacity() const noexcept { return m_Max; }
	T size() const noexcept { return capacity() - available(); }
	T available() const noexcept {
		T acc {0};
		for(const auto& range : m_FreeRanges) {
			acc += range.size();
		}
		return acc;
	};


	T create(T count = 1) {
		if(count == 0)
			throw std::runtime_error("there should be at least 1 ID created");
		if(m_FreeRanges.size() == 0)
			throw std::runtime_error("out of available ID's");

		auto i = 0;
		for(auto& r : m_FreeRanges) {
			if(r.size() >= count) {
				auto result = r.first;
				r.first += count;

				if(r.size() == 0)
					m_FreeRanges.erase(std::next(std::begin(m_FreeRanges), i));

				return result;
			}
			++i;
		}

		throw std::runtime_error("out of available ID's");
	}

	std::vector<std::pair<T, T>> create_multi(T count = 1) {
		std::vector<std::pair<T, T>> result {};
		for(auto i = 0; i < m_FreeRanges.size();) {
			auto& r		 = m_FreeRanges[i];
			auto consume = std::min(r.size(), count);
			count -= consume;

			result.emplace_back(std::pair<T, T> {r.first, r.first + consume});
			r.first += consume;
			if(r.size() == 0)
				m_FreeRanges.erase(std::next(std::begin(m_FreeRanges), i));
			else
				++i;

			if(count == 0)
				break;
		}
		if(count > 0) {
			for(auto& r : result) destroy(r.first, r.second - r.first);

			throw std::runtime_error("out of available ID's");
		}
		return result;
	}

	bool try_create(T& out, T count = 1) {
		if(m_FreeRanges.size() == 0 || count == 0) {
			return false;
		}

		auto i = 0;
		for(auto& r : m_FreeRanges) {
			if(r.size() >= count) {
				out = r.first;
				r.first += count;

				if(r.size() == 0)
					m_FreeRanges.erase(std::next(std::begin(m_FreeRanges), i));

				return true;
			}
			++i;
		}

		return false;
	}

	bool destroy(T id, T count = 1) {
		if(count == 0)
			return false;
		range r {id, id + count};
		auto insert_loc = std::upper_bound(std::begin(m_FreeRanges), std::end(m_FreeRanges), r);

		if((insert_loc != std::begin(m_FreeRanges) && std::prev(insert_loc)->overlaps(r)) ||
		   (insert_loc != std::end(m_FreeRanges) && insert_loc->overlaps(r))) {
			return false;
		}

		size_t index = std::distance(std::begin(m_FreeRanges), m_FreeRanges.insert(insert_loc, r));

		for(size_t i = ((index > 0) ? index - 1 : index), end = std::min(index + 1, m_FreeRanges.size()); i < end;) {
			if(m_FreeRanges.size() <= i + 1)
				break;
			if(m_FreeRanges[i].last == m_FreeRanges[i + 1].first) {
				m_FreeRanges[i].last = m_FreeRanges[i + 1].last;
				m_FreeRanges.erase(std::next(std::begin(m_FreeRanges), i + 1));
				continue;
			}
			++i;
		}
		return true;
	}

	bool valid(T id, T count = 1) const noexcept {
		range r {id, id + count};
		auto insert_loc = std::upper_bound(std::begin(m_FreeRanges), std::end(m_FreeRanges), r);
		if((insert_loc != std::begin(m_FreeRanges) && std::prev(insert_loc)->overlaps(r)) ||
		   (insert_loc != std::end(m_FreeRanges) && insert_loc->overlaps(r))) {
			return false;
		}

		return true;
	}

	T largest_continuous_range() const noexcept {
		range r {};
		for(const auto& range : m_FreeRanges) {
			if(range.size() > r.size())
				r = range;
		}
		return r;
	}

	bool resize(T size) {
		if(size == m_Max)
			return true;
		if(size > m_Max) {
			if(m_FreeRanges.size() > 0) {
				m_FreeRanges[m_FreeRanges.size() - 1].last = size;
			} else {
				m_FreeRanges.emplace_back(range {m_Max, size});
			}
			m_Max = size;
			return true;
		} else if(m_FreeRanges.size() > 0 && m_FreeRanges[m_FreeRanges.size() - 1].begin() >= size) {
			m_FreeRanges[m_FreeRanges.size() - 1].last = size;

			m_Max = size;
			if(m_FreeRanges[m_FreeRanges.size() - 1].size() == 0)
				m_FreeRanges.erase(std::prev(std::end(m_FreeRanges)));
			return true;
		}

		return false;
	}
};


template <class T = uint32_t>
class id_generator {
  private:
	struct Range {
		T m_First;
		T m_Last;
	};

	Range* m_Ranges {nullptr};	  // Sorted array of ranges of free IDs
	T m_Count;					  // Number of ranges in list
	T m_Capacity;				  // Total capacity of range list
	T m_MaxID;

  public:
	explicit id_generator(T max_id) {
		// Start with a single range, from 0 to max allowed ID (specified)
		auto newRange = static_cast<Range*>(::malloc(sizeof(Range)));
		if(newRange == NULL) {
			// LOG_ERROR << "Memory allocation failed";
			return;
		}
		m_Ranges			= newRange;
		m_Ranges[0].m_First = 0;
		m_Ranges[0].m_Last	= max_id;
		m_Count				= 1;
		m_Capacity			= 1;
		m_MaxID				= max_id;
	}

	id_generator() : id_generator(std::numeric_limits<T>::max()) {};

	~id_generator() {
		if(m_Ranges != nullptr)
			::free(m_Ranges);
	}

	id_generator& operator=(const id_generator&) = default;
	id_generator(const id_generator&)			 = default;

	id_generator(id_generator&& other)
		: m_Ranges(other.m_Ranges), m_Count(other.m_Count), m_Capacity(other.m_Capacity), m_MaxID(other.m_MaxID) {
		other.m_Ranges = nullptr;
	}

	id_generator& operator=(id_generator&& other) {
		if(this != &other) {
			m_Ranges   = other.m_Ranges;
			m_Count	   = other.m_Count;
			m_Capacity = other.m_Capacity;
			m_MaxID	   = other.m_MaxID;

			other.m_Ranges = nullptr;
		}
		return *this;
	}

	T GetCapacity() const { return m_MaxID; }
	bool CreateID(T& id) {
		if(m_Ranges[0].m_First <= m_Ranges[0].m_Last) {
			id = m_Ranges[0].m_First;

			// If current range is full and there is another one, that will become the new current range
			if(m_Ranges[0].m_First == m_Ranges[0].m_Last && m_Count > 1) {
				DestroyRange(0);
			} else {
				++m_Ranges[0].m_First;
			}
			return true;
		}

		// No availble ID left
		return false;
	}

	std::pair<bool, T> CreateID() {
		if(m_Ranges[0].m_First <= m_Ranges[0].m_Last) {
			T id = m_Ranges[0].m_First;

			// If current range is full and there is another one, that will become the new current range
			if(m_Ranges[0].m_First == m_Ranges[0].m_Last && m_Count > 1) {
				DestroyRange(0);
			} else {
				++m_Ranges[0].m_First;
			}
			return std::make_pair(true, id);
		}

		// No availble ID left
		return std::make_pair(false, T());
	}

	bool CreateRangeID(T& id, const T count) {
		T i = 0;
		do {
			const T range_count = 1 + m_Ranges[i].m_Last - m_Ranges[i].m_First;
			if(count <= range_count) {
				id = m_Ranges[i].m_First;

				// If current range is full and there is another one, that will become the new current range
				if(count == range_count && i + 1 < m_Count) {
					DestroyRange(i);
				} else {
					m_Ranges[i].m_First += count;
				}
				return true;
			}
			++i;
		} while(i < m_Count);

		// No range of free IDs was large enough to create the requested continuous ID sequence
		return false;
	}

	bool DestroyID(const T id) { return DestroyRangeID(id, 1); }

	bool DestroyRangeID(const T id, const T count) {
		const T end_id = id + count;

		// Binary search of the range list
		T i0 = 0;
		T i1 = m_Count - 1;

		for(;;) {
			const T i = (i0 + i1) / 2;

			if(id < m_Ranges[i].m_First) {
				// Before current range, check if neighboring
				if(end_id >= m_Ranges[i].m_First) {
					if(end_id != m_Ranges[i].m_First)
						return false;	 // Overlaps a range of free IDs, thus (at least partially) invalid IDs

					// Neighbor id, check if neighboring previous range too
					if(i > i0 && id - 1 == m_Ranges[i - 1].m_Last) {
						// Merge with previous range
						m_Ranges[i - 1].m_Last = m_Ranges[i].m_Last;
						DestroyRange(i);
					} else {
						// Just grow range
						m_Ranges[i].m_First = id;
					}
					return true;
				} else {
					// Non-neighbor id
					if(i != i0) {
						// Cull upper half of list
						i1 = i - 1;
					} else {
						// Found our position in the list, insert the deleted range here
						InsertRange(i);
						m_Ranges[i].m_First = id;
						m_Ranges[i].m_Last	= end_id - 1;
						return true;
					}
				}
			} else if(id > m_Ranges[i].m_Last) {
				// After current range, check if neighboring
				if(id - 1 == m_Ranges[i].m_Last) {
					// Neighbor id, check if neighboring next range too
					if(i < i1 && end_id == m_Ranges[i + 1].m_First) {
						// Merge with next range
						m_Ranges[i].m_Last = m_Ranges[i + 1].m_Last;
						DestroyRange(i + 1);
					} else {
						// Just grow range
						m_Ranges[i].m_Last += count;
					}
					return true;
				} else {
					// Non-neighbor id
					if(i != i1) {
						// Cull bottom half of list
						i0 = i + 1;
					} else {
						// Found our position in the list, insert the deleted range here
						InsertRange(i + 1);
						m_Ranges[i + 1].m_First = id;
						m_Ranges[i + 1].m_Last	= end_id - 1;
						return true;
					}
				}
			} else {
				// Inside a free block, not a valid ID
				return false;
			}
		}
	}

	bool IsID(const T id) const {
		// Binary search of the range list
		T i0 = 0;
		T i1 = m_Count - 1;

		for(;;) {
			const T i = (i0 + i1) / 2;

			if(id < m_Ranges[i].m_First) {
				if(i == i0)
					return true;

				// Cull upper half of list
				i1 = i - 1;
			} else if(id > m_Ranges[i].m_Last) {
				if(i == i1)
					return true;

				// Cull bottom half of list
				i0 = i + 1;
			} else {
				// Inside a free block, not a valid ID
				return false;
			}
		}
	}

	T GetAvailableIDs() const {
		T count = m_Count;
		T i		= 0;

		do {
			count += m_Ranges[i].m_Last - m_Ranges[i].m_First;
			++i;
		} while(i < m_Count);

		return count;
	}

	T GetLargestContinuousRange() const {
		T max_count = 0;
		T i			= 0;

		do {
			T count = m_Ranges[i].m_Last - m_Ranges[i].m_First + 1;
			if(count > max_count)
				max_count = count;

			++i;
		} while(i < m_Count);

		return max_count;
	}

	std::pair<Range*, T> AllRanges() const { return std::make_pair(m_Ranges, m_Count); }

	void PrintRanges() const {
		T i = 0;
		for(;;) {
			if(m_Ranges[i].m_First < m_Ranges[i].m_Last)
				printf("%u-%u", m_Ranges[i].m_First, m_Ranges[i].m_Last);
			else if(m_Ranges[i].m_First == m_Ranges[i].m_Last)
				printf("%u", m_Ranges[i].m_First);
			else
				printf("-");

			++i;
			if(i >= m_Count) {
				printf("\n");
				return;
			}

			printf(", ");
		}
	}


  private:
	void InsertRange(const T index) {
		if(m_Count >= m_Capacity) {
			m_Capacity += m_Capacity;
			// m_Ranges = (Range *)realloc(m_Ranges, m_Capacity * sizeof(Range));
			auto newRange = (Range*)realloc(m_Ranges, m_Capacity * sizeof(Range));
			if(newRange == NULL) {
				// LOG_ERROR << "Memory allocation failed";
				return;
			}
			m_Ranges = newRange;
		}

		::memmove(m_Ranges + index + 1, m_Ranges + index, (m_Count - index) * sizeof(Range));
		++m_Count;
	}

	void DestroyRange(const T index) {
		--m_Count;
		::memmove(m_Ranges + index, m_Ranges + index + 1, (m_Count - index) * sizeof(Range));
	}
};
}	 // namespace psl
