#include "stdafx_psl.h"
#include "memory/allocator.h"
#include "memory/region.h"
#include "memory/range.h"
#include <algorithm>
using namespace memory;


bool allocator_base::owns(const memory::segment& segment) const noexcept
{
	return segment.range().begin >= (std::uintptr_t)m_Region->data() && segment.range().end <= (std::uintptr_t)m_Region->data() + (std::uintptr_t)m_Region->size() && get_owns(segment);
}

size_t allocator_base::alignment() const noexcept
{
	return (m_Region) ? m_Region->alignment() : 0u;
}

bool allocator_base::deallocate(segment& segment)
{
	range local = segment.range();
	if(!m_Region->range().contains(segment.range()) || !do_deallocate(segment))
		return false;

	if(is_physically_backed())	// zero-reset
		std::memset((void*)local.begin, 0, local.size());

	static range r{0u, 0u};
	segment = memory::segment{r, false};

	return true;
};

bool allocator_base::commit(const range& range)
{
	return m_Region->commit(range);
}

void allocator_base::compact()
{
	do_compact(m_Region);
}

std::vector<range> allocator_base::committed()
{
	return get_committed();
}
std::vector<range> allocator_base::available()
{
	return get_available();
}

void default_allocator::initialize(region* region)
{
	m_Free.emplace_back((std::uintptr_t)(region->data()), (std::uintptr_t)(region->data()) + region->size());
}

std::optional<segment> default_allocator::do_allocate(region* region, std::size_t bytes)
{
	auto base_offset = region->data();
	auto mod = bytes % region->alignment();
	bytes = (mod)? bytes + region->alignment() - mod : bytes;
	auto index = 0u;
	for(auto& free : m_Free)
	{
		++index;
		if (free.size() >= bytes)
		{
			auto begin = free.begin;
			mod = begin % region->alignment();
			begin = (mod) ? begin + region->alignment() - mod : begin;
			if (free.end < begin + bytes)
				continue;

			range r{begin, begin + bytes};
			if(commit(r))
			{
				if (free.begin == begin)
				{
					free.begin += bytes;
				}
				else
				{
					auto oldBegin = free.begin;
					auto oldEnd = begin;
					free.begin = r.end;
					auto it = std::next(std::begin(m_Free), index - 1);
					m_Free.emplace(it, memory::range{ oldBegin, oldEnd });
				}
				return std::optional<segment>{std::in_place_t{}, m_Committed.emplace_back(r), is_physically_backed()};
			}
		}
	}
	
	return { };
}
bool default_allocator::do_deallocate(segment& segment)
{
	range r = segment.range();
	for(auto& committed : m_Committed)
	{
		if(committed.contains(r))
		{
			if(committed.begin == r.begin && committed.end == r.end)
			{
				m_Committed.remove(committed);
			}
			else if(committed.begin == r.begin)
			{
				committed.begin += r.size();
			}
			else if(committed.end == r.end)
			{
				committed.end -= r.size();
			}
			else
			{
				m_Committed.emplace_back(r.end, committed.end);
				committed.end = r.begin;
			}
			break;
		}
	}
	for(auto& free : m_Free)
	{
		if(free.begin == r.end)
		{
			free.begin = r.begin;
			goto collapse;
		}
		else if(free.end == r.begin)
		{
			free.end = r.end;
			goto collapse;
		}
	}
	m_Free.emplace_back(r);

	collapse:
	for(auto it = std::begin(m_Free); it != std::end(m_Free); ++it)
	{
		for(auto it_next = std::begin(m_Free); it_next != std::end(m_Free); ++it_next)
		{
			if(it == it_next)
				continue;

			if(it->end == it_next->begin)
			{
				it->end = it_next->end;
				m_Free.erase(it_next);
				goto end;
			}
			else if(it->begin == it_next->end)
			{
				it->begin = it_next->end;
				m_Free.erase(it_next);
				goto end;
			}
		}
	}

end:

	return true;
}
std::vector<range> default_allocator::get_committed()
{
	if(m_Committed.size() == 0)
		return {};
	if(m_Free.size() == 0)
		return std::vector<range>{ { std::begin(m_Committed)->begin, std::end(m_Committed)->end }};

	std::vector<range> all_ranges{};
	
	if(std::begin(m_Free)->begin > std::begin(m_Committed)->end)
	{
		all_ranges.emplace_back(std::begin(m_Committed)->begin, std::begin(m_Free)->begin);
	}
	for(std::list<range>::iterator it = std::begin(m_Free), next = std::next(it, 1); next != std::end(m_Free); ++it, ++next)
	{
		all_ranges.emplace_back(it->end, next->begin);
	}

	if(m_Free.size() > 0 && m_Committed.size() > 0 && std::prev(std::end(m_Free), 1)->end < std::prev(std::end(m_Committed),1)->begin)
	{
		all_ranges.emplace_back(std::prev(std::end(m_Free),1)->end, std::prev(std::end(m_Committed),1)->end);
	}
	return all_ranges;
}
std::vector<range> default_allocator::get_available()
{
	m_Free.sort([](const range& first, const range& second) { return first.begin < second.begin; });
	return std::vector<range>{std::begin(m_Free), std::end(m_Free)};
}

