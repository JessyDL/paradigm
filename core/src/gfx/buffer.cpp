#include "gfx/buffer.hpp"
#include "gfx/context.hpp"
#ifdef PE_VULKAN
	#include "vk/buffer.hpp"
	#include "vk/context.hpp"
#endif
#ifdef PE_GLES
	#include "gles/buffer.hpp"
	#include "gles/context.hpp"
#endif
#include "data/buffer.hpp"

using namespace core;
using namespace core::gfx;
using namespace core::resource;

#ifdef PE_VULKAN
buffer_t::buffer_t(core::resource::handle<core::ivk::buffer_t>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle) {}
#endif
#ifdef PE_GLES
buffer_t::buffer_t(core::resource::handle<core::igles::buffer_t>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle) {}
#endif

buffer_t::buffer_t(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   handle<context> context,
				   handle<data::buffer_t> data)
	: m_Backend(context->backend()) {
	switch(m_Backend) {
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle =
		  cache.create_using<core::ivk::buffer_t>(metaData.uid, context->resource<graphics_backend::vulkan>(), data);
		break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::buffer_t>(metaData.uid, data);
		break;
#endif
	}
}

buffer_t::buffer_t(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   handle<context> context,
				   handle<data::buffer_t> data,
				   handle<buffer_t> staging) {
	switch(context->backend()) {
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = cache.create_using<core::ivk::buffer_t>(metaData.uid,
															 context->resource<graphics_backend::vulkan>(),
															 data,
															 staging->resource<graphics_backend::vulkan>());
		break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::buffer_t>(metaData.uid, data);
		break;
#endif
	}
}

buffer_t::~buffer_t() {}


const core::data::buffer_t& buffer_t::data() const {
#ifdef PE_GLES
	if(m_GLESHandle) {
		return m_GLESHandle->data();
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		return m_VKHandle->data().value();
		;
	}
#endif
	throw std::logic_error("core::gfx::buffer_t has no API specific buffer associated with it");
}


[[nodiscard]] std::optional<memory::segment> buffer_t::reserve(uint64_t size) {
#ifdef PE_GLES
	if(m_GLESHandle) {
		return m_GLESHandle->allocate(size);
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		return m_VKHandle->reserve(size);
	}
#endif
	throw std::logic_error("core::gfx::buffer_t has no API specific buffer associated with it");
}
[[nodiscard]] psl::array<std::pair<memory::segment, memory::range_t>> buffer_t::reserve(psl::array<uint64_t> sizes,
																						bool optimize) {
#ifdef PE_GLES
	if(m_GLESHandle) {
		return m_GLESHandle->allocate(sizes, optimize);
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		return m_VKHandle->reserve(sizes, optimize);
	}
#endif
	throw std::logic_error("core::gfx::buffer_t has no API specific buffer associated with it");
}

bool buffer_t::deallocate(memory::segment& segment) {
#ifdef PE_GLES
	if(m_GLESHandle) {
		return m_GLESHandle->deallocate(segment);
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		return m_VKHandle->deallocate(segment);
	}
#endif
	throw std::logic_error("core::gfx::buffer_t has no API specific buffer associated with it");
}
bool buffer_t::copy_from(const buffer_t& other, psl::array<core::gfx::memory_copy> ranges) {
#ifdef PE_GLES
	if(m_GLESHandle) {
		return m_GLESHandle->copy_from(other.resource<graphics_backend::gles>().value(), ranges);
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		psl::array<vk::BufferCopy> buffer_ranges;
		std::transform(
		  std::begin(ranges), std::end(ranges), std::back_inserter(buffer_ranges), [](const gfx::memory_copy& range) {
			  return vk::BufferCopy {range.source_offset, range.destination_offset, range.size};
		  });

		return m_VKHandle->copy_from(other.resource<graphics_backend::vulkan>().value(), buffer_ranges);
	}
#endif
	throw std::logic_error("core::gfx::buffer_t has no API specific buffer associated with it");
}

bool buffer_t::commit(const psl::array<core::gfx::commit_instruction>& instructions) {
#ifdef PE_GLES
	if(m_GLESHandle) {
		return m_GLESHandle->commit(instructions);
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		return m_VKHandle->commit(instructions);
	}
#endif
	throw std::logic_error("core::gfx::buffer_t has no API specific buffer associated with it");
}

size_t buffer_t::free_size() const noexcept {
	size_t available = std::numeric_limits<size_t>::max();
#ifdef PE_GLES
	if(m_GLESHandle)
		available = std::min(available, m_GLESHandle->free_size());
#endif
#ifdef PE_VULKAN
	if(m_VKHandle)
		available = std::min(available, m_VKHandle->free_size());
#endif
	return available;
}

auto align_to(size_t value, size_t alignment) {
	auto remainder = value % alignment;
	return (remainder) ? value + (alignment - remainder) : value;
};

shader_buffer_binding::shader_buffer_binding(core::resource::cache_t& cache,
											 const core::resource::metadata& metaData,
											 psl::meta::file* metaFile,
											 core::resource::handle<core::gfx::buffer_t> buffer,
											 size_t size,
											 size_t alignment)
	: buffer(buffer), segment(buffer->reserve(size).value()),
	  region(segment.range().size(),
			 align_to(alignment, buffer->data().region().alignment()),
			 new memory::default_allocator(false)) {}

shader_buffer_binding::~shader_buffer_binding() {
	buffer->deallocate(segment);
}
