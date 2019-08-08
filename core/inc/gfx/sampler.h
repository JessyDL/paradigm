#pragma once
#include "fwd/gfx/sampler.h"

#ifdef PE_VULKAN
#include "vk/sampler.h"
#endif
#ifdef PE_GLES
#include "gles/sampler.h"
#endif