#pragma once
#include <string_view>
#include <vector>

#include "core/gfx/types.hpp"

class CLIParser {
  public:
	CLIParser(int argc, char** argv) {
		for(auto i = 0; i < argc; ++i) {
			std::string_view text {argv[i]};
			if(text == "--vulkan") {
#if defined(PE_VULKAN)
				m_Backends.push_back(core::gfx::graphics_backend::vulkan);
#else
				throw std::runtime_error("Requested a Vulkan backend, but application does not support Vulkan");
#endif
			} else if(text == "--gles") {
#if defined(PE_GLES)
				m_Backends.push_back(core::gfx::graphics_backend::gles);
#else
				throw std::runtime_error("Requested a GLES backend, but application does not support GLES");
#endif
			} else if(text == "--webgpu") {
#if defined(PE_WEBGPU)
				m_Backends.push_back(core::gfx::graphics_backend::webgpu);
#else
				throw std::runtime_error("Requested a WebGPU backend, but application does not support WebGPU");
#endif
			}
		}

		if(m_Backends.empty()) {
#if defined(PE_VULKAN)
			m_Backends.push_back(core::gfx::graphics_backend::vulkan);
#elif defined(PE_WEBGPU)
			m_Backends.push_back(core::gfx::graphics_backend::webgpu);
#elif defined(PE_GLES)
			m_Backends.push_back(core::gfx::graphics_backend::gles);
#else
			throw std::runtime_error("No graphics backend specified, and no default backend is available");
#endif
		}
	}

	const std::vector<core::gfx::graphics_backend>& get_backends() const {
		return m_Backends;
	}

  private:
	std::vector<core::gfx::graphics_backend> m_Backends {};
};
