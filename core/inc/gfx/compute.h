#pragma once
#include "resource/resource.hpp"
#include "fwd/gfx/compute.h"
#include "psl/math/vec.h"

namespace core::data
{
	class material;
}
namespace core::gfx
{
	class context;
	class pipeline_cache;

	class compute
	{
		friend class core::resource::cache;

		template <typename T>
		compute(T handle) : m_Handle(handle){};

	  public:
		using alias_type = core::resource::alias<
#ifdef PE_VULKAN
			core::ivk::compute
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::compute
#endif
			>;
		using value_type = alias_type;
		compute(core::resource::handle<value_type>& handle);
		compute(core::resource::cache& cache, const core::resource::metadata& metaData, core::meta::shader* metaFile,
				core::resource::handle<context> context_handle, core::resource::handle<core::data::material> data,
				core::resource::handle<pipeline_cache> pipeline_cache);
		~compute() = default;

		compute(const compute& other)	 = default;
		compute(compute&& other) noexcept = default;
		compute& operator=(const compute& other) = default;
		compute& operator=(compute&& other) noexcept = default;


		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

		core::meta::shader* meta() const noexcept;

		void dispatch(const psl::static_array<uint32_t, 3>& size);
	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx