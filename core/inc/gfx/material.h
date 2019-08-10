#pragma once
#include "resource/resource.hpp"
#include <variant>

#ifdef PE_VULKAN
namespace core::ivk
{
	class material;
}
#endif
#ifdef PE_GLES
namespace core::igles
{
	class material;
}
#endif

namespace core::data
{
	class material;
}

namespace core::gfx
{
	class context;
	class pipeline_cache;
	class buffer;

	class material
	{
		using value_type = std::variant<
#ifdef PE_VULKAN
			core::ivk::material
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::material
#endif
			>;
	  public:
		material(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile, core::resource::handle<context> context_handle,
				 core::resource::handle<core::data::material> data,
				 core::resource::handle<pipeline_cache> pipeline_cache, core::resource::handle<buffer> materialBuffer);

		~material() = default;

		material(const material& other)		= delete;
		material(material&& other) noexcept = delete;
		material& operator=(const material& other) = delete;
		material& operator=(material&& other) noexcept = delete;


		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

		const core::data::material& data() const noexcept;
	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx