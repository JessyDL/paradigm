#pragma once
#include "fwd/gfx/buffer.h"
#include "gfx/types.h"
#include "psl/array.hpp"
#include "psl/memory/range.hpp"
#include "psl/memory/region.hpp"
#include "psl/memory/segment.hpp"
#include "resource/resource.hpp"
#include <optional>

namespace core::data
{
	class buffer_t;
}

namespace core::gfx
{
	class context;
	class buffer_t
	{
	  public:
#ifdef PE_VULKAN
		explicit buffer_t(core::resource::handle<core::ivk::buffer_t>& handle);
#endif
#ifdef PE_GLES
		explicit buffer_t(core::resource::handle<core::igles::buffer_t>& handle);
#endif

		buffer_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<core::gfx::context> context,
			   core::resource::handle<core::data::buffer_t> data);

		buffer_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<core::gfx::context> context,
			   core::resource::handle<core::data::buffer_t> data,
			   core::resource::handle<core::gfx::buffer_t> staging);
		~buffer_t();

		buffer_t(const buffer_t& other)		= delete;
		buffer_t(buffer_t&& other) noexcept = delete;
		buffer_t& operator=(const buffer_t& other) = delete;
		buffer_t& operator=(buffer_t&& other) noexcept = delete;


		const core::data::buffer_t& data() const;

		[[nodiscard]] std::optional<memory::segment> reserve(uint64_t size);
		[[nodiscard]] psl::array<std::pair<memory::segment, memory::range_t>> reserve(psl::array<uint64_t> sizes,
																					bool optimize = false);
		bool deallocate(memory::segment& segment);

		bool copy_from(const buffer_t& other, psl::array<core::gfx::memory_copy> ranges);
		bool commit(const psl::array<core::gfx::commit_instruction>& instructions);

		size_t free_size() const noexcept;

		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<buffer_t, backend>> resource() const noexcept
		{
#ifdef PE_VULKAN
			if constexpr(backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
			if constexpr(backend == graphics_backend::gles) return m_GLESHandle;
#endif
		};

	  private:
		core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
		core::resource::handle<core::ivk::buffer_t> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::buffer_t> m_GLESHandle;
#endif
	};

	struct shader_buffer_binding
	{
		shader_buffer_binding(core::resource::cache_t& cache,
							  const core::resource::metadata& metaData,
							  psl::meta::file* metaFile,
							  core::resource::handle<buffer_t> buffer,
							  size_t size,
							  size_t alignment = 4);
		~shader_buffer_binding();
		core::resource::handle<buffer_t> buffer;
		memory::segment segment;
		memory::region region;
	};
}	 // namespace core::gfx