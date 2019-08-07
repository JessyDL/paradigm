#include "gles/buffer.h"
#include "data/buffer.h"

using namespace core::igles;
using namespace core::gfx;

buffer::buffer(const psl::UID& uid, core::resource::cache& cache,
			   core::resource::handle<core::data::buffer> buffer_data)
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