#pragma once
#include <variant>
#include "resource/resource.hpp"

#ifdef PE_GLES
namespace core::igles
{
	class shader;
}
#endif

#ifdef PE_VULKAN
namespace core::ivk
{
	class shader;
}
#endif

namespace core::meta
{
	class shader;
}
namespace core::gfx
{
	class context;

	class shader
	{
		using value_type = std::variant<
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

		friend class core::resource::cache;
		
		template<typename T>
		shader(T handle) : m_Handle(handle){};
	  public:
		shader(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
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