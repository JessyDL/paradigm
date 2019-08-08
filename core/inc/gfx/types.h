#pragma once
#include "gfx/stdafx.h"

namespace core::gfx
{
	template <typename T>
	class enum_flag
	{
	  public:
		using value_type = std::underlying_type_t<T>;
		enum_flag(T value) : m_Enum(static_cast<value_type>(value)){};
		enum_flag(value_type value) : m_Enum(value){};

		~enum_flag()				= default;
		enum_flag(const enum_flag&) = default;
		enum_flag(enum_flag&&)		= default;
		enum_flag& operator=(const enum_flag&) = default;
		enum_flag& operator=(enum_flag&&) = default;

		enum_flag& operator|=(enum_flag const& rhs) noexcept
		{
			m_Enum |= rhs.m_Enum;
			return *this;
		}

		enum_flag& operator&=(enum_flag const& rhs) noexcept
		{
			m_Enum &= rhs.m_Enum;
			return *this;
		}

		enum_flag& operator^=(enum_flag const& rhs) noexcept
		{
			m_Enum ^= rhs.m_Enum;
			return *this;
		}

		enum_flag operator|(enum_flag const& rhs) const noexcept
		{
			enum_flag result(*this);
			result |= rhs;
			return result;
		}

		enum_flag operator&(enum_flag const& rhs) const noexcept
		{
			enum_flag result(*this);
			result &= rhs;
			return result;
		}

		enum_flag operator^(enum_flag const& rhs) const noexcept
		{
			enum_flag<T> result(*this);
			result ^= rhs;
			return result;
		}
		bool operator==(enum_flag const& rhs) const noexcept { return m_Enum == rhs.m_Enum; }
		bool operator!=(enum_flag const& rhs) const noexcept { return m_Enum != rhs.m_Enum; }

		enum_flag& operator|=(T const& rhs) noexcept { return *this |= static_cast<value_type>(rhs); }
		enum_flag& operator&=(T const& rhs) noexcept { return *this &= static_cast<value_type>(rhs); }
		enum_flag& operator^=(T const& rhs) noexcept { return *this ^= static_cast<value_type>(rhs); }
		enum_flag operator|(T const& rhs) const noexcept { return *this | static_cast<value_type>(rhs); }
		enum_flag operator&(T const& rhs) const noexcept { return *this & static_cast<value_type>(rhs); }
		enum_flag operator^(T const& rhs) const noexcept { return *this ^ static_cast<value_type>(rhs); }
		bool operator==(T const& rhs) const noexcept { return m_Enum == static_cast<value_type>(rhs); }
		bool operator!=(T const& rhs) const noexcept { return m_Enum != static_cast<value_type>(rhs); }

		operator T() const noexcept { return T{m_Enum}; }
		explicit operator value_type() const noexcept { return m_Enum; }

	  private:
		value_type m_Enum;
	};

	enum class shader_stage : uint8_t
	{
		vertex				   = 1 << 0,
		tesselation_control	= 1 << 1,
		tesselation_evaluation = 1 << 2,
		geometry			   = 1 << 3,
		fragment			   = 1 << 4,
		compute				   = 1 << 5
	};

#ifdef PE_VULKAN
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

#endif
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

#ifdef PE_VULKAN
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
#endif
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

#ifdef PE_VULKAN
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
#endif

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

	enum class image_type
	{
		planar_1D  = 0,
		planar_2D  = 1,
		planar_3D  = 2,
		cube_2D	= 3,
		array_1D   = 4,
		array_2D   = 5,
		array_cube = 6
	};

#ifdef PE_VULKAN
	inline vk::ImageViewType to_vk(image_type value) noexcept
	{
		switch(value)
		{
		case image_type::planar_1D: return vk::ImageViewType::e1D; break;
		case image_type::planar_2D: return vk::ImageViewType::e2D; break;
		case image_type::planar_3D: return vk::ImageViewType::e3D; break;
		case image_type::cube_2D: return vk::ImageViewType::eCube; break;
		case image_type::array_1D: return vk::ImageViewType::e1DArray; break;
		case image_type::array_2D: return vk::ImageViewType::e2DArray; break;
		case image_type::array_cube: return vk::ImageViewType::eCubeArray; break;
		}

		assert(false);
		return vk::ImageViewType::e1D;
	}

	inline image_type to_image_type(vk::ImageViewType value) noexcept
	{
		switch(value)
		{
		case vk::ImageViewType::e1D: return image_type::planar_1D; break;
		case vk::ImageViewType::e2D: return image_type::planar_2D; break;
		case vk::ImageViewType::e3D: return image_type::planar_3D; break;
		case vk::ImageViewType::eCube: return image_type::cube_2D; break;
		case vk::ImageViewType::e1DArray: return image_type::array_1D; break;
		case vk::ImageViewType::e2DArray: return image_type::array_2D; break;
		case vk::ImageViewType::eCubeArray: return image_type::array_cube; break;
		}

		assert(false);
		return image_type::planar_1D;
	}

	inline vk::ImageType to_type(image_type value) noexcept
	{

		switch(value)
		{
		case image_type::planar_1D: return vk::ImageType::e1D; break;
		case image_type::planar_2D: return vk::ImageType::e2D; break;
		case image_type::planar_3D: return vk::ImageType::e3D; break;
		case image_type::cube_2D: return vk::ImageType::e2D; break;
		case image_type::array_1D: return vk::ImageType::e1D; break;
		case image_type::array_2D: return vk::ImageType::e2D; break;
		case image_type::array_cube: return vk::ImageType::e2D; break;
		}

		assert(false);
		return vk::ImageType::e1D;
	}
#endif

	enum class image_aspect
	{
		color   = 1 << 0,
		depth   = 1 << 1,
		stencil = 1 << 2
	};


	inline image_aspect operator|(image_aspect bit0, image_aspect bit1)
	{
		using image_aspect_flags = enum_flag<image_aspect>;
		return (image_aspect_flags(bit0) | bit1);
	}
#ifdef PE_VULKAN
	inline vk::ImageAspectFlags to_vk(image_aspect value) noexcept
	{
		return vk::ImageAspectFlags{vk::ImageAspectFlagBits{static_cast<std::underlying_type_t<image_aspect>>(value)}};
	}

	inline image_aspect to_image_aspect(vk::ImageAspectFlags value) noexcept
	{
		return image_aspect(static_cast<VkImageAspectFlags>(value));
	}

#endif

	enum class image_usage
	{
		transfer_source			= 1 << 0,
		transfer_destination	= 1 << 1,
		sampled					= 1 << 2,
		storage					= 1 << 3,
		color_attachment		= 1 << 4,
		dept_stencil_attachment = 1 << 5,
		transient_attachment	= 1 << 6,
		input_attachment		= 1 << 7
	};


	inline image_usage operator|(image_usage bit0, image_usage bit1)
	{
		using image_usage_flags = enum_flag<image_usage>;
		return image_usage_flags(bit0) | bit1;
	}

#ifdef PE_VULKAN
	inline vk::ImageUsageFlags to_vk(image_usage value) noexcept
	{
		return vk::ImageUsageFlags{vk::ImageUsageFlagBits{static_cast<std::underlying_type_t<image_usage>>(value)}};
	}

	inline image_usage to_image_usage(vk::ImageUsageFlags value) noexcept
	{
		return image_usage(static_cast<VkImageUsageFlags>(value));
	}

#endif

	enum class component_bits
	{
		r = 1 << 0,
		g = 1 << 1,
		b = 1 << 2,
		a = 1 << 3,
		x = r,
		y = g,
		z = b,
		w = a
	};

	inline component_bits operator|(component_bits bit0, component_bits bit1)
	{
		using component_bit_flags = enum_flag<component_bits>;
		return component_bit_flags(bit0) | bit1;
	}

	namespace conversion
	{
#ifdef PE_VULKAN
		inline vk::ColorComponentFlags to_vk(component_bits value) noexcept
		{
			return vk::ColorComponentFlags{
				vk::ColorComponentFlagBits{static_cast<std::underlying_type_t<component_bits>>(value)}};
		}
		inline component_bits to_component_bits(vk::ColorComponentFlags value) noexcept
		{
			return component_bits(static_cast<VkColorComponentFlags>(value));
		}


#endif

#ifdef PE_GLES

#endif
	} // namespace conversion


	// based on https://github.com/KhronosGroup/KTX-Specification/blob/master/formats.json
	enum class format
	{
		undefined					= 0,
		r4g4_unorm_pack8			= 1000,
		r4g4b4a4_unorm_pack16		= 143,
		b4g4r4a4_unorm_pack16		= 1,
		r5g6b5_unorm_pack16			= 2,
		b5g6r5_unorm_pack16			= 3,
		r5g5b5a1_unorm_pack16		= 4,
		b5g5r5a1_unorm_pack16		= 5,
		a1r5g5b5_unorm_pack16		= 6,
		r8_unorm					= 7,
		r8_snorm					= 8,
		r8_uint						= 9,
		r8_sint						= 10,
		r8_srgb						= 11,
		r8g8_unorm					= 12,
		r8g8_snorm					= 13,
		r8g8_uint					= 14,
		r8g8_sint					= 15,
		r8g8_srgb					= 16,
		r8g8b8_unorm				= 17,
		r8g8b8_snorm				= 18,
		r8g8b8_uint					= 19,
		r8g8b8_sint					= 20,
		r8g8b8_srgb					= 21,
		b8g8r8_unorm				= 22,
		b8g8r8_snorm				= 23,
		b8g8r8_uint					= 24,
		b8g8r8_sint					= 25,
		b8g8r8_srgb					= 26,
		r8g8b8a8_unorm				= 27,
		r8g8b8a8_snorm				= 28,
		r8g8b8a8_uint				= 29,
		r8g8b8a8_sint				= 30,
		r8g8b8a8_srgb				= 31,
		b8g8r8a8_unorm				= 32,
		b8g8r8a8_snorm				= 33,
		b8g8r8a8_uint				= 34,
		b8g8r8a8_sint				= 35,
		b8g8r8a8_srgb				= 36,
		a2r10g10b10_unorm_pack32	= 37,
		a2r10g10b10_uint_pack32		= 38,
		a2b10g10r10_unorm_pack32	= 39,
		a2b10g10r10_uint_pack32		= 40,
		r16_unorm					= 41,
		r16_snorm					= 42,
		r16_uint					= 43,
		r16_sint					= 44,
		r16_sfloat					= 45,
		r16g16_unorm				= 46,
		r16g16_snorm				= 47,
		r16g16_uint					= 48,
		r16g16_sint					= 49,
		r16g16_sfloat				= 50,
		r16g16b16_unorm				= 51,
		r16g16b16_snorm				= 52,
		r16g16b16_uint				= 53,
		r16g16b16_sint				= 54,
		r16g16b16_sfloat			= 55,
		r16g16b16a16_unorm			= 56,
		r16g16b16a16_snorm			= 57,
		r16g16b16a16_uint			= 58,
		r16g16b16a16_sint			= 59,
		r16g16b16a16_sfloat			= 60,
		r32_uint					= 61,
		r32_sint					= 62,
		r32_sfloat					= 63,
		r32g32_uint					= 64,
		r32g32_sint					= 65,
		r32g32_sfloat				= 66,
		r32g32b32_uint				= 67,
		r32g32b32_sint				= 68,
		r32g32b32_sfloat			= 69,
		r32g32b32a32_uint			= 70,
		r32g32b32a32_sint			= 71,
		r32g32b32a32_sfloat			= 72,
		b10g11r11_ufloat_pack32		= 73,
		e5b9g9r9_ufloat_pack32		= 74,
		d16_unorm					= 75,
		x8_d24_unorm_pack32			= 76,
		d32_sfloat					= 77,
		s8_uint						= 78,
		d24_unorm_s8_uint			= 79,
		d32_sfloat_s8_uint			= 80,
		bc1_rgb_unorm_block			= 81,
		bc1_rgb_srgb_block			= 82,
		bc1_rgba_unorm_block		= 83,
		bc1_rgba_srgb_block			= 84,
		bc2_unorm_block				= 85,
		bc2_srgb_block				= 86,
		bc3_unorm_block				= 87,
		bc3_srgb_block				= 88,
		bc4_unorm_block				= 89,
		bc4_snorm_block				= 90,
		bc5_unorm_block				= 91,
		bc5_snorm_block				= 92,
		bc6h_ufloat_block			= 93,
		bc6h_sfloat_block			= 94,
		bc7_unorm_block				= 95,
		bc7_srgb_block				= 96,
		etc2_r8g8b8_unorm_block		= 97,
		etc2_r8g8b8_srgb_block		= 98,
		etc2_r8g8b8a1_unorm_block   = 99,
		etc2_r8g8b8a1_srgb_block	= 100,
		etc2_r8g8b8a8_unorm_block   = 101,
		etc2_r8g8b8a8_srgb_block	= 102,
		eac_r11_unorm_block			= 103,
		eac_r11_snorm_block			= 104,
		eac_r11g11_unorm_block		= 105,
		eac_r11g11_snorm_block		= 106,
		astc_4x4_unorm_block		= 107,
		astc_4x4_srgb_block			= 108,
		astc_5x4_unorm_block		= 109,
		astc_5x4_srgb_block			= 110,
		astc_5x5_unorm_block		= 111,
		astc_5x5_srgb_block			= 112,
		astc_6x5_unorm_block		= 113,
		astc_6x5_srgb_block			= 114,
		astc_6x6_unorm_block		= 115,
		astc_6x6_srgb_block			= 116,
		astc_8x5_unorm_block		= 117,
		astc_8x5_srgb_block			= 118,
		astc_8x6_unorm_block		= 119,
		astc_8x6_srgb_block			= 120,
		astc_8x8_unorm_block		= 121,
		astc_8x8_srgb_block			= 122,
		astc_10x5_unorm_block		= 123,
		astc_10x5_srgb_block		= 124,
		astc_10x6_unorm_block		= 125,
		astc_10x6_srgb_block		= 126,
		astc_10x8_unorm_block		= 127,
		astc_10x8_srgb_block		= 128,
		astc_10x10_unorm_block		= 129,
		astc_10x10_srgb_block		= 130,
		astc_12x10_unorm_block		= 131,
		astc_12x10_srgb_block		= 132,
		astc_12x12_unorm_block		= 133,
		astc_12x12_srgb_block		= 134,
		pvrtc1_2bpp_unorm_block_img = 135,
		pvrtc1_4bpp_unorm_block_img = 136,
		pvrtc2_2bpp_unorm_block_img = 137,
		pvrtc2_4bpp_unorm_block_img = 138,
		pvrtc1_2bpp_srgb_block_img  = 139,
		pvrtc1_4bpp_srgb_block_img  = 140,
		pvrtc2_2bpp_srgb_block_img  = 141,
		pvrtc2_4bpp_srgb_block_img  = 142
	};

	inline bool is_texture_format(format value) noexcept
	{
		return static_cast<std::underlying_type_t<format>>(value) < 1000;
	}

#ifdef PE_VULKAN
	inline format to_format(vk::Format value) noexcept
	{
		switch(static_cast<VkFormat>(value))
		{
		case VK_FORMAT_R4G4_UNORM_PACK8: return format::r4g4_unorm_pack8; break;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return format::r4g4b4a4_unorm_pack16; break;
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return format::b4g4r4a4_unorm_pack16; break;
		case VK_FORMAT_R5G6B5_UNORM_PACK16: return format::r5g6b5_unorm_pack16; break;
		case VK_FORMAT_B5G6R5_UNORM_PACK16: return format::b5g6r5_unorm_pack16; break;
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return format::r5g5b5a1_unorm_pack16; break;
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return format::b5g5r5a1_unorm_pack16; break;
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return format::a1r5g5b5_unorm_pack16; break;
		case VK_FORMAT_R8_UNORM: return format::r8_unorm; break;
		case VK_FORMAT_R8_SNORM: return format::r8_snorm; break;
		case VK_FORMAT_R8_UINT: return format::r8_uint; break;
		case VK_FORMAT_R8_SINT: return format::r8_sint; break;
		case VK_FORMAT_R8_SRGB: return format::r8_srgb; break;
		case VK_FORMAT_R8G8_UNORM: return format::r8g8_unorm; break;
		case VK_FORMAT_R8G8_SNORM: return format::r8g8_snorm; break;
		case VK_FORMAT_R8G8_UINT: return format::r8g8_uint; break;
		case VK_FORMAT_R8G8_SINT: return format::r8g8_sint; break;
		case VK_FORMAT_R8G8_SRGB: return format::r8g8_srgb; break;
		case VK_FORMAT_R8G8B8_UNORM: return format::r8g8b8_unorm; break;
		case VK_FORMAT_R8G8B8_SNORM: return format::r8g8b8_snorm; break;
		case VK_FORMAT_R8G8B8_UINT: return format::r8g8b8_uint; break;
		case VK_FORMAT_R8G8B8_SINT: return format::r8g8b8_sint; break;
		case VK_FORMAT_R8G8B8_SRGB: return format::r8g8b8_srgb; break;
		case VK_FORMAT_B8G8R8_UNORM: return format::b8g8r8_unorm; break;
		case VK_FORMAT_B8G8R8_SNORM: return format::b8g8r8_snorm; break;
		case VK_FORMAT_B8G8R8_UINT: return format::b8g8r8_uint; break;
		case VK_FORMAT_B8G8R8_SINT: return format::b8g8r8_sint; break;
		case VK_FORMAT_B8G8R8_SRGB: return format::b8g8r8_srgb; break;
		case VK_FORMAT_R8G8B8A8_UNORM: return format::r8g8b8a8_unorm; break;
		case VK_FORMAT_R8G8B8A8_SNORM: return format::r8g8b8a8_snorm; break;
		case VK_FORMAT_R8G8B8A8_UINT: return format::r8g8b8a8_uint; break;
		case VK_FORMAT_R8G8B8A8_SINT: return format::r8g8b8a8_sint; break;
		case VK_FORMAT_R8G8B8A8_SRGB: return format::r8g8b8a8_srgb; break;
		case VK_FORMAT_B8G8R8A8_UNORM: return format::b8g8r8a8_unorm; break;
		case VK_FORMAT_B8G8R8A8_SNORM: return format::b8g8r8a8_snorm; break;
		case VK_FORMAT_B8G8R8A8_UINT: return format::b8g8r8a8_uint; break;
		case VK_FORMAT_B8G8R8A8_SINT: return format::b8g8r8a8_sint; break;
		case VK_FORMAT_B8G8R8A8_SRGB: return format::b8g8r8a8_srgb; break;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return format::a2r10g10b10_unorm_pack32; break;
		case VK_FORMAT_A2R10G10B10_UINT_PACK32: return format::a2r10g10b10_uint_pack32; break;
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return format::a2b10g10r10_unorm_pack32; break;
		case VK_FORMAT_A2B10G10R10_UINT_PACK32: return format::a2b10g10r10_uint_pack32; break;
		case VK_FORMAT_R16_UNORM: return format::r16_unorm; break;
		case VK_FORMAT_R16_SNORM: return format::r16_snorm; break;
		case VK_FORMAT_R16_UINT: return format::r16_uint; break;
		case VK_FORMAT_R16_SINT: return format::r16_sint; break;
		case VK_FORMAT_R16_SFLOAT: return format::r16_sfloat; break;
		case VK_FORMAT_R16G16_UNORM: return format::r16g16_unorm; break;
		case VK_FORMAT_R16G16_SNORM: return format::r16g16_snorm; break;
		case VK_FORMAT_R16G16_UINT: return format::r16g16_uint; break;
		case VK_FORMAT_R16G16_SINT: return format::r16g16_sint; break;
		case VK_FORMAT_R16G16_SFLOAT: return format::r16g16_sfloat; break;
		case VK_FORMAT_R16G16B16_UNORM: return format::r16g16b16_unorm; break;
		case VK_FORMAT_R16G16B16_SNORM: return format::r16g16b16_snorm; break;
		case VK_FORMAT_R16G16B16_UINT: return format::r16g16b16_uint; break;
		case VK_FORMAT_R16G16B16_SINT: return format::r16g16b16_sint; break;
		case VK_FORMAT_R16G16B16_SFLOAT: return format::r16g16b16_sfloat; break;
		case VK_FORMAT_R16G16B16A16_UNORM: return format::r16g16b16a16_unorm; break;
		case VK_FORMAT_R16G16B16A16_SNORM: return format::r16g16b16a16_snorm; break;
		case VK_FORMAT_R16G16B16A16_UINT: return format::r16g16b16a16_uint; break;
		case VK_FORMAT_R16G16B16A16_SINT: return format::r16g16b16a16_sint; break;
		case VK_FORMAT_R16G16B16A16_SFLOAT: return format::r16g16b16a16_sfloat; break;
		case VK_FORMAT_R32_UINT: return format::r32_uint; break;
		case VK_FORMAT_R32_SINT: return format::r32_sint; break;
		case VK_FORMAT_R32_SFLOAT: return format::r32_sfloat; break;
		case VK_FORMAT_R32G32_UINT: return format::r32g32_uint; break;
		case VK_FORMAT_R32G32_SINT: return format::r32g32_sint; break;
		case VK_FORMAT_R32G32_SFLOAT: return format::r32g32_sfloat; break;
		case VK_FORMAT_R32G32B32_UINT: return format::r32g32b32_uint; break;
		case VK_FORMAT_R32G32B32_SINT: return format::r32g32b32_sint; break;
		case VK_FORMAT_R32G32B32_SFLOAT: return format::r32g32b32_sfloat; break;
		case VK_FORMAT_R32G32B32A32_UINT: return format::r32g32b32a32_uint; break;
		case VK_FORMAT_R32G32B32A32_SINT: return format::r32g32b32a32_sint; break;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return format::r32g32b32a32_sfloat; break;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return format::b10g11r11_ufloat_pack32; break;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return format::e5b9g9r9_ufloat_pack32; break;
		case VK_FORMAT_D16_UNORM: return format::d16_unorm; break;
		case VK_FORMAT_X8_D24_UNORM_PACK32: return format::x8_d24_unorm_pack32; break;
		case VK_FORMAT_D32_SFLOAT: return format::d32_sfloat; break;
		case VK_FORMAT_S8_UINT: return format::s8_uint; break;
		case VK_FORMAT_D24_UNORM_S8_UINT: return format::d24_unorm_s8_uint; break;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return format::d32_sfloat_s8_uint; break;
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return format::bc1_rgb_unorm_block; break;
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return format::bc1_rgb_srgb_block; break;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return format::bc1_rgba_unorm_block; break;
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return format::bc1_rgba_srgb_block; break;
		case VK_FORMAT_BC2_UNORM_BLOCK: return format::bc2_unorm_block; break;
		case VK_FORMAT_BC2_SRGB_BLOCK: return format::bc2_srgb_block; break;
		case VK_FORMAT_BC3_UNORM_BLOCK: return format::bc3_unorm_block; break;
		case VK_FORMAT_BC3_SRGB_BLOCK: return format::bc3_srgb_block; break;
		case VK_FORMAT_BC4_UNORM_BLOCK: return format::bc4_unorm_block; break;
		case VK_FORMAT_BC4_SNORM_BLOCK: return format::bc4_snorm_block; break;
		case VK_FORMAT_BC5_UNORM_BLOCK: return format::bc5_unorm_block; break;
		case VK_FORMAT_BC5_SNORM_BLOCK: return format::bc5_snorm_block; break;
		case VK_FORMAT_BC6H_UFLOAT_BLOCK: return format::bc6h_ufloat_block; break;
		case VK_FORMAT_BC6H_SFLOAT_BLOCK: return format::bc6h_sfloat_block; break;
		case VK_FORMAT_BC7_UNORM_BLOCK: return format::bc7_unorm_block; break;
		case VK_FORMAT_BC7_SRGB_BLOCK: return format::bc7_srgb_block; break;
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return format::etc2_r8g8b8_unorm_block; break;
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return format::etc2_r8g8b8_srgb_block; break;
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return format::etc2_r8g8b8a1_unorm_block; break;
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return format::etc2_r8g8b8a1_srgb_block; break;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return format::etc2_r8g8b8a8_unorm_block; break;
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return format::etc2_r8g8b8a8_srgb_block; break;
		case VK_FORMAT_EAC_R11_UNORM_BLOCK: return format::eac_r11_unorm_block; break;
		case VK_FORMAT_EAC_R11_SNORM_BLOCK: return format::eac_r11_snorm_block; break;
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return format::eac_r11g11_unorm_block; break;
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return format::eac_r11g11_snorm_block; break;
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return format::astc_4x4_unorm_block; break;
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return format::astc_4x4_srgb_block; break;
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return format::astc_5x4_unorm_block; break;
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return format::astc_5x4_srgb_block; break;
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return format::astc_5x5_unorm_block; break;
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return format::astc_5x5_srgb_block; break;
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return format::astc_6x5_unorm_block; break;
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return format::astc_6x5_srgb_block; break;
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return format::astc_6x6_unorm_block; break;
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return format::astc_6x6_srgb_block; break;
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return format::astc_8x5_unorm_block; break;
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return format::astc_8x5_srgb_block; break;
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return format::astc_8x6_unorm_block; break;
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return format::astc_8x6_srgb_block; break;
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return format::astc_8x8_unorm_block; break;
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return format::astc_8x8_srgb_block; break;
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return format::astc_10x5_unorm_block; break;
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return format::astc_10x5_srgb_block; break;
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return format::astc_10x6_unorm_block; break;
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return format::astc_10x6_srgb_block; break;
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return format::astc_10x8_unorm_block; break;
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return format::astc_10x8_srgb_block; break;
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return format::astc_10x10_unorm_block; break;
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return format::astc_10x10_srgb_block; break;
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return format::astc_12x10_unorm_block; break;
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return format::astc_12x10_srgb_block; break;
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return format::astc_12x12_unorm_block; break;
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return format::astc_12x12_srgb_block; break;
		case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: return format::pvrtc1_2bpp_unorm_block_img; break;
		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: return format::pvrtc1_4bpp_unorm_block_img; break;
		case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: return format::pvrtc2_2bpp_unorm_block_img; break;
		case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: return format::pvrtc2_4bpp_unorm_block_img; break;
		case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: return format::pvrtc1_2bpp_srgb_block_img; break;
		case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG: return format::pvrtc1_4bpp_srgb_block_img; break;
		case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: return format::pvrtc2_2bpp_srgb_block_img; break;
		case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG: return format::pvrtc2_4bpp_srgb_block_img; break;
		case VK_FORMAT_UNDEFINED: return format::undefined; break;
		default: assert(false);
		}
		return format::undefined;
	}
	inline vk::Format to_vk(format value) noexcept
	{
		VkFormat result = VK_FORMAT_UNDEFINED;
		switch(value)
		{
		case format::undefined: result = VK_FORMAT_UNDEFINED; break;
		case format::r4g4_unorm_pack8: result = VK_FORMAT_R4G4_UNORM_PACK8; break;
		case format::r4g4b4a4_unorm_pack16: result = VK_FORMAT_R4G4B4A4_UNORM_PACK16; break;
		case format::b4g4r4a4_unorm_pack16: result = VK_FORMAT_B4G4R4A4_UNORM_PACK16; break;
		case format::r5g6b5_unorm_pack16: result = VK_FORMAT_R5G6B5_UNORM_PACK16; break;
		case format::b5g6r5_unorm_pack16: result = VK_FORMAT_B5G6R5_UNORM_PACK16; break;
		case format::r5g5b5a1_unorm_pack16: result = VK_FORMAT_R5G5B5A1_UNORM_PACK16; break;
		case format::b5g5r5a1_unorm_pack16: result = VK_FORMAT_B5G5R5A1_UNORM_PACK16; break;
		case format::a1r5g5b5_unorm_pack16: result = VK_FORMAT_A1R5G5B5_UNORM_PACK16; break;
		case format::r8_unorm: result = VK_FORMAT_R8_UNORM; break;
		case format::r8_snorm: result = VK_FORMAT_R8_SNORM; break;
		case format::r8_uint: result = VK_FORMAT_R8_UINT; break;
		case format::r8_sint: result = VK_FORMAT_R8_SINT; break;
		case format::r8_srgb: result = VK_FORMAT_R8_SRGB; break;
		case format::r8g8_unorm: result = VK_FORMAT_R8G8_UNORM; break;
		case format::r8g8_snorm: result = VK_FORMAT_R8G8_SNORM; break;
		case format::r8g8_uint: result = VK_FORMAT_R8G8_UINT; break;
		case format::r8g8_sint: result = VK_FORMAT_R8G8_SINT; break;
		case format::r8g8_srgb: result = VK_FORMAT_R8G8_SRGB; break;
		case format::r8g8b8_unorm: result = VK_FORMAT_R8G8B8_UNORM; break;
		case format::r8g8b8_snorm: result = VK_FORMAT_R8G8B8_SNORM; break;
		case format::r8g8b8_uint: result = VK_FORMAT_R8G8B8_UINT; break;
		case format::r8g8b8_sint: result = VK_FORMAT_R8G8B8_SINT; break;
		case format::r8g8b8_srgb: result = VK_FORMAT_R8G8B8_SRGB; break;
		case format::b8g8r8_unorm: result = VK_FORMAT_B8G8R8_UNORM; break;
		case format::b8g8r8_snorm: result = VK_FORMAT_B8G8R8_SNORM; break;
		case format::b8g8r8_uint: result = VK_FORMAT_B8G8R8_UINT; break;
		case format::b8g8r8_sint: result = VK_FORMAT_B8G8R8_SINT; break;
		case format::b8g8r8_srgb: result = VK_FORMAT_B8G8R8_SRGB; break;
		case format::r8g8b8a8_unorm: result = VK_FORMAT_R8G8B8A8_UNORM; break;
		case format::r8g8b8a8_snorm: result = VK_FORMAT_R8G8B8A8_SNORM; break;
		case format::r8g8b8a8_uint: result = VK_FORMAT_R8G8B8A8_UINT; break;
		case format::r8g8b8a8_sint: result = VK_FORMAT_R8G8B8A8_SINT; break;
		case format::r8g8b8a8_srgb: result = VK_FORMAT_R8G8B8A8_SRGB; break;
		case format::b8g8r8a8_unorm: result = VK_FORMAT_B8G8R8A8_UNORM; break;
		case format::b8g8r8a8_snorm: result = VK_FORMAT_B8G8R8A8_SNORM; break;
		case format::b8g8r8a8_uint: result = VK_FORMAT_B8G8R8A8_UINT; break;
		case format::b8g8r8a8_sint: result = VK_FORMAT_B8G8R8A8_SINT; break;
		case format::b8g8r8a8_srgb: result = VK_FORMAT_B8G8R8A8_SRGB; break;
		case format::a2r10g10b10_unorm_pack32: result = VK_FORMAT_A2R10G10B10_UNORM_PACK32; break;
		case format::a2r10g10b10_uint_pack32: result = VK_FORMAT_A2R10G10B10_UINT_PACK32; break;
		case format::a2b10g10r10_unorm_pack32: result = VK_FORMAT_A2B10G10R10_UNORM_PACK32; break;
		case format::a2b10g10r10_uint_pack32: result = VK_FORMAT_A2B10G10R10_UINT_PACK32; break;
		case format::r16_unorm: result = VK_FORMAT_R16_UNORM; break;
		case format::r16_snorm: result = VK_FORMAT_R16_SNORM; break;
		case format::r16_uint: result = VK_FORMAT_R16_UINT; break;
		case format::r16_sint: result = VK_FORMAT_R16_SINT; break;
		case format::r16_sfloat: result = VK_FORMAT_R16_SFLOAT; break;
		case format::r16g16_unorm: result = VK_FORMAT_R16G16_UNORM; break;
		case format::r16g16_snorm: result = VK_FORMAT_R16G16_SNORM; break;
		case format::r16g16_uint: result = VK_FORMAT_R16G16_UINT; break;
		case format::r16g16_sint: result = VK_FORMAT_R16G16_SINT; break;
		case format::r16g16_sfloat: result = VK_FORMAT_R16G16_SFLOAT; break;
		case format::r16g16b16_unorm: result = VK_FORMAT_R16G16B16_UNORM; break;
		case format::r16g16b16_snorm: result = VK_FORMAT_R16G16B16_SNORM; break;
		case format::r16g16b16_uint: result = VK_FORMAT_R16G16B16_UINT; break;
		case format::r16g16b16_sint: result = VK_FORMAT_R16G16B16_SINT; break;
		case format::r16g16b16_sfloat: result = VK_FORMAT_R16G16B16_SFLOAT; break;
		case format::r16g16b16a16_unorm: result = VK_FORMAT_R16G16B16A16_UNORM; break;
		case format::r16g16b16a16_snorm: result = VK_FORMAT_R16G16B16A16_SNORM; break;
		case format::r16g16b16a16_uint: result = VK_FORMAT_R16G16B16A16_UINT; break;
		case format::r16g16b16a16_sint: result = VK_FORMAT_R16G16B16A16_SINT; break;
		case format::r16g16b16a16_sfloat: result = VK_FORMAT_R16G16B16A16_SFLOAT; break;
		case format::r32_uint: result = VK_FORMAT_R32_UINT; break;
		case format::r32_sint: result = VK_FORMAT_R32_SINT; break;
		case format::r32_sfloat: result = VK_FORMAT_R32_SFLOAT; break;
		case format::r32g32_uint: result = VK_FORMAT_R32G32_UINT; break;
		case format::r32g32_sint: result = VK_FORMAT_R32G32_SINT; break;
		case format::r32g32_sfloat: result = VK_FORMAT_R32G32_SFLOAT; break;
		case format::r32g32b32_uint: result = VK_FORMAT_R32G32B32_UINT; break;
		case format::r32g32b32_sint: result = VK_FORMAT_R32G32B32_SINT; break;
		case format::r32g32b32_sfloat: result = VK_FORMAT_R32G32B32_SFLOAT; break;
		case format::r32g32b32a32_uint: result = VK_FORMAT_R32G32B32A32_UINT; break;
		case format::r32g32b32a32_sint: result = VK_FORMAT_R32G32B32A32_SINT; break;
		case format::r32g32b32a32_sfloat: result = VK_FORMAT_R32G32B32A32_SFLOAT; break;
		case format::b10g11r11_ufloat_pack32: result = VK_FORMAT_B10G11R11_UFLOAT_PACK32; break;
		case format::e5b9g9r9_ufloat_pack32: result = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32; break;
		case format::d16_unorm: result = VK_FORMAT_D16_UNORM; break;
		case format::x8_d24_unorm_pack32: result = VK_FORMAT_X8_D24_UNORM_PACK32; break;
		case format::d32_sfloat: result = VK_FORMAT_D32_SFLOAT; break;
		case format::s8_uint: result = VK_FORMAT_S8_UINT; break;
		case format::d24_unorm_s8_uint: result = VK_FORMAT_D24_UNORM_S8_UINT; break;
		case format::d32_sfloat_s8_uint: result = VK_FORMAT_D32_SFLOAT_S8_UINT; break;
		case format::bc1_rgb_unorm_block: result = VK_FORMAT_BC1_RGB_UNORM_BLOCK; break;
		case format::bc1_rgb_srgb_block: result = VK_FORMAT_BC1_RGB_SRGB_BLOCK; break;
		case format::bc1_rgba_unorm_block: result = VK_FORMAT_BC1_RGBA_UNORM_BLOCK; break;
		case format::bc1_rgba_srgb_block: result = VK_FORMAT_BC1_RGBA_SRGB_BLOCK; break;
		case format::bc2_unorm_block: result = VK_FORMAT_BC2_UNORM_BLOCK; break;
		case format::bc2_srgb_block: result = VK_FORMAT_BC2_SRGB_BLOCK; break;
		case format::bc3_unorm_block: result = VK_FORMAT_BC3_UNORM_BLOCK; break;
		case format::bc3_srgb_block: result = VK_FORMAT_BC3_SRGB_BLOCK; break;
		case format::bc4_unorm_block: result = VK_FORMAT_BC4_UNORM_BLOCK; break;
		case format::bc4_snorm_block: result = VK_FORMAT_BC4_SNORM_BLOCK; break;
		case format::bc5_unorm_block: result = VK_FORMAT_BC5_UNORM_BLOCK; break;
		case format::bc5_snorm_block: result = VK_FORMAT_BC5_SNORM_BLOCK; break;
		case format::bc6h_ufloat_block: result = VK_FORMAT_BC6H_UFLOAT_BLOCK; break;
		case format::bc6h_sfloat_block: result = VK_FORMAT_BC6H_SFLOAT_BLOCK; break;
		case format::bc7_unorm_block: result = VK_FORMAT_BC7_UNORM_BLOCK; break;
		case format::bc7_srgb_block: result = VK_FORMAT_BC7_SRGB_BLOCK; break;
		case format::etc2_r8g8b8_unorm_block: result = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK; break;
		case format::etc2_r8g8b8_srgb_block: result = VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK; break;
		case format::etc2_r8g8b8a1_unorm_block: result = VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK; break;
		case format::etc2_r8g8b8a1_srgb_block: result = VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK; break;
		case format::etc2_r8g8b8a8_unorm_block: result = VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK; break;
		case format::etc2_r8g8b8a8_srgb_block: result = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK; break;
		case format::eac_r11_unorm_block: result = VK_FORMAT_EAC_R11_UNORM_BLOCK; break;
		case format::eac_r11_snorm_block: result = VK_FORMAT_EAC_R11_SNORM_BLOCK; break;
		case format::eac_r11g11_unorm_block: result = VK_FORMAT_EAC_R11G11_UNORM_BLOCK; break;
		case format::eac_r11g11_snorm_block: result = VK_FORMAT_EAC_R11G11_SNORM_BLOCK; break;
		case format::astc_4x4_unorm_block: result = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
		case format::astc_4x4_srgb_block: result = VK_FORMAT_ASTC_4x4_SRGB_BLOCK; break;
		case format::astc_5x4_unorm_block: result = VK_FORMAT_ASTC_5x4_UNORM_BLOCK; break;
		case format::astc_5x4_srgb_block: result = VK_FORMAT_ASTC_5x4_SRGB_BLOCK; break;
		case format::astc_5x5_unorm_block: result = VK_FORMAT_ASTC_5x5_UNORM_BLOCK; break;
		case format::astc_5x5_srgb_block: result = VK_FORMAT_ASTC_5x5_SRGB_BLOCK; break;
		case format::astc_6x5_unorm_block: result = VK_FORMAT_ASTC_6x5_UNORM_BLOCK; break;
		case format::astc_6x5_srgb_block: result = VK_FORMAT_ASTC_6x5_SRGB_BLOCK; break;
		case format::astc_6x6_unorm_block: result = VK_FORMAT_ASTC_6x6_UNORM_BLOCK; break;
		case format::astc_6x6_srgb_block: result = VK_FORMAT_ASTC_6x6_SRGB_BLOCK; break;
		case format::astc_8x5_unorm_block: result = VK_FORMAT_ASTC_8x5_UNORM_BLOCK; break;
		case format::astc_8x5_srgb_block: result = VK_FORMAT_ASTC_8x5_SRGB_BLOCK; break;
		case format::astc_8x6_unorm_block: result = VK_FORMAT_ASTC_8x6_UNORM_BLOCK; break;
		case format::astc_8x6_srgb_block: result = VK_FORMAT_ASTC_8x6_SRGB_BLOCK; break;
		case format::astc_8x8_unorm_block: result = VK_FORMAT_ASTC_8x8_UNORM_BLOCK; break;
		case format::astc_8x8_srgb_block: result = VK_FORMAT_ASTC_8x8_SRGB_BLOCK; break;
		case format::astc_10x5_unorm_block: result = VK_FORMAT_ASTC_10x5_UNORM_BLOCK; break;
		case format::astc_10x5_srgb_block: result = VK_FORMAT_ASTC_10x5_SRGB_BLOCK; break;
		case format::astc_10x6_unorm_block: result = VK_FORMAT_ASTC_10x6_UNORM_BLOCK; break;
		case format::astc_10x6_srgb_block: result = VK_FORMAT_ASTC_10x6_SRGB_BLOCK; break;
		case format::astc_10x8_unorm_block: result = VK_FORMAT_ASTC_10x8_UNORM_BLOCK; break;
		case format::astc_10x8_srgb_block: result = VK_FORMAT_ASTC_10x8_SRGB_BLOCK; break;
		case format::astc_10x10_unorm_block: result = VK_FORMAT_ASTC_10x10_UNORM_BLOCK; break;
		case format::astc_10x10_srgb_block: result = VK_FORMAT_ASTC_10x10_SRGB_BLOCK; break;
		case format::astc_12x10_unorm_block: result = VK_FORMAT_ASTC_12x10_UNORM_BLOCK; break;
		case format::astc_12x10_srgb_block: result = VK_FORMAT_ASTC_12x10_SRGB_BLOCK; break;
		case format::astc_12x12_unorm_block: result = VK_FORMAT_ASTC_12x12_UNORM_BLOCK; break;
		case format::astc_12x12_srgb_block: result = VK_FORMAT_ASTC_12x12_SRGB_BLOCK; break;
		case format::pvrtc1_2bpp_unorm_block_img: result = VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG; break;
		case format::pvrtc1_4bpp_unorm_block_img: result = VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG; break;
		case format::pvrtc2_2bpp_unorm_block_img: result = VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG; break;
		case format::pvrtc2_4bpp_unorm_block_img: result = VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG; break;
		case format::pvrtc1_2bpp_srgb_block_img: result = VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG; break;
		case format::pvrtc1_4bpp_srgb_block_img: result = VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG; break;
		case format::pvrtc2_2bpp_srgb_block_img: result = VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG; break;
		case format::pvrtc2_4bpp_srgb_block_img: result = VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG; break;
		default: assert(false);
		}

		return vk::Format{result};
	}
#endif
#ifdef PE_GLES
	inline bool to_gles(format value, GLint& internalFormat, GLint& format, GLint& type) noexcept
	{
		internalFormat = -1;
		switch(value)
		{
		case format::undefined:
			internalFormat = 0;
			format		   = 0;
			type		   = 0;
			break;
		case format::b4g4r4a4_unorm_pack16:
			internalFormat = GL_RGBA4;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_SHORT_4_4_4_4;
			break;
		case format::r4g4b4a4_unorm_pack16: assert(false); break;
		case format::b5g6r5_unorm_pack16:
			internalFormat = GL_RGB565;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_SHORT_5_6_5;
			break;
		case format::r5g6b5_unorm_pack16: assert(false); break;
		case format::a1r5g5b5_unorm_pack16:
			internalFormat = GL_RGB5_A1;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_SHORT_5_5_5_1;
			break;
		case format::r5g5b5a1_unorm_pack16:
		case format::b5g5r5a1_unorm_pack16: assert(false); break;
		case format::r8_unorm:
			internalFormat = GL_R8;
			format		   = GL_RED;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::r8_snorm:
			internalFormat = GL_R8_SNORM;
			format		   = GL_RED;
			type		   = GL_BYTE;
			break;
		case format::r8_uint:
			internalFormat = GL_R8UI;
			format		   = GL_RED_INTEGER;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::r8_sint:
			internalFormat = GL_R8I;
			format		   = GL_RED_INTEGER;
			type		   = GL_BYTE;
			break;
		case format::r8_srgb: assert(false); break;
		case format::r8g8_unorm:
			internalFormat = GL_RG8;
			format		   = GL_RG;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::r8g8_snorm:
			internalFormat = GL_RG8_SNORM;
			format		   = GL_RG;
			type		   = GL_BYTE;
			break;
		case format::r8g8_uint:
			internalFormat = GL_RG8UI;
			format		   = GL_RG_INTEGER;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::r8g8_sint:
			internalFormat = GL_RG8I;
			format		   = GL_RG_INTEGER;
			type		   = GL_BYTE;
			break;
		case format::r8g8_srgb: assert(false); break;

		case format::r8g8b8_unorm:
			internalFormat = GL_RGB8;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::r8g8b8_snorm:
			internalFormat = GL_RGB8_SNORM;
			format		   = GL_RGB;
			type		   = GL_BYTE;
			break;
		case format::r8g8b8_uint:
			internalFormat = GL_RGB8UI;
			format		   = GL_RGB_INTEGER;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::r8g8b8_sint:
			internalFormat = GL_RGB8I;
			format		   = GL_RGB_INTEGER;
			type		   = GL_BYTE;
			break;
		case format::r8g8b8_srgb:
			internalFormat = GL_SRGB8;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::b8g8r8_unorm:
		case format::b8g8r8_snorm:
		case format::b8g8r8_uint:
		case format::b8g8r8_sint:
		case format::b8g8r8_srgb: assert(false); break;

		case format::r8g8b8a8_unorm:
			internalFormat = GL_RGBA8;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::r8g8b8a8_snorm:
			internalFormat = GL_RGBA8_SNORM;
			format		   = GL_RGBA;
			type		   = GL_BYTE;
			break;
		case format::r8g8b8a8_uint:
			internalFormat = GL_RGBA8UI;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::r8g8b8a8_sint:
			internalFormat = GL_RGBA8I;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_BYTE;
			break;
		case format::r8g8b8a8_srgb:
			internalFormat = GL_SRGB8_ALPHA8;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::b8g8r8a8_unorm:
		case format::b8g8r8a8_snorm:
		case format::b8g8r8a8_uint:
		case format::b8g8r8a8_sint:
		case format::b8g8r8a8_srgb:
		case format::a2r10g10b10_unorm_pack32:
		case format::a2r10g10b10_uint_pack32:
		case format::a2b10g10r10_unorm_pack32:
		case format::a2b10g10r10_uint_pack32:
		case format::r16_unorm:
		case format::r16_snorm: assert(false); break;

		case format::r16_uint:
			internalFormat = GL_R16UI;
			format		   = GL_RED_INTEGER;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format::r16_sint:
			internalFormat = GL_R16I;
			format		   = GL_RED_INTEGER;
			type		   = GL_SHORT;
			break;
		case format::r16_sfloat:
			internalFormat = GL_R16F;
			format		   = GL_RED;
			type		   = GL_HALF_FLOAT;
			break;
		case format::r16g16_unorm:
		case format::r16g16_snorm: assert(false); break;

		case format::r16g16_uint:
			internalFormat = GL_RG16UI;
			format		   = GL_RG_INTEGER;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format::r16g16_sint:
			internalFormat = GL_RG16I;
			format		   = GL_RG_INTEGER;
			type		   = GL_SHORT;
			break;
		case format::r16g16_sfloat:
			internalFormat = GL_RG16F;
			format		   = GL_RG;
			type		   = GL_HALF_FLOAT;
			break;
		case format::r16g16b16_unorm:
		case format::r16g16b16_snorm: assert(false); break;

		case format::r16g16b16_uint:
			internalFormat = GL_RGB16UI;
			format		   = GL_RGB_INTEGER;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format::r16g16b16_sint:
			internalFormat = GL_RGB16I;
			format		   = GL_RGB_INTEGER;
			type		   = GL_SHORT;
			break;
		case format::r16g16b16_sfloat:
			internalFormat = GL_RGB16F;
			format		   = GL_RGB;
			type		   = GL_HALF_FLOAT;
			break;
		case format::r16g16b16a16_unorm:
		case format::r16g16b16a16_snorm: assert(false); break;

		case format::r16g16b16a16_uint:
			internalFormat = GL_RGBA16UI;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format::r16g16b16a16_sint:
			internalFormat = GL_RGBA16I;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_SHORT;
			break;
		case format::r16g16b16a16_sfloat:
			internalFormat = GL_RGBA16F;
			format		   = GL_RGBA;
			type		   = GL_HALF_FLOAT;
			break;
		case format::r32_uint:
			internalFormat = GL_R32UI;
			format		   = GL_RED_INTEGER;
			type		   = GL_UNSIGNED_INT;
			break;
		case format::r32_sint:
			internalFormat = GL_R32I;
			format		   = GL_RED_INTEGER;
			type		   = GL_INT;
			break;
		case format::r32_sfloat:
			internalFormat = GL_R32F;
			format		   = GL_RED;
			type		   = GL_FLOAT;
			break;
		case format::r32g32_uint:
			internalFormat = GL_RG32UI;
			format		   = GL_RG_INTEGER;
			type		   = GL_UNSIGNED_INT;
			break;
		case format::r32g32_sint:
			internalFormat = GL_RG32I;
			format		   = GL_RG_INTEGER;
			type		   = GL_INT;
			break;
		case format::r32g32_sfloat:
			internalFormat = GL_RG32F;
			format		   = GL_RG;
			type		   = GL_FLOAT;
			break;
		case format::r32g32b32_uint:
			internalFormat = GL_RGB32UI;
			format		   = GL_RGB_INTEGER;
			type		   = GL_UNSIGNED_INT;
			break;
		case format::r32g32b32_sint:
			internalFormat = GL_RGB32I;
			format		   = GL_RGB_INTEGER;
			type		   = GL_INT;
			break;
		case format::r32g32b32_sfloat:
			internalFormat = GL_RGB32F;
			format		   = GL_RGB;
			type		   = GL_FLOAT;
			break;
		case format::r32g32b32a32_uint:
			internalFormat = GL_RGBA32UI;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_UNSIGNED_INT;
			break;
		case format::r32g32b32a32_sint:
			internalFormat = GL_RGBA32I;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_INT;
			break;
		case format::r32g32b32a32_sfloat:
			internalFormat = GL_RGBA32F;
			format		   = GL_RGBA;
			type		   = GL_FLOAT;
			break;
		case format::b10g11r11_ufloat_pack32:
			internalFormat = GL_R11F_G11F_B10F;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_INT_10F_11F_11F_REV;
			break;
		case format::e5b9g9r9_ufloat_pack32:
			internalFormat = GL_RGB9_E5;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_INT_5_9_9_9_REV;
			break;
		case format::d16_unorm:
			internalFormat = GL_DEPTH_COMPONENT16;
			format		   = GL_DEPTH_COMPONENT;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format::x8_d24_unorm_pack32:
			internalFormat = GL_DEPTH_COMPONENT24;
			format		   = GL_DEPTH_COMPONENT;
			type		   = GL_UNSIGNED_INT;
			break;
		case format::d32_sfloat:
			internalFormat = GL_DEPTH_COMPONENT32F;
			format		   = GL_DEPTH_COMPONENT;
			type		   = GL_FLOAT;
			break;
		case format::s8_uint:
			internalFormat = GL_STENCIL_INDEX8;
			format		   = GL_STENCIL_INDEX;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format::d24_unorm_s8_uint:
			internalFormat = GL_DEPTH24_STENCIL8;
			format		   = GL_DEPTH_STENCIL;
			type		   = GL_UNSIGNED_INT_24_8;
			break;
		case format::d32_sfloat_s8_uint:
			internalFormat = GL_DEPTH32F_STENCIL8;
			format		   = GL_DEPTH_STENCIL;
			type		   = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
			break;
		case format::bc1_rgb_unorm_block:
		case format::bc1_rgb_srgb_block:
		case format::bc1_rgba_unorm_block:
		case format::bc1_rgba_srgb_block:
		case format::bc2_unorm_block:
		case format::bc2_srgb_block:
		case format::bc3_unorm_block:
		case format::bc3_srgb_block:
		case format::bc4_unorm_block:
		case format::bc4_snorm_block:
		case format::bc5_unorm_block:
		case format::bc5_snorm_block:
		case format::bc6h_ufloat_block:
		case format::bc6h_sfloat_block:
		case format::bc7_unorm_block:
		case format::bc7_srgb_block: assert(false); break;
		case format::etc2_r8g8b8_unorm_block:
			internalFormat = GL_COMPRESSED_RGB8_ETC2;
			format		   = 0;
			type		   = 0;
			break;
		case format::etc2_r8g8b8_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ETC2;
			format		   = 0;
			type		   = 0;
			break;
		case format::etc2_r8g8b8a1_unorm_block:
			internalFormat = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
			format		   = 0;
			type		   = 0;
			break;
		case format::etc2_r8g8b8a1_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2;
			format		   = 0;
			type		   = 0;
			break;
		case format::etc2_r8g8b8a8_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format::etc2_r8g8b8a8_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format::eac_r11_unorm_block:
			internalFormat = GL_COMPRESSED_R11_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format::eac_r11_snorm_block:
			internalFormat = GL_COMPRESSED_SIGNED_R11_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format::eac_r11g11_unorm_block:
			internalFormat = GL_COMPRESSED_RG11_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format::eac_r11g11_snorm_block:
			internalFormat = GL_COMPRESSED_SIGNED_RG11_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_4x4_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_4x4;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_4x4_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_5x4_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_5x4;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_5x4_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_5x5_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_5x5;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_5x5_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_6x5_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_6x5;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_6x5_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_6x6_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_6x6;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_6x6_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_8x5_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_8x5;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_8x5_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_8x6_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_8x6;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_8x6_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_8x8_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_8x8;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_8x8_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_10x5_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_10x5;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_10x5_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_10x6_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_10x6;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_10x6_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_10x8_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_10x8;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_10x8_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_10x10_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_10x10;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_10x10_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_12x10_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_12x10;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_12x10_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_12x12_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_12x12;
			format		   = 0;
			type		   = 0;
			break;
		case format::astc_12x12_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12;
			format		   = 0;
			type		   = 0;
			break;
#ifndef GL_IMG_texture_compression_pvrtc
		case format::pvrtc1_2bpp_unorm_block_img:
		case format::pvrtc1_4bpp_unorm_block_img:
		case format::pvrtc2_2bpp_unorm_block_img:
		case format::pvrtc2_4bpp_unorm_block_img:
		case format::pvrtc1_2bpp_srgb_block_img:
		case format::pvrtc1_4bpp_srgb_block_img:
		case format::pvrtc2_2bpp_srgb_block_img:
		case format::pvrtc2_4bpp_srgb_block_img: assert(false); break;
#else
		case format::pvrtc1_2bpp_unorm_block_img:
			internalFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format::pvrtc1_4bpp_unorm_block_img:
			internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format::pvrtc2_2bpp_unorm_block_img:
			internalFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format::pvrtc2_4bpp_unorm_block_img:
			internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format::pvrtc1_2bpp_srgb_block_img:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format::pvrtc1_4bpp_srgb_block_img:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format::pvrtc2_2bpp_srgb_block_img:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format::pvrtc2_4bpp_srgb_block_img:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG;
			format		   = 0;
			type		   = 0;
			break;
#endif
		default: assert(false);
		}

		return internalFormat != -1;
	}
#endif

	enum class sampler_mipmap_mode
	{
		nearest = 0,
		linear  = 1
	};

#ifdef PE_VULKAN
	inline vk::SamplerMipmapMode to_vk(sampler_mipmap_mode value) noexcept
	{
		return vk::SamplerMipmapMode(static_cast<std::underlying_type_t<sampler_mipmap_mode>>(value));
	}
#endif
	enum class sampler_address_mode
	{
		repeat				 = 0,
		mirrored_repeat		 = 1,
		clamp_to_edge		 = 2,
		clamp_to_border		 = 3,
		mirror_clamp_to_edge = 4
	};

#ifdef PE_VULKAN

	inline vk::SamplerAddressMode to_vk(sampler_address_mode value) noexcept
	{
		return vk::SamplerAddressMode(static_cast<std::underlying_type_t<sampler_address_mode>>(value));
	}
#endif
#ifdef PE_GLES
	inline GLuint to_gles(sampler_address_mode value) noexcept
	{
		switch(value)
		{
		case sampler_address_mode::repeat: return GL_REPEAT; break;
		case sampler_address_mode::mirrored_repeat: return GL_MIRRORED_REPEAT; break;
		case sampler_address_mode::clamp_to_edge: return GL_CLAMP_TO_EDGE; break;
		case sampler_address_mode::clamp_to_border: return GL_CLAMP_TO_BORDER; break;
#ifdef GL_EXT_texture_mirror_clamp
		case sampler_address_mode::mirror_clamp_to_edge: return GL_MIRROR_CLAMP_TO_EDGE_EXT; break;
#else
		case sampler_address_mode::mirror_clamp_to_edge: return GL_REPEAT; break;
#endif
		}

		assert(false);
		return GL_REPEAT;
	}
#endif

	enum class border_color
	{
		float_transparent_black = 0,
		int_transparent_black   = 1,
		float_opaque_black		= 2,
		int_opaque_black		= 3,
		float_opaque_white		= 4,
		int_opaque_white		= 5
	};

#ifdef PE_VULKAN

	inline vk::BorderColor to_vk(border_color value) noexcept
	{
		return vk::BorderColor(static_cast<std::underlying_type_t<border_color>>(value));
	}
#endif

#ifdef PE_GLES
	inline void to_gles(border_color value, GLint sampler) noexcept
	{
		float fvalues[]{0.0f, 0.0f, 0.0f, 0.0f};
		int ivalues[]{0, 0, 0, 0};
		switch(value)
		{
		case border_color::float_transparent_black:
			glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, fvalues);
			break;
		case border_color::int_transparent_black:
			glSamplerParameterIiv(sampler, GL_TEXTURE_BORDER_COLOR, ivalues);
			break;
		case border_color::float_opaque_black:
			fvalues[3] = 1.0f;
			glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, fvalues);
			break;
		case border_color::int_opaque_black:
			ivalues[3] = 1;
			glSamplerParameterIiv(sampler, GL_TEXTURE_BORDER_COLOR, ivalues);
			break;
		case border_color::float_opaque_white:
			std::fill(std::begin(fvalues), std::end(fvalues), 1.0f);
			glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, fvalues);
			break;
		case border_color::int_opaque_white:
			std::fill(std::begin(ivalues), std::end(ivalues), 1);
			glSamplerParameterIiv(sampler, GL_TEXTURE_BORDER_COLOR, ivalues);
			break;
		}
	}
