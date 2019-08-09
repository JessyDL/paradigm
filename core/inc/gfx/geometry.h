#pragma once
#include "fwd/gfx/geometry.h"
#include "resource/resource.hpp"


#ifdef PE_VULKAN
#include "vk/geometry.h"
#endif
#ifdef PE_GLES
#include "gles/geometry.h"
#endif

namespace core::gfx
{
	class context;
	class buffer;

	class geometry
	{
		using value_type = std::variant<
#ifdef PE_VULKAN
			core::ivk::geometry
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::geometry
#endif
			>;
	  public:
		geometry(const psl::UID& uid, core::resource::cache& cache, core::resource::handle<context> context,
				 core::resource::handle<core::data::geometry> data,
				 core::resource::handle<buffer> geometryBuffer,
				 core::resource::handle<buffer> indicesBuffer);
		~geometry();
		geometry(const geometry&) = delete;
		geometry(geometry&&)	  = delete;
		geometry& operator=(const geometry&) = delete;
		geometry& operator=(geometry&&) = delete;

		core::resource::handle<value_type> resource() noexcept { return m_Handle; };
	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx