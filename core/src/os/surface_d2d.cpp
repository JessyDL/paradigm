
#include "os/surface.h"

#if defined(SURFACE_D2D)
using namespace core::os;

bool surface::init_surface() { return true; }

void surface::deinit_surface() {}


void surface::focus(bool value) {}


void surface::update_surface() {}


void surface::resize_surface() {}

#endif