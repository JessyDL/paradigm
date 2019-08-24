#pragma once
#include "fwd/gfx/buffer.h"
#include "resource/resource.hpp"
#include "psl/array.h"
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
		  using alias_type = core::resource::alias<
#ifdef PE_VULKAN
			core::ivk::buffer
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::buffer
#endif
			>;
		using value_type = alias_type;
		buffer(core::resource::handle<value_type>& handle);
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

		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

		const core::data::buffer& data() const noexcept;

		[[nodiscard]] std::optional<memory::segment> reserve(uint64_t size);
		[[nodiscard]] psl::array<std::pair<memory::segment, memory::range>> reserve(psl::array<uint64_t> sizes,
																					bool optimize = false);
		bool deallocate(memory::segment& segment);

		bool copy_from(const buffer& other, psl::array<core::gfx::memory_copy> ranges);
		bool commit(const psl::array<core::gfx::commit_instruction>& instructions);

		size_t free_size() const noexcept;
	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx