
#include "core/os/surface.hpp"
#include "core/systems/input.hpp"

#if defined(PE_VULKAN)
	#include "core/vk/swapchain.hpp"
#endif

using namespace psl;
using namespace core::os;
using namespace core;

surface::surface(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 core::resource::handle<data::window> data)
	: m_Data(data), m_InputSystem(new core::systems::input()), m_Open(false) {
	m_Open = init_surface();
}

surface::~surface() {
	deinit_surface();
	delete(m_InputSystem);
}
bool surface::focused() const {
	return m_Focused;
}
bool surface::open() const {
	return m_Open;
}
void surface::terminate() {
	m_Open = false;
#if defined(PE_VULKAN)
	m_Swapchains.clear();
#endif
}
bool surface::tick() {
	if(m_Open)
		update_surface();
	return m_Open;
}

bool surface::resize(uint32_t width, uint32_t height) {
	if(m_Data->width() == width && m_Data->height() == height)
		return true;
	m_Data->width(width);
	m_Data->height(height);
	resize_surface();

#if defined(PE_VULKAN)
	for(auto& swapchain : m_Swapchains) {
		swapchain->resize();
	}
#endif
	return true;
}

#if defined(PE_VULKAN)
void surface::register_swapchain(core::resource::handle<core::ivk::swapchain> swapchain) {
	m_Swapchains.push_back(swapchain);
}
#endif
const data::window& surface::data() const {
	return m_Data.value();
}

core::systems::input& surface::input() const noexcept {
	return *m_InputSystem;
}
