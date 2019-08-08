#pragma once
#include "fwd/gfx/geometry.h"

#ifdef PE_VULKAN
#include "vk/geometry.h"
#endif
#ifdef PE_GLES
#include "gles/geometry.h"
#endif