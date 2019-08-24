#pragma once

namespace core::gfx
{
	enum class graphics_backend
	{
		vulkan = 1 << 0,
		gles   = 1 << 1
	};
}