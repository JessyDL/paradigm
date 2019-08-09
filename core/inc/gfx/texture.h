#pragma once
#include "resource/resource.hpp"


#ifdef PE_VULKAN
namespace core::ivk
{
	class texture;
}
#endif
#ifdef PE_GLES
namespace core::igles
{
	class texture;
}
#endif

namespace core::gfx
{
	class context;
	class texture
	{
		using value_type = std::variant<
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
	  public:
		texture(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
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
} // namespace core::gfx