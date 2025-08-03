#pragma once
#include "core/gfx/types.hpp"
#include "core/resource/resource.hpp"
#include "psl/view_ptr.hpp"
#include <functional>
#include <memory>

namespace core::os {
class context;
class surface;
}	 // namespace core::os

namespace core::gfx {
class render_graph;
class context;
class buffer_t;
struct shader_buffer_binding;
class drawpass;
}	 // namespace core::gfx

namespace memory {
class region;
}

/// \brief This runs a full instance
class engine_instance_t {
  public:
	struct options_t {
		core::gfx::graphics_backend backend {core::gfx::graphics_backend::undefined};
		struct {
			size_t size {20_mb};
			size_t alignment {4};
		} cpu_backed_memory_region {};

		struct {
			size_t size {32_mb};
			size_t alignment {4};
		} staging_buffer {};

		struct {
			size_t size {32_mb};
			size_t alignment {4};
		} vertex_buffer {};

		struct {
			size_t size {32_mb};
			size_t alignment {4};
		} index_buffer {};

		struct {
			size_t size {32_mb};
			size_t alignment {4};
		} instance_buffer {};

		struct {
			size_t size {8_mb};
		} instance_material_buffer {};

		struct {
			size_t size {8_mb};
		} instance_material_binding {};

		struct {
			size_t size {1_mb};
		} global_shader_buffer {};

		struct {
			size_t size {1_mb};
		} frame_cam_buffer_binding {};

		psl::string8_t application_name {"{ENGINE_NAME}"};
		psl::string8_t window_title {"{ENGINE_FULL_NAME} ({GRAPHICS_BACKEND}): {APPLICATION_NAME}"};
	};
	engine_instance_t(options_t options, std::unique_ptr<core::os::context> os_context);
	~engine_instance_t();
	/// \brief This method runs until the surface or os context is closed.
	/// Every frame, it will call the callback function.
	void run(std::function<void(engine_instance_t const&, std::chrono::duration<float>, std::chrono::duration<float>)> callback);

	core::resource::handle<core::gfx::context> const& context() const noexcept {
		return m_ContextHandle;
	}
	core::resource::handle<core::os::surface> const& surface() const noexcept {
		return m_SurfaceHandle;
	}

	core::resource::handle<core::gfx::buffer_t> const& staging_buffer() const noexcept {
		return m_StagingBufferHandle;
	}

	core::resource::handle<core::gfx::buffer_t> const& vertex_buffer() const noexcept {
		return m_VertexBufferHandle;
	}

	core::resource::handle<core::gfx::buffer_t> const& index_buffer() const noexcept {
		return m_IndexBufferHandle;
	}

	core::resource::handle<core::gfx::buffer_t> const& instance_buffer() const noexcept {
		return m_InstanceBufferHandle;
	}

	core::resource::handle<core::gfx::buffer_t> const& instance_material_buffer() const noexcept {
		return m_InstanceMaterialBufferHandle;
	}

	core::resource::handle<core::gfx::buffer_t> const& global_shader_buffer() const noexcept {
		return m_GlobalShaderBufferHandle;
	}

	core::resource::handle<core::gfx::shader_buffer_binding> const& instance_material_binding() const noexcept {
		return m_InstanceMaterialBindingHandle;
	}
	core::resource::handle<core::gfx::shader_buffer_binding> const& frame_cam_buffer_binding() const noexcept {
		return m_FrameCamBufferBindingHandle;
	}

	psl::view_ptr<core::gfx::drawpass> swapchain() const noexcept {
		return m_Swapchain;
	}

	core::resource::cache_t& cache();

  private:
	core::gfx::graphics_backend m_Backend {core::gfx::graphics_backend::undefined};
	std::unique_ptr<core::os::context> m_OSContext;
	std::unique_ptr<memory::region> m_MemoryRegion;
	std::unique_ptr<core::resource::cache_t> m_Cache;
	std::unique_ptr<core::gfx::render_graph> m_RenderGraph;
	core::resource::handle<core::os::surface> m_SurfaceHandle;
	core::resource::handle<core::gfx::context> m_ContextHandle;
	core::resource::handle<core::gfx::buffer_t> m_StagingBufferHandle;
	core::resource::handle<core::gfx::buffer_t> m_VertexBufferHandle;
	core::resource::handle<core::gfx::buffer_t> m_IndexBufferHandle;
	core::resource::handle<core::gfx::buffer_t> m_InstanceBufferHandle;
	core::resource::handle<core::gfx::buffer_t> m_InstanceMaterialBufferHandle;
	core::resource::handle<core::gfx::buffer_t> m_GlobalShaderBufferHandle;
	core::resource::handle<core::gfx::shader_buffer_binding> m_InstanceMaterialBindingHandle;
	core::resource::handle<core::gfx::shader_buffer_binding> m_FrameCamBufferBindingHandle;
	psl::view_ptr<core::gfx::drawpass> m_Swapchain;
};
