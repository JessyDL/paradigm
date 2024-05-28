#pragma once
#include "core/resource/handle.hpp"
#include "core/resource/resource.hpp"
#include "core/wgpu/iwgpu.hpp"


namespace core::data {
class material_t;
}

namespace core::iwgpu {
class context;
class pipeline;
class pipeline_cache {
  public:
	pipeline_cache(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   core::resource::handle<core::iwgpu::context> context);

	~pipeline_cache();

	core::resource::handle<pipeline> get(core::resource::handle<core::data::material_t> data);

  private:
	core::resource::handle<core::iwgpu::context> m_Context;
};
}	 // namespace core::iwgpu