#pragma once
#include "gfx/stdafx.h"

namespace core::gfx
{
	enum class shader_stage : uint8_t
	{
		vertex				   = 1 << 1,
		tesselation_control	= 1 << 2,
		tesselation_evaluation = 1 << 3,
		geometry			   = 1 << 4,
		fragment			   = 1 << 5,
		compute				   = 1 << 6
	};
	
	inline vk::ShaderStageFlags to_vk(shader_stage stage) noexcept
	{
		using vk_type  = std::underlying_type_t<vk::ShaderStageFlagBits>;
		using gfx_type = std::underlying_type_t<shader_stage>;

		vk_type destination{0};
		destination += static_cast<gfx_type>(stage) & static_cast<gfx_type>(shader_stage::vertex)
						   ? static_cast<vk_type>(vk::ShaderStageFlagBits::eVertex)
						   : 0;
		destination += static_cast<gfx_type>(stage) & static_cast<gfx_type>(shader_stage::fragment)
						   ? static_cast<vk_type>(vk::ShaderStageFlagBits::eFragment)
						   : 0;
		destination += static_cast<gfx_type>(stage) & static_cast<gfx_type>(shader_stage::geometry)
						   ? static_cast<vk_type>(vk::ShaderStageFlagBits::eGeometry)
						   : 0;
		destination += static_cast<gfx_type>(stage) & static_cast<gfx_type>(shader_stage::tesselation_control)
						   ? static_cast<vk_type>(vk::ShaderStageFlagBits::eTessellationControl)
						   : 0;
		destination += static_cast<gfx_type>(stage) & static_cast<gfx_type>(shader_stage::compute)
						   ? static_cast<vk_type>(vk::ShaderStageFlagBits::eCompute)
						   : 0;
		destination += static_cast<gfx_type>(stage) & static_cast<gfx_type>(shader_stage::tesselation_evaluation)
						   ? static_cast<vk_type>(vk::ShaderStageFlagBits::eTessellationEvaluation)
						   : 0;
		return vk::ShaderStageFlags{vk::ShaderStageFlagBits{destination}};
	}

	inline shader_stage to_shader_stage(vk::ShaderStageFlags flags)
	{
		using gfx_type = std::underlying_type_t<shader_stage>;

		gfx_type res{0};
		res += (flags & vk::ShaderStageFlagBits::eVertex) ? static_cast<gfx_type>(shader_stage::vertex) : 0;
		res += (flags & vk::ShaderStageFlagBits::eFragment) ? static_cast<gfx_type>(shader_stage::fragment) : 0;
		res += (flags & vk::ShaderStageFlagBits::eCompute) ? static_cast<gfx_type>(shader_stage::compute) : 0;
		res += (flags & vk::ShaderStageFlagBits::eGeometry) ? static_cast<gfx_type>(shader_stage::geometry) : 0;
		res += (flags & vk::ShaderStageFlagBits::eTessellationControl)
				   ? static_cast<gfx_type>(shader_stage::tesselation_control)
				   : 0;
		res += (flags & vk::ShaderStageFlagBits::eTessellationEvaluation)
				   ? static_cast<gfx_type>(shader_stage::tesselation_evaluation)
				   : 0;

		return shader_stage{res};
	}

#ifdef PE_GLES
	inline decltype(GL_VERTEX_SHADER) to_gles(shader_stage stage) noexcept
	{
		switch(stage)
		{
		case shader_stage::vertex: return GL_VERTEX_SHADER; break;
		case shader_stage::fragment: return GL_FRAGMENT_SHADER; break;
		case shader_stage::compute: return GL_COMPUTE_SHADER; break;
		case shader_stage::geometry: return GL_GEOMETRY_SHADER; break;
		case shader_stage::tesselation_control: return GL_TESS_CONTROL_SHADER; break;
		case shader_stage::tesselation_evaluation: return GL_TESS_EVALUATION_SHADER; break;
		}
		assert(false);
		return -1;
	}

	inline shader_stage to_shader_stage(GLint value) noexcept
	{
		switch(value)
		{
		case GL_VERTEX_SHADER: return shader_stage::vertex; break;
		case GL_FRAGMENT_SHADER: return shader_stage::fragment; break;
		case GL_COMPUTE_SHADER: return shader_stage::compute; break;
		case GL_GEOMETRY_SHADER: return shader_stage::geometry; break;
		case GL_TESS_CONTROL_SHADER: return shader_stage::tesselation_control; break;
		case GL_TESS_EVALUATION_SHADER: return shader_stage::tesselation_evaluation; break;
		}
		assert(false);
		return shader_stage{};
	}

#endif
} // namespace core::gfx