#pragma once
#include "fwd/gfx/texture.h"
#include "resource/resource.hpp"

namespace core::gfx
{
	class context;
	class texture
	{
	  public:
		using alias_type = core::resource::alias<
#ifdef PE_VULKAN
			core::ivk::texture
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::texture
#endif
			>;
		using value_type = alias_type;
		using meta_type = core::meta::texture;
		texture(core::resource::handle<value_type>& handle);
		texture(core::resource::cache& cache, const core::resource::metadata& metaData,
				core::meta::texture* metaFile,
				core::resource::handle<core::gfx::context> context);

		~texture();

		texture(const texture& other)	 = delete;
		texture(texture&& other) noexcept = delete;
		texture& operator=(const texture& other) = delete;
		texture& operator=(texture&& other) noexcept = delete;

		core::resource::handle<value_type> resource() noexcept { return m_Handle; };

	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gf