void default_allocator::do_compact(region* region)
{
	m_Free.sort([](const range& first, const range& second) { return first.begin < second.begin; });
	m_Committed.sort([](const range& first, const range& second) { return first.begin < second.begin; });

	auto free_it = std::begin(m_Free);

	auto commit_it = std::begin(m_Committed);
	while(free_it != std::end(m_Free) && commit_it != std::end(m_Committed))
	{
		commit_it = std::find_if(commit_it, std::end(m_Committed), [&free_it](const range& element) { return element > *free_it; });

		while(commit_it != std::end(m_Committed) && free_it->size() >= commit_it->size())
		{
			auto old_range = *commit_it;
			memory::range new_range{free_it->begin, free_it->begin + commit_it->size()};

			if(is_physically_backed())
				std::memcpy((void*)(free_it->begin), (void*)(old_range.begin), old_range.size());

			commit_it->begin = new_range.begin;
			commit_it->end = new_range.end;
			free_it->begin = new_range.end;
			if(old_range.begin == free_it->end)
			{
				free_it->end = old_range.end;
				if(auto next = std::next(free_it, 1); next != std::end(m_Free) && free_it->end == next->begin)
				{
					free_it->end = next->end;
					m_Free.erase(next);
				}
			}
			else
			{
				auto insert_loc = std::find_if(free_it, std::end(m_Free), [&old_range](const range& element) { return old_range.begin == element.end;});
				if(insert_loc != std::end(m_Free))
				{
					insert_loc->end = old_range.end;
				}
				else
				{
					auto loc = std::find_if(free_it, std::end(m_Free), [&old_range](const range& element) { return old_range.begin > element.begin; });
					m_Free.emplace(std::prev(loc ,1), old_range);
				}
			}
			commit_it = std::next(commit_it, 1);
		}
		free_it = std::next(free_it, 1);
	}
	m_Free.erase(std::remove_if(std::begin(m_Free), std::end(m_Free), [](const range& element) { return element.size() == 0; }), std::end(m_Free));
}

bool default_allocator::get_owns(const memory::segment& segment) const noexcept
{
	return std::find_if(std::begin(m_Committed), std::end(m_Committed), [&segment](const memory::range& range)
						{
							return &segment.range() == &range;
						}) != std::end(m_Committed);
}

void block_allocator::initialize(region* region)
{
	const size_t size = (size_t)std::floor((double)region->size() / (double)m_BlockSize);
	for (auto i = 0u; i < size; ++i)
	{
		m_Free.emplace(i);
	}
	
	m_Ranges.resize((size_t)std::floor((double)region->size() / (double)m_BlockSize));
}


std::optional<segment> block_allocator::do_allocate(region* region, std::size_t bytes)
{
	if (m_Free.size() == 0)
		return {};

	auto index = m_Free.top();

	m_Ranges[index].begin = (std::uintptr_t)(region->data()) + index * m_BlockSize;
	m_Ranges[index].end = m_Ranges[index].begin + m_BlockSize;

	if (commit(m_Ranges[index]))
	{
		m_Free.pop();
		return std::optional<segment>{std::in_place_t{}, m_Ranges[index], is_physically_backed()};
	}
	return {};
}

bool block_allocator::do_deallocate(segment& segment)
{
	auto index = ((std::uintptr_t)(&segment.range()) - (std::uintptr_t)(m_Ranges.data())) / m_BlockSize;
	m_Free.push(index);
	m_Ranges[index].begin = 0;
	m_Ranges[index].end = 0;
	return true;
}

std::vector<range> block_allocator::get_committed()
{
	auto copy = m_Ranges;
	auto it = std::remove_if(std::begin(copy), std::end(copy), [](const memory::range& range)
	{
		return range.end == 0 && range.begin == 0;
	});
	copy.erase(it, std::end(copy));

	return copy;
}

std::vector<range> block_allocator::get_available()
{
	auto copy = m_Ranges;

	auto it = std::remove_if(std::begin(copy), std::end(copy), [](const memory::range& range)
	{
		return range.end == 0 && range.begin == 0;
	});
	copy.erase(std::begin(copy), it);

	return copy;
}

bool block_allocator::get_owns(const memory::segment& segment) const noexcept
{
	return segment.range().size() == m_BlockSize &&		// correct size
		m_Free.size() != m_Ranges.size() &&				// there are actual allocations
		segment.range().begin - m_Ranges[0].begin % m_BlockSize == 0 &&	// it is aligned correctly
		std::find_if(std::begin(m_Ranges), std::end(m_Ranges), [&segment](const memory::range& range)	// expensive search
						{
							return &segment.range() == &range;
						}) != std::end(m_Ranges);
}