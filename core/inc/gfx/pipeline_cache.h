#pragma once
#include <variant>
#include "resource/resource.hpp"

#ifdef PE_VULKAN
namespace core::ivk
{
	class pipeline_cache;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class program_cache;
} // namespace core::igles
#endif
namespace core::gfx
{
	class context;

	class pipeline_cache
	{
		using value_type = std::variant<
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
	  public:
		pipeline_cache(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
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