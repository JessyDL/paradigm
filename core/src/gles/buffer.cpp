#include "gles/buffer.hpp"
#include "data/buffer.hpp"
#include "gles/conversion.hpp"
#include "logging.hpp"

using namespace core::igles;
using namespace core::gfx;
using namespace core::gfx::conversion;
using namespace core;

#define USE_BUFFER_SUBDATA

buffer_t::buffer_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<core::data::buffer_t> buffer_data) :
	m_BufferDataHandle(buffer_data),
	m_UID(metaData.uid)
{
	m_BufferType = to_gles(buffer_data->usage());


	glGenBuffers(1, &m_Buffer);
	glBindBuffer(m_BufferType, m_Buffer);

	auto draw_type = GL_STREAM_DRAW;
	switch(buffer_data->write_frequency())
	{
	case memory_write_frequency::per_frame:
		draw_type = GL_STREAM_DRAW;
		break;
	case memory_write_frequency::sometimes:
		draw_type = GL_DYNAMIC_DRAW;
		break;
	case memory_write_frequency::almost_never:
		draw_type = GL_STATIC_DRAW;
		break;
	}

	// we pre-allocate a buffer, but don't feed it data yet.
	glBufferData(m_BufferType, buffer_data->size(), nullptr, draw_type);
	auto error = glGetError();
	std::vector<core::gfx::memory_copy> commands;
	for(const auto& segment : buffer_data->segments())
	{
		commands.emplace_back(
		  core::gfx::memory_copy {segment.range().begin, segment.range().begin, segment.range().size()});
	}
	set(buffer_data->region().data(), commands);
	error = glGetError();
	glBindBuffer(m_BufferType, 0);
}

buffer_t::~buffer_t() { glDeleteBuffers(1, &m_Buffer); }


bool buffer_t::set(const void* data, std::vector<core::gfx::memory_copy> commands)
{
	glBindBuffer(GL_COPY_WRITE_BUFFER, m_Buffer);
	auto error = glGetError();
	for(auto command : commands)
	{
#ifdef USE_BUFFER_SUBDATA
		glBufferSubData(m_BufferType,
						command.destination_offset,
						command.size,
						(void*)((std::uintptr_t)data + command.source_offset));
#else
		auto ptr = glMapBufferRange(GL_COPY_WRITE_BUFFER, command.destination_offset, command.size, GL_MAP_WRITE_BIT);
		memcpy(ptr, (void*)((std::uintptr_t)data + command.source_offset), command.size);
		error = glGetError();
		glUnmapBuffer(GL_COPY_WRITE_BUFFER);
#endif
		error = glGetError();
	}
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	return true;
}

bool buffer_t::set(std::vector<core::gfx::memory_copy> commands)
{
	glBindBuffer(m_BufferType, m_Buffer);
	auto error = glGetError();
	for(auto command : commands)
	{
#ifdef USE_BUFFER_SUBDATA
		glBufferSubData(
		  m_BufferType, command.destination_offset, command.size, (void*)((std::uintptr_t)command.source_offset));
#else
		auto ptr = glMapBufferRange(m_BufferType, command.destination_offset, command.size, GL_MAP_WRITE_BIT);
		error	 = glGetError();
		memcpy(ptr, (void*)(command.source_offset), command.size);
		glUnmapBuffer(m_BufferType);
#endif
		error = glGetError();
	}
	glBindBuffer(m_BufferType, 0);
	return true;
}

std::optional<memory::segment> buffer_t::allocate(size_t size) { return m_BufferDataHandle->allocate(size); }
std::vector<std::pair<memory::segment, memory::range_t>> buffer_t::allocate(psl::array<size_t> sizes, bool optimize)
{
	PROFILE_SCOPE(core::profiler)
	size_t totalSize =
	  std::accumulate(std::next(std::begin(sizes)), std::end(sizes), sizes[0], [](size_t sum, const size_t& element) {
		  return sum + element;
	  });
	std::vector<std::pair<memory::segment, memory::range_t>> result;

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
				result[i].first	 = segment.value();
				result[i].second = memory::range_t {accOffset, accOffset + sizes[i]};
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
			res.second = memory::range_t {0, size};
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

bool buffer_t::deallocate(memory::segment& segment) { return m_BufferDataHandle->deallocate(segment); }

bool buffer_t::copy_from(const buffer_t& other, const psl::array<core::gfx::memory_copy>& ranges)
{
	auto totalsize = std::accumulate(
	  ranges.begin(), ranges.end(), 0, [&](int sum, const gfx::memory_copy& region) { return sum + (int)region.size; });

	core::igles::log->info("copying buffer {0} into {1} for a total size of {2} using {3} copy instructions",
						   utility::to_string(other.m_UID),
						   utility::to_string(m_UID),
						   totalsize,
						   ranges.size());

	for(const auto& region : ranges)
	{
		core::igles::log->info("srcOffset | dstOffset | size : {0} | {1} | {2}",
							   region.source_offset,
							   region.destination_offset,
							   region.size);
	}

	glBindBuffer(GL_COPY_READ_BUFFER, other.m_Buffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, m_Buffer);
	for(const auto& region : ranges)
	{
		glCopyBufferSubData(
		  GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, region.source_offset, region.destination_offset, region.size);
	}
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

	return true;
}


bool buffer_t::commit(const psl::array<core::gfx::commit_instruction>& instructions)
{
	glBindBuffer(GL_COPY_WRITE_BUFFER, m_Buffer);
	for(const auto& instruction : instructions)
	{
		std::uintptr_t offset = instruction.segment.range().begin -
								(std::uintptr_t)m_BufferDataHandle->region().data() +
								instruction.sub_range.value_or(memory::range_t {}).begin;
#ifdef USE_BUFFER_SUBDATA
		glBufferSubData(GL_COPY_WRITE_BUFFER, offset, instruction.size, (void*)((std::uintptr_t)instruction.source));
#else
		auto ptr = glMapBufferRange(GL_COPY_WRITE_BUFFER, offset, instruction.size, GL_MAP_WRITE_BIT);
		memcpy(ptr, (void*)(instruction.source), instruction.size);
		glUnmapBuffer(GL_COPY_WRITE_BUFFER);

#endif
	}
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	return true;
}

size_t buffer_t::free_size() const noexcept
{
	auto available = m_BufferDataHandle->region().allocator()->available();
	return std::accumulate(std::next(std::begin(available)),
						   std::end(available),
						   available[0].size(),
						   [](size_t sum, const memory::range_t& element) { return sum + element.size(); });
}