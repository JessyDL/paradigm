#pragma once
#include "context.h"
#include "stdafx.h"
#include "vk/context.h"

namespace core::gfx::limits
{
	inline uint64_t storage_buffer_offset_alignment(const core::gfx::context& context)
	{
		switch(context.backend())
		{
		case graphics_backend::gles:
		{
			int gl_align = 4;
			glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &gl_align);
			return static_cast<uint64_t>(gl_align);
		}
		break;
		case graphics_backend::vulkan:
			return static_cast<uint64_t>(
				context.resource().get<core::ivk::context>()->properties().limits.minStorageBufferOffsetAlignment);
			break;
		}
		return 0;
	}

	inline uint64_t uniform_buffer_offset_alignment(const core::gfx::context& context)
	{
		switch(context.backend())
		{
		case graphics_backend::gles:
		{

			int gl_align = 4;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &gl_align);
			return static_cast<uint64_t>(gl_align);
		}
		break;
		case graphics_backend::vulkan:
			return static_cast<uint64_t>(
				context.resource().get<core::ivk::context>()->properties().limits.minUniformBufferOffsetAlignment);
			break;
		}
		return 0;
	}
} // namespace core::gfx::limits