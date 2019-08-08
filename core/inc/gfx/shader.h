#pragma once
#include "fwd/gfx/shader.h"

#ifdef PE_VULKAN
#include "vk/shader.h"
#endif
#ifdef PE_GLES
#include "gles/shader.h"
#endif