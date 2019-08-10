#pragma once
#include "fwd/gfx/buffer.h"
#include "resource/resource.hpp"
#include "array.h"
#include "memory/segment.h"
#include "memory/range.h"
#include <optional>

#ifdef PE_VULKAN
#include "vk/buffer.h"
#endif
#ifdef PE_GLES
#include "gles/buffer.h"
#endif

#include <variant>

namespace core::data
{
	class buffer;
}

namespace core::gfx
{
	class context;
	class buffer
	{
		using value_type = std::variant<
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
	  public:
		buffer(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* meta,
			   core::resource::handle<core::gfx::context> context, core::resource::handle<core::data::buffer> data);

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
	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx