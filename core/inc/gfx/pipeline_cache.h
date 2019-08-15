#pragma once
#include <variant>
#include "resource/resource.hpp"
#include "fwd/gfx/pipeline_cache.h"

namespace core::gfx
{
	class context;

	class pipeline_cache
	{
	  public:
		using alias_type = core::resource::alias<
#ifdef PE_VULKAN
			core::ivk::pipeline_cache
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::program_cache
#endif
			>;
		using value_type = alias_type;
		pipeline_cache(core::resource::handle<value_type>& handle);
		pipeline_cache(core::resource::cache& cache, const core::resource::metadata& metaData,
					   psl::meta::file* metaFile,
					   core::resource::handle<core::gfx::context> context);
		~pipeline_cache() = default;

		pipeline_cache(const pipeline_cache& other)		= default;
		pipeline_cache(pipeline_cache&& other) noexcept = default;
		pipeline_cache& operator=(const pipeline_cache& other) = default;
		pipeline_cache& operator=(pipeline_cache&& other) noexcept = default;

		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx