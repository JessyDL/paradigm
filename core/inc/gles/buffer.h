#pragma once
#include "resource/resource.hpp"
#include "memory/segment.h"
#include <optional>
#include "gfx/types.h"

namespace core::data
{
	class buffer;
}

namespace core::igles
{
	class buffer
	{
	  public:
		buffer(const psl::UID& uid, core::resource::cache& cache,
			   core::resource::handle<core::data::buffer> buffer_data);

		~buffer();
		buffer(const buffer&) = delete;
		buffer(buffer&&)	  = delete;
		buffer& operator=(const buffer&) = delete;
		buffer& operator=(buffer&&) = delete;

		bool set(const void* data, std::vector<core::gfx::memory_copy> commands);

		inline GLuint id() const noexcept { return m_Buffer; };

	  private:
		GLuint m_Buffer;
		GLint m_BufferType;
		core::resource::handle<core::data::buffer> m_BufferDataHandle;
	};
} // namespace core::igles