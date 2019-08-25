#include "gfx/buffer.h"
#include "gfx/context.h"
#ifdef PE_VULKAN
#include "vk/context.h"
#include "vk/buffer.h"
#endif
#ifdef PE_GLES
#include "gles/context.h"
#include "gles/buffer.h"
#endif
#include "data/buffer.h"

using namespace core;
using namespace core::gfx;
using namespace core::resource;

buffer::buffer(core::resource::handle<value_type>& handle) : m_Handle(handle){};
buffer::buffer(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
			   handle<context> context, handle<data::buffer> data)
{
	switch(context->backend())
	{
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::buffer>(metaData.uid, context->resource().get<core::ivk::context>(),
														  data);
		break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles: m_Handle << cache.create_using<core::igles::buffer>(metaData.uid, data); break;
#endif
	}
}

buffer::buffer(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
			   handle<context> context, handle<data::buffer> data, handle<buffer> staging)
{
	switch(context->backend())
	{
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::buffer>(metaData.uid, context->resource().get<core::ivk::context>(),
														  data, staging->resource().get<core::ivk::buffer>());
		break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles: m_Handle << cache.create_using<core::igles::buffer>(metaData.uid, data); break;
#endif
	}
}

buffer::~buffer() {}


const core::data::buffer& buffer::data() const noexcept
{
#ifdef PE_GLES
	if(m_Handle.contains<igles::buffer>())
	{
		return m_Handle.value<igles::buffer>().data();
	}
#endif
#ifdef PE_VULKAN
	if(m_Handle.contains<ivk::buffer>())
	{
		return m_Handle.value<ivk::buffer>().data().value();
		;
	}
#endif
}


[[nodiscard]] std::optional<memory::segment> buffer::reserve(uint64_t size)
{
#ifdef PE_GLES
	if(m_Handle.contains<core::igles::buffer>())
	{
		return m_Handle.value<core::igles::buffer>().allocate(size);
	}
#endif
#ifdef PE_VULKAN
	if(m_Handle.contains<core::ivk::buffer>())
	{
		return m_Handle.value<core::ivk::buffer>().reserve(size);
	}
#endif
}
[[nodiscard]] psl::array<std::pair<memory::segment, memory::range>> buffer::reserve(psl::array<uint64_t> sizes,
																					bool optimize)
{
#ifdef PE_GLES
	if(m_Handle.contains<core::igles::buffer>())
	{
		return m_Handle.value<core::igles::buffer>().allocate(sizes, optimize);
	}
#endif
#ifdef PE_VULKAN
	if(m_Handle.contains<core::ivk::buffer>())
	{
		return m_Handle.value<core::ivk::buffer>().reserve(sizes, optimize);
	}
#endif
}

bool buffer::deallocate(memory::segment& segment)
{
#ifdef PE_GLES
	if(m_Handle.contains<core::igles::buffer>())
	{
		return m_Handle.value<core::igles::buffer>().deallocate(segment);
	}
#endif
#ifdef PE_VULKAN
	if(m_Handle.contains<core::ivk::buffer>())
	{
		return m_Handle.value<core::ivk::buffer>().deallocate(segment);
	}
#endif
}
bool buffer::copy_from(const buffer& other, psl::array<core::gfx::memory_copy> ranges)
{
#ifdef PE_GLES
	if(m_Handle.contains<core::igles::buffer>())
	{
		return m_Handle.value<core::igles::buffer>().copy_from(other.resource().value<core::igles::buffer>(), ranges);
	}
#endif
#ifdef PE_VULKAN
	if(m_Handle.contains<core::ivk::buffer>())
	{
		psl::array<vk::BufferCopy> buffer_ranges;
		std::transform(std::begin(ranges), std::end(ranges), std::back_inserter(buffer_ranges),
					   [](const gfx::memory_copy& range) {
						   return vk::BufferCopy{range.source_offset, range.destination_offset, range.size};
					   });

		return m_Handle.value<core::ivk::buffer>().copy_from(other.resource().value<core::ivk::buffer>(),
															 buffer_ranges);
	}
#endif
}

bool buffer::commit(const psl::array<core::gfx::commit_instruction>& instructions)
{
#ifdef PE_GLES
	if(m_Handle.contains<core::igles::buffer>())
	{
		return m_Handle.value<core::igles::buffer>().commit(instructions);
	}
#endif
#ifdef PE_VULKAN
	if(m_Handle.contains<core::ivk::buffer>())
	{
		return m_Handle.value<core::ivk::buffer>().commit(instructions);
	}
#endif
}

size_t buffer::free_size() const noexcept
{
	size_t available = std::numeric_limits<size_t>::max();
#ifdef PE_GLES
	if(m_Handle.contains<core::igles::buffer>())
		available = std::min(available, m_Handle.value<core::igles::buffer>().free_size());
#endif
#ifdef PE_VULKAN
	if(m_Handle.contains<core::ivk::buffer>())
		available = std::min(available, m_Handle.value<core::ivk::buffer>().free_size());
#endif
	return available;
}