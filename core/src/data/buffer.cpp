#include "data/buffer.h"

using namespace psl;
using namespace core::data;

buffer::buffer(const UID& uid, core::resource::cache& cache, vk::BufferUsageFlags usage,
			   vk::MemoryPropertyFlags memoryPropertyFlags,
			   memory::region&& memory_region)
	: m_Region(std::move(memory_region)), m_Usage(usage), m_MemoryPropertyFlags(memoryPropertyFlags)
{
}

buffer::~buffer()
{
}


std::optional<memory::segment> buffer::allocate(size_t size)
{ 
	if(auto segm = m_Region.allocate(size); segm)
	{
		m_Segments.push_back(segm.value());
		return segm;
	}
	return std::optional<memory::segment>{}; 
}
bool buffer::deallocate(memory::segment& segment) 
{ 
	const memory::range range{segment.range()};
	auto find_it = std::find_if(std::begin(m_Segments), std::end(m_Segments), [&range](const memory::segment& entry) { return entry.range() == range; });
	if (find_it == std::end(m_Segments))
	{
		core::data::log->critical("could not erase the range {0} - {1}", range.begin, range.end);
		return false;
	}

	if(m_Region.deallocate(segment))
	{
		std::rotate(find_it, std::next(find_it), std::end(m_Segments));
		m_Segments.erase(std::prev(std::end(m_Segments)), std::end(m_Segments));
		return true;
	}
	else
	{
		core::data::log->warn("tried to erase a range {0} - {1} that was not present in the buffer", range.begin, range.end);
	}
	return false; 
}

size_t buffer::size() const
{
	return m_Region.size();
	//return std::accumulate(std::begin(m_Segments), std::end(m_Segments), (size_t)0u,
	//					   [](size_t sum, const memory::segment& segment) { return sum + segment.range().size(); });
}
vk::BufferUsageFlags buffer::usage() const { return m_Usage.value; }
vk::MemoryPropertyFlags buffer::memoryPropertyFlags() const { return m_MemoryPropertyFlags.value; }
const memory::region& buffer::region() const { return m_Region; }
const std::vector<memory::segment>& buffer::segments() const { return m_Segments; };
