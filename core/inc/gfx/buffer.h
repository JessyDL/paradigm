#pragma once
#include "fwd/gfx/buffer.h"

#ifdef PE_VULKAN
#include "vk/buffer.h"
#endif
#ifdef PE_GLES
#include "gles/buffer.h"
#endif