#endif

	enum class cullmode
	{
		none  = 0,
		front = 1,
		back  = 2,
		all   = 3
	};

	namespace conversion
	{
#ifdef PE_VULKAN
		inline vk::CullModeFlags to_vk(cullmode value) noexcept
		{
			return vk::CullModeFlags(static_cast<std::underlying_type_t<cullmode>>(value));
		}

		inline cullmode to_cullmode(vk::CullModeFlags value) noexcept
		{
			return cullmode(static_cast<VkCullModeFlags>(value));
		}

#endif
#ifdef PE_GLES
		inline GLuint to_gles(cullmode value) noexcept
		{
			switch(value)
			{
			case cullmode::none: return GL_NONE; break;
			case cullmode::front: return GL_FRONT; break;
			case cullmode::back: return GL_BACK; break;
			case cullmode::all: return GL_FRONT_AND_BACK; break;
			}
			assert(false);
			return GL_BACK;
		}
#endif
	} // namespace conversion


	enum class compare_op
	{
		never		  = 0,
		less		  = 1,
		equal		  = 2,
		less_equal	= 3,
		greater		  = 4,
		not_equal	 = 5,
		greater_equal = 6,
		always		  = 7

	};
	namespace conversion
	{
#ifdef PE_VULKAN
		inline vk::CompareOp to_vk(compare_op value) noexcept
		{
			return vk::CompareOp(static_cast<std::underlying_type_t<compare_op>>(value));
		}

		inline compare_op to_compare_op(vk::CompareOp value) noexcept
		{
			return compare_op(static_cast<std::underlying_type_t<vk::CompareOp>>(value));
		}
#endif
#ifdef PE_GLES
		inline GLuint to_gles(compare_op value) noexcept
		{
			switch(value)
			{
			case compare_op::never: return GL_NEVER; break;
			case compare_op::less: return GL_LESS; break;
			case compare_op::equal: return GL_EQUAL; break;
			case compare_op::less_equal: return GL_LEQUAL; break;
			case compare_op::greater: return GL_GREATER; break;
			case compare_op::not_equal: return GL_NOTEQUAL; break;
			case compare_op::greater_equal: return GL_GEQUAL; break;
			case compare_op::always: return GL_ALWAYS; break;
			}
			assert(false);
			return GL_NEVER;
		}
#endif

	} // namespace conversion

	enum class filter
	{
		nearest = 0,
		linear  = 1,
		cubic   = 2
	};

