#pragma once
#include <variant>
#include "resource/resource.hpp"
#include "fwd/gfx/sampler.h"

namespace core::data
{
	class sampler;
}

namespace core::gfx
{
	class context;

	class sampler
	{
	  public:
		  using alias_type = core::resource::alias<
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
		using value_type = alias_type;
		sampler(core::resource::handle<value_type>& handle);
		sampler(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
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