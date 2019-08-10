#include "gles/buffer.h"
#include "data/buffer.h"
#include "logging.h"

using namespace core::igles;
using namespace core::gfx;
using namespace core;

buffer::buffer(const psl::UID& uid, core::resource::cache& cache,
			   core::resource::handle<core::data::buffer> buffer_data)
	: m_BufferDataHandle(buffer_data), m_UID(uid)
{
	glGenBuffers(1, &m_Buffer);
	m_BufferType = to_gles(to_memory_type(buffer_data->usage()));
	glBindBuffer(m_BufferType, m_Buffer);

	auto draw_type = GL_STREAM_DRAW;
	switch(buffer_data->write_frequency())
	{
	case memory_write_frequency::per_frame: draw_type = GL_STREAM_DRAW; break;
	case memory_write_frequency::sometimes: draw_type = GL_DYNAMIC_DRAW; break;
	case memory_write_frequency::almost_never: draw_type = GL_STATIC_DRAW; break;
	}

	// we pre-allocate a buffer, but don't feed it data yet.
	glBufferData(m_BufferType, buffer_data->size(), nullptr, draw_type);
	std::vector<core::gfx::memory_copy> commands;
	for(const auto& segment : buffer_data->segments())
	{
		commands.emplace_back(
			core::gfx::memory_copy{segment.range().begin, segment.range().begin, segment.range().size()});
	}
	set(buffer_data->region().data(), commands);
	glBindBuffer(m_BufferType, 0);
}

buffer::~buffer() { glDeleteBuffers(1, &m_Buffer); }


bool buffer::set(const void* data, std::vector<core::gfx::memory_copy> commands)
{
	glBindBuffer(m_BufferType, m_Buffer);
	for(auto command : commands)
	{
		// glBufferSubData(m_BufferType, command.destination_offset, command.size,
		//				(void*)((std::uintptr_t)data + command.source_offset));
		auto ptr = glMapBufferRange(m_BufferType, command.destination_offset, command.size, GL_MAP_WRITE_BIT);
		memcpy(ptr, (void*)((std::uintptr_t)data + command.source_offset), command.size);
	}
	glUnmapBuffer(m_BufferType);
	return true;
}

bool buffer::set(std::vector<core::gfx::memory_copy> commands)
{
	glBindBuffer(m_BufferType, m_Buffer);
	for(auto command : commands)
	{
		// glBufferSubData(m_BufferType, command.destination_offset, command.size,
		//				(void*)((std::uintptr_t)data + command.source_offset));
		auto ptr = glMapBufferRange(m_BufferType, command.destination_offset, command.size, GL_MAP_WRITE_BIT);
		memcpy(ptr, (void*)(command.source_offset), command.size);
		glUnmapBuffer(m_BufferType);
	}
	glBindBuffer(m_BufferType, 0);
	return true;
}

std::optional<memory::segment> buffer::allocate(size_t size) { return m_BufferDataHandle->allocate(size); }
std::vector<std::pair<memory::segment, memory::range>> buffer::allocate(psl::array<size_t> sizes, bool optimize)
{
	PROFILE_SCOPE(core::profiler)
	size_t totalSize = std::accumulate(std::next(std::begin(sizes)), std::end(sizes), sizes[0],
									   [](size_t sum, const size_t& element) { return sum + element; });
	std::vector<std::pair<memory::segment, memory::range>> result;

	// todo: low priority
	// this should check for biggest continuous space in the memory::region and split like that
	if(optimize)
	{
		auto segment = m_BufferDataHandle->allocate(totalSize);
		if(segment)
		{
			result.resize(sizes.size());
			size_t accOffset = 0u;
			for(auto i = 0u; i < sizes.size(); ++i)
			{
				result[i].first  = segment.value();
				result[i].second = memory::range{accOffset, accOffset + sizes[i]};
				accOffset += sizes[i];
			}
			return result;
		}
	}

	// either optimization failed, or we aren't optimizing
	result.reserve(sizes.size());
	for(const auto& size : sizes)
	{
		if(auto segment = m_BufferDataHandle->allocate(size); segment)
		{
			auto& res  = result.emplace_back();
			res.first  = segment.value();
			res.second = memory::range{0, size};
		}
		else
		{
			goto failure;
		}
	}

	return result;

	// in case something goes bad, roll back the already completed allocations
failure:
	for(auto& segment : result)
	{
		m_BufferDataHandle->deallocate(segment.first);
	}
	return {};
}

bool buffer::deallocate(memory::segment& segment) { return m_BufferDataHandle->deallocate(segment); }

bool buffer::copy_from(const buffer& other, const psl::array<core::gfx::memory_copy>& ranges)
{
	auto totalsize = std::accumulate(ranges.begin(), ranges.end(), 0,
									 [&](int sum, const gfx::memory_copy& region) { return sum + (int)region.size; });

	core::igles::log->info("copying buffer {0} into {1} for a total size of {2} using {3} copy instructions",
						   utility::to_string(other.m_UID), utility::to_string(m_UID), totalsize, ranges.size());

	for(const auto& region : ranges)
	{
		core::igles::log->info("srcOffset | dstOffset | size : {0} | {1} | {2}", region.source_offset,
							   region.destination_offset, region.size);
	}

	glBindBuffer(GL_COPY_READ_BUFFER, other.m_Buffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, m_Buffer);
	for(const auto& region : ranges)
	{
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, region.source_offset,
							region.destination_offset, region.size);
	}

	return true;
}