#ifdef PE_VULKAN
	inline vk::Filter to_vk(filter value) noexcept
	{
		return vk::Filter(static_cast<std::underlying_type_t<filter>>(value));
	}
#endif
#ifdef PE_GLES
	inline GLuint to_gles(filter value) noexcept
	{
		switch(value)
		{
		case filter::nearest: return GL_NEAREST; break;
		case filter::linear: return GL_LINEAR; break;
#ifdef GL_IMG_texture_filter_cubic
		case filter::cubic: return CUBIC_IMG; break;
#else
		case filter::cubic: return GL_LINEAR; break;
#endif
		}
		assert(false);
		return GL_LINEAR;
	}
#endif

	/// \brief these signify the binding type in the shader
	enum class binding_type
	{
		sampler,
		combined_image_sampler,
		sampled_image,
		storage_image,
		uniform_texel_buffer,
		storage_texel_buffer,
		uniform_buffer,
		storage_buffer,
		uniform_buffer_dynamic,
		storage_buffer_dynamic,
		input_attachment
	};

	namespace conversion
	{
#ifdef PE_VULKAN
		inline vk::DescriptorType to_vk(binding_type value) noexcept
		{
			switch(value)
			{
			case binding_type::sampler: return vk::DescriptorType::eSampler; break;
			case binding_type::combined_image_sampler: return vk::DescriptorType::eCombinedImageSampler; break;
			case binding_type::sampled_image: return vk::DescriptorType::eSampledImage; break;
			case binding_type::storage_image: return vk::DescriptorType::eStorageImage; break;
			case binding_type::uniform_texel_buffer: return vk::DescriptorType::eUniformTexelBuffer; break;
			case binding_type::storage_texel_buffer: return vk::DescriptorType::eStorageTexelBuffer; break;
			case binding_type::uniform_buffer: return vk::DescriptorType::eUniformBuffer; break;
			case binding_type::storage_buffer: return vk::DescriptorType::eStorageBuffer; break;
			case binding_type::uniform_buffer_dynamic: return vk::DescriptorType::eUniformBufferDynamic; break;
			case binding_type::storage_buffer_dynamic: return vk::DescriptorType::eStorageBufferDynamic; break;
			case binding_type::input_attachment: return vk::DescriptorType::eInputAttachment; break;
			}
			assert(false);
			return vk::DescriptorType{0};
		}

		inline binding_type to_binding_type(vk::DescriptorType value) noexcept 
		{
			switch(value)
			{
			case vk::DescriptorType::eSampler: return binding_type::sampler; break;
			case vk::DescriptorType::eCombinedImageSampler: return binding_type::combined_image_sampler; break;
			case vk::DescriptorType::eSampledImage: return binding_type::sampled_image; break;
			case vk::DescriptorType::eStorageImage: return binding_type::storage_image; break;
			case vk::DescriptorType::eUniformTexelBuffer: return binding_type::uniform_texel_buffer; break;
			case vk::DescriptorType::eStorageTexelBuffer: return binding_type::storage_texel_buffer; break;
			case vk::DescriptorType::eUniformBuffer: return binding_type::uniform_buffer; break;
			case vk::DescriptorType::eStorageBuffer: return binding_type::storage_buffer; break;
			case vk::DescriptorType::eUniformBufferDynamic: return binding_type::uniform_buffer_dynamic; break;
			case vk::DescriptorType::eStorageBufferDynamic: return binding_type::storage_buffer_dynamic; break;
			case vk::DescriptorType::eInputAttachment: return binding_type::input_attachment; break;
			}
			assert(false);
			return binding_type{0};
		}
#endif
	} // namespace conversion
	enum class blend_op
	{
		add				 = 0,
		subtract		 = 1,
		reverse_subtract = 2,
		min				 = 3,
		max				 = 4
	};

	namespace conversion
	{
#ifdef PE_VULKAN
		inline vk::BlendOp to_vk(blend_op value) noexcept
		{
			return vk::BlendOp{static_cast<std::underlying_type_t<blend_op>>(value)};
		}

		inline blend_op to_blend_op(vk::BlendOp value) noexcept
		{
			return blend_op(static_cast<std::underlying_type_t<vk::BlendOp>>(value));
		}
#endif

#ifdef PE_GLES
		inline GLuint to_gles(blend_op value) noexcept
		{
			switch(value)
			{
			case blend_op::add: return GL_FUNC_ADD; break;
			case blend_op::subtract: return GL_FUNC_SUBTRACT; break;
			case blend_op::reverse_subtract: return GL_FUNC_REVERSE_SUBTRACT; break;
			case blend_op::min: return GL_MIN; break;
			case blend_op::max: return GL_MAX; break;
			}
			assert(false);
			return GL_FUNC_ADD;
		}
#endif
	} // namespace conversion

	enum class blend_factor
	{
		zero						= 0,
		one							= 1,
		source_color				= 2,
		one_minus_source_color		= 3,
		dst_color					= 4,
		one_minus_destination_color = 5,
		source_alpha				= 6,
		one_minus_source_alpha		= 7,
		dst_alpha					= 8,
		one_minus_destination_alpha = 9,
		constant_color				= 10,
		one_minus_constant_color	= 11,
		constant_alpha				= 12,
		one_minus_constant_alpha	= 13,
		source_alpha_saturate		= 14,
	};

	namespace conversion
	{
#ifdef PE_VULKAN
		inline vk::BlendFactor to_vk(blend_factor value) noexcept
		{
			return vk::BlendFactor{static_cast<std::underlying_type_t<blend_factor>>(value)};
		}

		inline blend_factor to_blend_factor(vk::BlendFactor value) noexcept
		{
			return blend_factor(static_cast<std::underlying_type_t<vk::BlendFactor>>(value));
		}
#endif

#ifdef PE_GLES
		inline GLuint to_gles(blend_factor value) noexcept
		{
			switch(value)
			{
			case blend_factor::zero: return GL_ZERO; break;
			case blend_factor::one: return GL_ONE; break;
			case blend_factor::source_color: return GL_SRC_COLOR; break;
			case blend_factor::one_minus_source_color: return GL_ONE_MINUS_SRC_COLOR; break;
			case blend_factor::dst_color: return GL_DST_COLOR; break;
			case blend_factor::one_minus_destination_color: return GL_ONE_MINUS_DST_COLOR; break;
			case blend_factor::source_alpha: return GL_SRC_ALPHA; break;
			case blend_factor::one_minus_source_alpha: return GL_ONE_MINUS_SRC_ALPHA; break;
			case blend_factor::dst_alpha: return GL_DST_ALPHA; break;
			case blend_factor::one_minus_destination_alpha: return GL_ONE_MINUS_DST_ALPHA; break;
			case blend_factor::constant_color: return GL_CONSTANT_COLOR; break;
			case blend_factor::one_minus_constant_color: return GL_ONE_MINUS_CONSTANT_COLOR; break;
			case blend_factor::constant_alpha: return GL_CONSTANT_ALPHA; break;
			case blend_factor::one_minus_constant_alpha: return GL_ONE_MINUS_CONSTANT_ALPHA; break;
			case blend_factor::source_alpha_saturate: return GL_SRC_ALPHA_SATURATE; break;
			}
			assert(false);
			return GL_FUNC_ADD;
		}
#endif
	} // namespace conversion
} // namespace core::gfx