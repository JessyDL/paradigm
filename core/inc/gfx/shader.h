#pragma once
#include "resource/resource.hpp"
#include "fwd/gfx/shader.h"

namespace core::gfx
{
	class context;

	class shader
	{
		friend class core::resource::cache;
		
		template<typename T>
		shader(T handle) : m_Handle(handle){};

	  public:
		  using alias_type = core::resource::alias<
#ifdef PE_VULKAN
			core::ivk::shader
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::shader
#endif
			>;
		using value_type = alias_type;
		shader(core::resource::handle<value_type>& handle);
		shader(core::resource::cache& cache, const core::resource::metadata& metaData, core::meta::shader* metaFile,
			   core::resource::handle<core::gfx::context> context);
		~shader() = default;

		shader(const shader& other)		= default;
		shader(shader&& other) noexcept = default;
		shader& operator=(const shader& other) = default;
		shader& operator=(shader&& other) noexcept = default;


		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

		core::meta::shader* meta() const noexcept;

	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx