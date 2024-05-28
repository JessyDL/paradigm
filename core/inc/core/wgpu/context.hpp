#pragma once
#include "core/fwd/resource/resource.hpp"
#include "core/gfx/limits.hpp"
#include "core/wgpu/iwgpu.hpp"

namespace core::resource {
class cache_t;
}

namespace core::os {
class surface;
}

namespace core::iwgpu {
class context {
  public:
	context(core::resource::cache_t& cache,
			const core::resource::metadata& metaData,
			psl::meta::file* metaFile,
			psl::string8::view name,
			core::resource::handle<core::os::surface> surface);
	~context() = default;

	context(const context& other)			 = delete;
	context(context&& other)				 = delete;
	context& operator=(const context& other) = delete;
	context& operator=(context&& other)		 = delete;

	wgpu::Instance& instance() noexcept { return m_Instance; }
	wgpu::Adapter& adapter() noexcept { return m_Adapter; }
	wgpu::Device& device() noexcept { return m_Device; }
	wgpu::Queue& queue() noexcept { return m_Queue; }
	wgpu::Surface& surface() noexcept { return m_Surface; }

	auto limits() const noexcept -> const core::gfx::limits& { return m_Limits; }

  private:
	wgpu::Instance m_Instance  = {};
	wgpu::Adapter m_Adapter	   = {};
	wgpu::Device m_Device	   = {};
	wgpu::Queue m_Queue		   = {};
	wgpu::Surface m_Surface	   = {};
	core::gfx::limits m_Limits = {};
};
}	 // namespace core::iwgpu
