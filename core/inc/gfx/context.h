#pragma once
#include "fwd/gfx/context.h"

#ifdef PE_VULKAN
#include "vk/context.h"
#endif
#ifdef PE_GLES
#include "gles/context.h"
#endif