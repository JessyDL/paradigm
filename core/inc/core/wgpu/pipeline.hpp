#pragma once
#include "core/fwd/resource/resource.hpp"
#include "core/wgpu/iwgpu.hpp"

namespace core::data {
class material_t;
}	 // namespace core::data

namespace core::iwgpu {
class context;
class pipeline {
  public:
	pipeline(core::resource::cache_t& cache,
			 const core::resource::metadata& metaData,
			 psl::meta::file* metaFile,
			 core::resource::handle<core::iwgpu::context> context,
			 core::resource::handle<core::data::material_t> data);
	~pipeline();

  private:
	core::resource::cache_t& m_Cache;
	core::resource::handle<core::iwgpu::context> m_Context;
	psl::meta::file* m_Meta;
	const psl::UID m_UID;
};
}	 // namespace core::iwgpu
