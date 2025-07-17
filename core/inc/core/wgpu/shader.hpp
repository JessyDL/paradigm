#pragma once

#include "core/fwd/wgpu/shader.hpp"
#include "core/resource/resource.hpp"
#include "core/wgpu/iwgpu.hpp"

namespace core::iwgpu {
class context;

class shader {
  public:
	shader(core::resource::cache_t& cache,
		   const core::resource::metadata& metaData,
		   core::meta::shader* metaFile,
		   core::resource::handle<core::iwgpu::context> context);
	~shader();
	shader(const shader&)			 = delete;
	shader(shader&&)				 = delete;
	shader& operator=(const shader&) = delete;
	shader& operator=(shader&&)		 = delete;

	core::meta::shader* meta() const noexcept;

	wgpu::ShaderModule module() const noexcept {
		return m_Shader;
	}

  private:
	core::resource::handle<core::iwgpu::context> m_Context;
	core::resource::cache_t& m_Cache;
	core::meta::shader* m_Meta;
	wgpu::ShaderModule m_Shader;
	const psl::UID m_UID;
};
}	 // namespace core::iwgpu
