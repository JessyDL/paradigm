#include "psl/platform_def.hpp"
#if defined(PE_PLATFORM_ANDROID)
	#include "core/os/surface.hpp"

using namespace core::os;

bool surface::init_surface() {
	return true;
}

void surface::deinit_surface() {}


void surface::focus(bool value) {}


void surface::update_surface() {}


void surface::resize_surface() {}

#endif
