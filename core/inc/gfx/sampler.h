#pragma once
#include <variant>
#include "resource/resource.hpp"

#ifdef PE_GLES
namespace core::igles
{
	class sampler;
}
#endif

namespace core::ivk
{
	class sampler;
}

namespace core::data
{
	class sampler;
}


namespace core::gfx
{
	class context;

	class sampler
	{
		using value_type = std::variant<
#ifdef PE_VULKAN
			core::ivk::sampler
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::sampler
#endif

			>;
	  public:
		sampler(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
				core::resource::handle<core::gfx::context> context,
				core::resource::handle<core::data::sampler> sampler_data);
		~sampler() = default;

		sampler(const sampler& other)	 = delete;
		sampler(sampler&& other) noexcept = delete;
		sampler& operator=(const sampler& other) = delete;
		sampler& operator=(sampler&& other) noexcept = delete;

		core::resource::handle<value_type> resource() noexcept { return m_Handle; };

	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx