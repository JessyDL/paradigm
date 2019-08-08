#pragma once
#include "fwd/gfx/texture.h"

#ifdef PE_VULKAN
#include "vk/texture.h"
#endif
#ifdef PE_GLES
#include "gles/texture.h"
#endif