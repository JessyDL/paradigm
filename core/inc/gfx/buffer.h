#pragma once
#include "fwd/gfx/buffer.h"
#include "resource/resource.hpp"
#include "psl/array.h"
#include "psl/memory/region.h"
#include "psl/memory/segment.h"
#include "psl/memory/range.h"
#include <optional>
#include "gfx/types.h"

namespace core::data
{
	class buffer;
}

namespace core::gfx
{
	class context;
	class buffer
	{
	  public:
#ifdef PE_VULKAN
		  explicit buffer(core::resource::handle<core::ivk::buffer>& handle);
#endif
#ifdef PE_GLES
		  explicit buffer(core::resource::handle<core::igles::buffer>& handle);
#endif

		buffer(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
			   core::resource::handle<core::gfx::context> context, core::resource::handle<core::data::buffer> data);

		buffer(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
			   core::resource::handle<core::gfx::context> context, core::resource::handle<core::data::buffer> data,
			   core::resource::handle<core::gfx::buffer> staging);
		~buffer();

		buffer(const buffer& other)		= delete;
		buffer(buffer&& other) noexcept = delete;
		buffer& operator=(const buffer& other) = delete;
		buffer& operator=(buffer&& other) noexcept = delete;


		const core::data::buffer& data() const;

		[[nodiscard]] std::optional<memory::segment> reserve(uint64_t size);
		[[nodiscard]] psl::array<std::pair<memory::segment, memory::range>> reserve(psl::array<uint64_t> sizes,
																					bool optimize = false);
		bool deallocate(memory::segment& segment);

		bool copy_from(const buffer& other, psl::array<core::gfx::memory_copy> ranges);
		bool commit(const psl::array<core::gfx::commit_instruction>& instructions);

		size_t free_size() const noexcept;

		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<buffer, backend>> resource() const noexcept
		{
#ifdef PE_VULKAN
			if constexpr (backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
			if constexpr (backend == graphics_backend::gles) return m_GLESHandle;
#endif
		};

	private:
		core::gfx::graphics_backend m_Backend{ graphics_backend::undefined };
#ifdef PE_VULKAN
		core::resource::handle<core::ivk::buffer> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::buffer> m_GLESHandle;
#endif
	};

	struct shader_buffer_binding
	{
		shader_buffer_binding(core::resource::cache& cache, const core::resource::metadata& metaData,
			psl::meta::file* metaFile, core::resource::handle<buffer> buffer, size_t size,
			size_t alignment = 4);
		~shader_buffer_binding();
		core::resource::handle<buffer> buffer;
		memory::segment segment;
		memory::region region;
	};
} // namespace core::gfx