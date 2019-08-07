#pragma once
#include "gfx/stdafx.h"

namespace core::gfx
{
	enum class shader_stage : uint8_t
	{
		vertex				   = 1 << 0,
		tesselation_control	= 1 << 1,
		tesselation_evaluation = 1 << 2,
		geometry			   = 1 << 3,
		fragment			   = 1 << 4,
		compute				   = 1 << 5
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
	enum class vertex_input_rate : uint8_t
	{
		vertex   = 0,
		instance = 1
	};

	inline vk::VertexInputRate to_vk(vertex_input_rate value) noexcept
	{
		switch(value)
		{
		case vertex_input_rate::vertex: return vk::VertexInputRate::eVertex; break;
		case vertex_input_rate::instance: return vk::VertexInputRate::eInstance; break;
		}
		assert(false);
		return vk::VertexInputRate::eVertex;
	}

	inline vertex_input_rate to_vertex_input_rate(vk::VertexInputRate value) noexcept
	{
		switch(value)
		{
		case vk::VertexInputRate::eVertex: return vertex_input_rate::vertex; break;
		case vk::VertexInputRate::eInstance: return vertex_input_rate::instance; break;
		}
		assert(false);
		return vertex_input_rate::vertex;
	}

#ifdef PE_GLES
	inline GLint to_gles(vertex_input_rate value) noexcept
	{
		return static_cast<std::underlying_type_t<vertex_input_rate>>(value);
	}

	inline vertex_input_rate to_vertex_input_rate(GLint value) noexcept { return vertex_input_rate(value); }
#endif
	enum class memory_write_frequency
	{
		per_frame,
		sometimes,
		almost_never
	};
	struct memory_copy
	{
		size_t source_offset;	  // offset in the source location
		size_t destination_offset; // offset in the destination location
		size_t size;			   // size of the copy instruction
	};
	enum class memory_type
	{
		transfer_source		  = 1 << 0,
		transfer_destination  = 1 << 1,
		uniform_texel_buffer  = 1 << 2,
		storage_texel_buffer  = 1 << 3,
		uniform_buffer		  = 1 << 4,
		storage_buffer		  = 1 << 5,
		index_buffer		  = 1 << 6,
		vertex_buffer		  = 1 << 7,
		indirect_buffer		  = 1 << 8,
		conditional_rendering = 1 << 9
	};

	inline vk::BufferUsageFlags to_vk(memory_type memory) noexcept
	{
		using vk_type  = std::underlying_type_t<vk::BufferUsageFlagBits>;
		using gfx_type = std::underlying_type_t<memory_type>;

		vk_type destination{0};
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::transfer_source)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eTransferSrc)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::transfer_destination)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eTransferDst)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::uniform_texel_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eUniformTexelBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::storage_texel_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eStorageTexelBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::uniform_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eUniformBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::storage_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eStorageBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::index_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eIndexBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::vertex_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eVertexBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::indirect_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eIndirectBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::conditional_rendering)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eConditionalRenderingEXT)
						   : 0;
		return vk::BufferUsageFlags{vk::BufferUsageFlagBits{destination}};
	}

	inline memory_type to_memory_type(vk::BufferUsageFlags flags) noexcept
	{
		using gfx_type = std::underlying_type_t<memory_type>;

		gfx_type res{0};
		res += (flags & vk::BufferUsageFlagBits::eConditionalRenderingEXT)
				   ? static_cast<gfx_type>(memory_type::conditional_rendering)
				   : 0;
		res +=
			(flags & vk::BufferUsageFlagBits::eTransferSrc) ? static_cast<gfx_type>(memory_type::transfer_source) : 0;
		res += (flags & vk::BufferUsageFlagBits::eTransferDst)
				   ? static_cast<gfx_type>(memory_type::transfer_destination)
				   : 0;

		res += (flags & vk::BufferUsageFlagBits::eUniformTexelBuffer)
				   ? static_cast<gfx_type>(memory_type::uniform_texel_buffer)
				   : 0;

		res += (flags & vk::BufferUsageFlagBits::eStorageTexelBuffer)
				   ? static_cast<gfx_type>(memory_type::storage_texel_buffer)
				   : 0;

		res +=
			(flags & vk::BufferUsageFlagBits::eUniformBuffer) ? static_cast<gfx_type>(memory_type::uniform_buffer) : 0;

		res +=
			(flags & vk::BufferUsageFlagBits::eStorageBuffer) ? static_cast<gfx_type>(memory_type::storage_buffer) : 0;

		res += (flags & vk::BufferUsageFlagBits::eIndexBuffer) ? static_cast<gfx_type>(memory_type::index_buffer) : 0;

		res += (flags & vk::BufferUsageFlagBits::eVertexBuffer) ? static_cast<gfx_type>(memory_type::vertex_buffer) : 0;

		res += (flags & vk::BufferUsageFlagBits::eIndirectBuffer) ? static_cast<gfx_type>(memory_type::indirect_buffer)
																  : 0;

		return memory_type{res};
	}

#ifdef PE_GLES
	/// \warning gles does not have a concept of transfer_source/transfer_destination, all buffers are "valid" as either
	inline decltype(GL_ARRAY_BUFFER) to_gles(memory_type memory) noexcept
	{
		using gfx_type = std::underlying_type_t<memory_type>;

		if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::uniform_texel_buffer))
		{
			return GL_TEXTURE_BUFFER;
		}
		else if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::conditional_rendering))
		{
			assert(false);
			// not implemented
		}
		else if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::index_buffer))
		{
			return GL_ELEMENT_ARRAY_BUFFER;
		}
		else if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::vertex_buffer))
		{
			return GL_ARRAY_BUFFER;
		}
		else if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_type::storage_buffer))
		{
			return GL_SHADER_STORAGE_BUFFER;
		}

		assert(false);
		return -1;
	}
#endif

	enum class memory_property
	{
		device_local,
		host_visible,
		host_cached,
		lazily_allocated,
		protected_memory
	};


} // namespace core::gfx