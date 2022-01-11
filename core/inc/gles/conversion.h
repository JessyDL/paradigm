#pragma once
#include "igles.h"
#define GL_ENABLE_ALL_DEFINES
#include "details/definitions.h"
#include "gfx/types.h"

namespace core::gfx::conversion
{
	inline decltype(GL_VERTEX_SHADER) to_gles(shader_stage stage) noexcept
	{
		switch(stage)
		{
		case shader_stage::vertex:
			return GL_VERTEX_SHADER;
			break;
		case shader_stage::fragment:
			return GL_FRAGMENT_SHADER;
			break;
		case shader_stage::compute:
			return GL_COMPUTE_SHADER;
			break;
		case shader_stage::geometry:
			return GL_GEOMETRY_SHADER;
			break;
		case shader_stage::tesselation_control:
			return GL_TESS_CONTROL_SHADER;
			break;
		case shader_stage::tesselation_evaluation:
			return GL_TESS_EVALUATION_SHADER;
			break;
		}
		assert(false);
		return -1;
	}

	inline shader_stage to_shader_stage(GLint value) noexcept
	{
		switch(value)
		{
		case GL_VERTEX_SHADER:
			return shader_stage::vertex;
			break;
		case GL_FRAGMENT_SHADER:
			return shader_stage::fragment;
			break;
		case GL_COMPUTE_SHADER:
			return shader_stage::compute;
			break;
		case GL_GEOMETRY_SHADER:
			return shader_stage::geometry;
			break;
		case GL_TESS_CONTROL_SHADER:
			return shader_stage::tesselation_control;
			break;
		case GL_TESS_EVALUATION_SHADER:
			return shader_stage::tesselation_evaluation;
			break;
		}
		assert(false);
		return shader_stage {};
	}

	inline GLint to_gles(vertex_input_rate value) noexcept
	{
		return static_cast<std::underlying_type_t<vertex_input_rate>>(value);
	}

	inline vertex_input_rate to_vertex_input_rate(GLint value) noexcept { return vertex_input_rate(value); }
	/// \warning gles does not have a concept of transfer_source/transfer_destination, all buffers are "valid" as
	/// either
	inline decltype(GL_ARRAY_BUFFER) to_gles(memory_usage memory) noexcept
	{
		using gfx_type = std::underlying_type_t<memory_usage>;

		if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::uniform_texel_buffer))
		{
			return GL_TEXTURE_BUFFER;
		}
		else if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::conditional_rendering))
		{
			assert(false);
			// not implemented
		}
		else if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::index_buffer))
		{
			return GL_ELEMENT_ARRAY_BUFFER;
		}
		else if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::vertex_buffer))
		{
			return GL_ARRAY_BUFFER;
		}
		else if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::storage_buffer))
		{
			return GL_SHADER_STORAGE_BUFFER;
		}
		else if(static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::uniform_buffer))
		{
			return GL_UNIFORM_BUFFER;
		}

		assert(false);
		return -1;
	}

	inline bool to_gles(format_t value, GLint& internalFormat, GLint& format, GLint& type) noexcept
	{
		internalFormat = -1;
		switch(value)
		{
		case format_t::r4g4b4a4_unorm_pack16:
			internalFormat = GL_RGBA4;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_SHORT_4_4_4_4;
			break;
		case format_t::r5g6b5_unorm_pack16:
			internalFormat = GL_RGB565;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_SHORT_5_6_5;
			break;
		case format_t::r5g5b5a1_unorm_pack16:
			internalFormat = GL_RGB5_A1;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_SHORT_5_5_5_1;
			break;
		case format_t::r8_unorm:
			internalFormat = GL_R8;
			format		   = GL_RED;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8_snorm:
			internalFormat = GL_R8_SNORM;
			format		   = GL_RED;
			type		   = GL_BYTE;
			break;
		case format_t::r8_uint:
			internalFormat = GL_R8UI;
			format		   = GL_RED_INTEGER;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8_sint:
			internalFormat = GL_R8I;
			format		   = GL_RED_INTEGER;
			type		   = GL_BYTE;
			break;
		case format_t::r8_srgb:
			internalFormat = GL_SR8_EXT;
			format		   = GL_RED;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8g8_unorm:
			internalFormat = GL_RG8;
			format		   = GL_RG;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8g8_snorm:
			internalFormat = GL_RG8_SNORM;
			format		   = GL_RG;
			type		   = GL_BYTE;
			break;
		case format_t::r8g8_uint:
			internalFormat = GL_RG8UI;
			format		   = GL_RG_INTEGER;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8g8_sint:
			internalFormat = GL_RG8I;
			format		   = GL_RG_INTEGER;
			type		   = GL_BYTE;
			break;
		case format_t::r8g8_srgb:
			internalFormat = GL_SRG8_EXT;
			format		   = GL_RG;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8g8b8_unorm:
			internalFormat = GL_RGB8;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8g8b8_snorm:
			internalFormat = GL_RGB8_SNORM;
			format		   = GL_RGB;
			type		   = GL_BYTE;
			break;
		case format_t::r8g8b8_uint:
			internalFormat = GL_RGB8UI;
			format		   = GL_RGB_INTEGER;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8g8b8_sint:
			internalFormat = GL_RGB8I;
			format		   = GL_RGB_INTEGER;
			type		   = GL_BYTE;
			break;
		case format_t::r8g8b8_srgb:
			internalFormat = GL_SRGB8;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8g8b8a8_unorm:
			internalFormat = GL_RGBA8;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8g8b8a8_snorm:
			internalFormat = GL_RGBA8_SNORM;
			format		   = GL_RGBA;
			type		   = GL_BYTE;
			break;
		case format_t::r8g8b8a8_uint:
			internalFormat = GL_RGBA8UI;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::r8g8b8a8_sint:
			internalFormat = GL_RGBA8I;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_BYTE;
			break;
		case format_t::r8g8b8a8_srgb:
			internalFormat = GL_SRGB8_ALPHA8;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::a2b10g10r10_unorm_pack32:
			internalFormat = GL_RGB10_A2;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_INT_2_10_10_10_REV;
			break;
		case format_t::a2b10g10r10_uint_pack32:
			internalFormat = GL_RGB10_A2UI;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_UNSIGNED_INT_2_10_10_10_REV;
			break;
		/*case format_t::r16_unorm:
			internalFormat = GL_R16;
			format		   = GL_RED;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format_t::r16_snorm:
			internalFormat = GL_R16_SNORM;
			format		   = GL_RED;
			type		   = GL_SHORT;
			break;*/
		case format_t::r16_uint:
			internalFormat = GL_R16UI;
			format		   = GL_RED_INTEGER;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format_t::r16_sint:
			internalFormat = GL_R16I;
			format		   = GL_RED_INTEGER;
			type		   = GL_SHORT;
			break;
		case format_t::r16_sfloat:
			internalFormat = GL_R16F;
			format		   = GL_RED;
			type		   = GL_HALF_FLOAT;
			break;
		/*case format_t::r16g16_unorm:
			internalFormat = GL_RG16;
			format		   = GL_RG;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format_t::r16g16_snorm:
			internalFormat = GL_RG16_SNORM;
			format		   = GL_RG;
			type		   = GL_SHORT;
			break;*/
		case format_t::r16g16_uint:
			internalFormat = GL_RG16UI;
			format		   = GL_RG_INTEGER;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format_t::r16g16_sint:
			internalFormat = GL_RG16I;
			format		   = GL_RG_INTEGER;
			type		   = GL_SHORT;
			break;
		case format_t::r16g16_sfloat:
			internalFormat = GL_RG16F;
			format		   = GL_RG;
			type		   = GL_HALF_FLOAT;
			break;
		/*case format_t::r16g16b16_unorm:
			internalFormat = GL_RGB16;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format_t::r16g16b16_snorm:
			internalFormat = GL_RGB16_SNORM;
			format		   = GL_RGB;
			type		   = GL_SHORT;
			break;*/
		case format_t::r16g16b16_uint:
			internalFormat = GL_RGB16UI;
			format		   = GL_RGB_INTEGER;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format_t::r16g16b16_sint:
			internalFormat = GL_RGB16I;
			format		   = GL_RGB_INTEGER;
			type		   = GL_SHORT;
			break;
		case format_t::r16g16b16_sfloat:
			internalFormat = GL_RGB16F;
			format		   = GL_RGB;
			type		   = GL_HALF_FLOAT;
			break;
		/*case format_t::r16g16b16a16_unorm:
			internalFormat = GL_RGBA16;
			format		   = GL_RGBA;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format_t::r16g16b16a16_snorm:
			internalFormat = GL_RGBA16_SNORM;
			format		   = GL_RGBA;
			type		   = GL_SHORT;
			break;*/
		case format_t::r16g16b16a16_uint:
			internalFormat = GL_RGBA16UI;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format_t::r16g16b16a16_sint:
			internalFormat = GL_RGBA16I;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_SHORT;
			break;
		case format_t::r16g16b16a16_sfloat:
			internalFormat = GL_RGBA16F;
			format		   = GL_RGBA;
			type		   = GL_HALF_FLOAT;
			break;
		case format_t::r32_uint:
			internalFormat = GL_R32UI;
			format		   = GL_RED_INTEGER;
			type		   = GL_UNSIGNED_INT;
			break;
		case format_t::r32_sint:
			internalFormat = GL_R32I;
			format		   = GL_RED_INTEGER;
			type		   = GL_INT;
			break;
		case format_t::r32_sfloat:
			internalFormat = GL_R32F;
			format		   = GL_RED;
			type		   = GL_FLOAT;
			break;
		case format_t::r32g32_uint:
			internalFormat = GL_RG32UI;
			format		   = GL_RG_INTEGER;
			type		   = GL_UNSIGNED_INT;
			break;
		case format_t::r32g32_sint:
			internalFormat = GL_RG32I;
			format		   = GL_RG_INTEGER;
			type		   = GL_INT;
			break;
		case format_t::r32g32_sfloat:
			internalFormat = GL_RG32F;
			format		   = GL_RG;
			type		   = GL_FLOAT;
			break;
		case format_t::r32g32b32_uint:
			internalFormat = GL_RGB32UI;
			format		   = GL_RGB_INTEGER;
			type		   = GL_UNSIGNED_INT;
			break;
		case format_t::r32g32b32_sint:
			internalFormat = GL_RGB32I;
			format		   = GL_RGB_INTEGER;
			type		   = GL_INT;
			break;
		case format_t::r32g32b32_sfloat:
			internalFormat = GL_RGB32F;
			format		   = GL_RGB;
			type		   = GL_FLOAT;
			break;
		case format_t::r32g32b32a32_uint:
			internalFormat = GL_RGBA32UI;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_UNSIGNED_INT;
			break;
		case format_t::r32g32b32a32_sint:
			internalFormat = GL_RGBA32I;
			format		   = GL_RGBA_INTEGER;
			type		   = GL_INT;
			break;
		case format_t::r32g32b32a32_sfloat:
			internalFormat = GL_RGBA32F;
			format		   = GL_RGBA;
			type		   = GL_FLOAT;
			break;
		case format_t::b10g11r11_ufloat_pack32:
			internalFormat = GL_R11F_G11F_B10F;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_INT_10F_11F_11F_REV;
			break;
		case format_t::e5b9g9r9_ufloat_pack32:
			internalFormat = GL_RGB9_E5;
			format		   = GL_RGB;
			type		   = GL_UNSIGNED_INT_5_9_9_9_REV;
			break;
		case format_t::d16_unorm:
			internalFormat = GL_DEPTH_COMPONENT16;
			format		   = GL_DEPTH_COMPONENT;
			type		   = GL_UNSIGNED_SHORT;
			break;
		case format_t::x8_d24_unorm_pack32:
			internalFormat = GL_DEPTH_COMPONENT24;
			format		   = GL_DEPTH_COMPONENT;
			type		   = GL_UNSIGNED_INT;
			break;
		case format_t::d32_sfloat:
			internalFormat = GL_DEPTH_COMPONENT32F;
			format		   = GL_DEPTH_COMPONENT;
			type		   = GL_FLOAT;
			break;
		case format_t::s8_uint:
			internalFormat = GL_STENCIL_INDEX8;
			format		   = GL_STENCIL_INDEX;
			type		   = GL_UNSIGNED_BYTE;
			break;
		case format_t::d24_unorm_s8_uint:
			internalFormat = GL_DEPTH24_STENCIL8;
			format		   = GL_DEPTH_STENCIL;
			type		   = GL_UNSIGNED_INT_24_8;
			break;
		case format_t::d32_sfloat_s8_uint:
			internalFormat = GL_DEPTH32F_STENCIL8;
			format		   = GL_DEPTH_STENCIL;
			type		   = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
			break;
		case format_t::bc1_rgb_unorm_block:
			internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc1_rgb_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc1_rgba_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc1_rgba_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc2_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc2_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc3_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc3_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc4_unorm_block:
			internalFormat = GL_COMPRESSED_RED_RGTC1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc4_snorm_block:
			internalFormat = GL_COMPRESSED_SIGNED_RED_RGTC1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc5_unorm_block:
			internalFormat = GL_COMPRESSED_RED_GREEN_RGTC2_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc5_snorm_block:
			internalFormat = GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT;
			format		   = 0;
			type		   = 0;
			break;
		/*case format_t::bc6h_ufloat_block:
			internalFormat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc6h_sfloat_block:
			internalFormat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc7_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::bc7_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
			format		   = 0;
			type		   = 0;
			break;*/
		case format_t::etc2_r8g8b8_unorm_block:
			internalFormat = GL_COMPRESSED_RGB8_ETC2;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::etc2_r8g8b8_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ETC2;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::etc2_r8g8b8a1_unorm_block:
			internalFormat = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::etc2_r8g8b8a1_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::etc2_r8g8b8a8_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::etc2_r8g8b8a8_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::eac_r11_unorm_block:
			internalFormat = GL_COMPRESSED_R11_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::eac_r11_snorm_block:
			internalFormat = GL_COMPRESSED_SIGNED_R11_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::eac_r11g11_unorm_block:
			internalFormat = GL_COMPRESSED_RG11_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::eac_r11g11_snorm_block:
			internalFormat = GL_COMPRESSED_SIGNED_RG11_EAC;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_4x4_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_4x4_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_5x4_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_5x4_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_5x5_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_5x5_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_6x5_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_6x5_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_6x6_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_6x6_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_8x5_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_8x5_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_8x6_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_8x6_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_8x8_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_8x8_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_10x5_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_10x5_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_10x6_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_10x6_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_10x8_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_10x8_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_10x10_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_10x10_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_12x10_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_12x10_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_12x12_unorm_block:
			internalFormat = GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::astc_12x12_srgb_block:
			internalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::pvrtc1_2bpp_unorm_block_img:
			internalFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::pvrtc1_4bpp_unorm_block_img:
			internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::pvrtc2_2bpp_unorm_block_img:
			internalFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::pvrtc2_4bpp_unorm_block_img:
			internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::pvrtc1_2bpp_srgb_block_img:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::pvrtc1_4bpp_srgb_block_img:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::pvrtc2_2bpp_srgb_block_img:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG;
			format		   = 0;
			type		   = 0;
			break;
		case format_t::pvrtc2_4bpp_srgb_block_img:
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG;
			format		   = 0;
			type		   = 0;
			break;
		}
		return internalFormat != -1;
	}

	inline core::gfx::format_t to_format(GLint internalFormat, GLint format, GLint type) noexcept
	{
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
		{
			switch(format)
			{
			case GL_RED:
				return format_t::r8_unorm;
			case GL_RG:
				return core::gfx::format_t::r8g8_unorm;
			case GL_RGB:
				return core::gfx::format_t::r8g8b8_unorm;
			// case GL_BGR: return core::gfx::format_t::b8g8r8_unorm;
			case GL_RGBA:
				return core::gfx::format_t::r8g8b8a8_unorm;
			// case GL_BGRA: return core::gfx::format_t::b8g8r8a8_unorm;
			case GL_RED_INTEGER:
				return core::gfx::format_t::r8_uint;
			case GL_RG_INTEGER:
				return core::gfx::format_t::r8g8_uint;
			case GL_RGB_INTEGER:
				return core::gfx::format_t::r8g8b8_uint;
			// case GL_BGR_INTEGER: return core::gfx::format_t::b8g8r8_uint;
			case GL_RGBA_INTEGER:
				return core::gfx::format_t::r8g8b8a8_uint;
			// case GL_BGRA_INTEGER: return core::gfx::format_t::b8g8r8a8_uint;
			case GL_STENCIL_INDEX:
				return core::gfx::format_t::s8_uint;
			case GL_DEPTH_COMPONENT:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_STENCIL:
				return core::gfx::format_t::undefined;
			}
			break;
		}
		case GL_BYTE:
		{
			switch(format)
			{
			case GL_RED:
				return core::gfx::format_t::r8_snorm;
			case GL_RG:
				return core::gfx::format_t::r8g8_snorm;
			case GL_RGB:
				return core::gfx::format_t::r8g8b8_snorm;
			// case GL_BGR: return core::gfx::format_t::b8g8r8_snorm;
			case GL_RGBA:
				return core::gfx::format_t::r8g8b8a8_snorm;
			// case GL_BGRA: return core::gfx::format_t::b8g8r8a8_snorm;
			case GL_RED_INTEGER:
				return core::gfx::format_t::r8_sint;
			case GL_RG_INTEGER:
				return core::gfx::format_t::r8g8_sint;
			case GL_RGB_INTEGER:
				return core::gfx::format_t::r8g8b8_sint;
			// case GL_BGR_INTEGER: return core::gfx::format_t::b8g8r8_sint;
			case GL_RGBA_INTEGER:
				return core::gfx::format_t::r8g8b8a8_sint;
			// case GL_BGRA_INTEGER: return core::gfx::format_t::b8g8r8a8_sint;
			case GL_STENCIL_INDEX:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_COMPONENT:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_STENCIL:
				return core::gfx::format_t::undefined;
			}
			break;
		}

		//
		// 16 bits per component
		//
		case GL_UNSIGNED_SHORT:
		{
			switch(format)
			{
			case GL_RED:
				return core::gfx::format_t::r16_unorm;
			case GL_RG:
				return core::gfx::format_t::r16g16_unorm;
			case GL_RGB:
				return core::gfx::format_t::r16g16b16_unorm;
			// case GL_BGR: return core::gfx::format_t::undefined;
			case GL_RGBA:
				return core::gfx::format_t::r16g16b16a16_unorm;
			// case GL_BGRA: return core::gfx::format_t::undefined;
			case GL_RED_INTEGER:
				return core::gfx::format_t::r16_uint;
			case GL_RG_INTEGER:
				return core::gfx::format_t::r16g16_uint;
			case GL_RGB_INTEGER:
				return core::gfx::format_t::r16g16b16_uint;
			// case GL_BGR_INTEGER: return core::gfx::format_t::undefined;
			case GL_RGBA_INTEGER:
				return core::gfx::format_t::r16g16b16a16_uint;
			// case GL_BGRA_INTEGER: return core::gfx::format_t::undefined;
			case GL_STENCIL_INDEX:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_COMPONENT:
				return core::gfx::format_t::d16_unorm;
				// case GL_DEPTH_STENCIL: return core::gfx::format_t::d16_unorm_s8_uint;
			}
			break;
		}
		case GL_SHORT:
		{
			switch(format)
			{
			case GL_RED:
				return core::gfx::format_t::r16_snorm;
			case GL_RG:
				return core::gfx::format_t::r16g16_snorm;
			case GL_RGB:
				return core::gfx::format_t::r16g16b16_snorm;
			// case GL_BGR: return core::gfx::format_t::undefined;
			case GL_RGBA:
				return core::gfx::format_t::r16g16b16a16_snorm;
			// case GL_BGRA: return core::gfx::format_t::undefined;
			case GL_RED_INTEGER:
				return core::gfx::format_t::r16_sint;
			case GL_RG_INTEGER:
				return core::gfx::format_t::r16g16_sint;
			case GL_RGB_INTEGER:
				return core::gfx::format_t::r16g16b16_sint;
			// case GL_BGR_INTEGER: return core::gfx::format_t::undefined;
			case GL_RGBA_INTEGER:
				return core::gfx::format_t::r16g16b16a16_sint;
			// case GL_BGRA_INTEGER: return core::gfx::format_t::undefined;
			case GL_STENCIL_INDEX:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_COMPONENT:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_STENCIL:
				return core::gfx::format_t::undefined;
			}
			break;
		}
		case GL_HALF_FLOAT:
			// case GL_HALF_FLOAT_OES:
			{
				switch(format)
				{
				case GL_RED:
					return core::gfx::format_t::r16_sfloat;
				case GL_RG:
					return core::gfx::format_t::r16g16_sfloat;
				case GL_RGB:
					return core::gfx::format_t::r16g16b16_sfloat;
				// case GL_BGR: return core::gfx::format_t::undefined;
				case GL_RGBA:
					return core::gfx::format_t::r16g16b16a16_sfloat;
				// case GL_BGRA: return core::gfx::format_t::undefined;
				case GL_RED_INTEGER:
					return core::gfx::format_t::undefined;
				case GL_RG_INTEGER:
					return core::gfx::format_t::undefined;
				case GL_RGB_INTEGER:
					return core::gfx::format_t::undefined;
				// case GL_BGR_INTEGER: return core::gfx::format_t::undefined;
				case GL_RGBA_INTEGER:
					return core::gfx::format_t::undefined;
				// case GL_BGRA_INTEGER: return core::gfx::format_t::undefined;
				case GL_STENCIL_INDEX:
					return core::gfx::format_t::undefined;
				case GL_DEPTH_COMPONENT:
					return core::gfx::format_t::undefined;
				case GL_DEPTH_STENCIL:
					return core::gfx::format_t::undefined;
				}
				break;
			}

		//
		// 32 bits per component
		//
		case GL_UNSIGNED_INT:
		{
			switch(format)
			{
			case GL_RED:
				return core::gfx::format_t::r32_uint;
			case GL_RG:
				return core::gfx::format_t::r32g32_uint;
			case GL_RGB:
				return core::gfx::format_t::r32g32b32_uint;
			// case GL_BGR: return core::gfx::format_t::undefined;
			case GL_RGBA:
				return core::gfx::format_t::r32g32b32a32_uint;
			// case GL_BGRA: return core::gfx::format_t::undefined;
			case GL_RED_INTEGER:
				return core::gfx::format_t::r32_uint;
			case GL_RG_INTEGER:
				return core::gfx::format_t::r32g32_uint;
			case GL_RGB_INTEGER:
				return core::gfx::format_t::r32g32b32_uint;
			// case GL_BGR_INTEGER: return core::gfx::format_t::undefined;
			case GL_RGBA_INTEGER:
				return core::gfx::format_t::r32g32b32a32_uint;
			// case GL_BGRA_INTEGER: return core::gfx::format_t::undefined;
			case GL_STENCIL_INDEX:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_COMPONENT:
				return core::gfx::format_t::x8_d24_unorm_pack32;
			case GL_DEPTH_STENCIL:
				return core::gfx::format_t::d24_unorm_s8_uint;
			}
			break;
		}
		case GL_INT:
		{
			switch(format)
			{
			case GL_RED:
				return core::gfx::format_t::r32_sint;
			case GL_RG:
				return core::gfx::format_t::r32g32_sint;
			case GL_RGB:
				return core::gfx::format_t::r32g32b32_sint;
			// case GL_BGR: return core::gfx::format_t::undefined;
			case GL_RGBA:
				return core::gfx::format_t::r32g32b32a32_sint;
			// case GL_BGRA: return core::gfx::format_t::undefined;
			case GL_RED_INTEGER:
				return core::gfx::format_t::r32_sint;
			case GL_RG_INTEGER:
				return core::gfx::format_t::r32g32_sint;
			case GL_RGB_INTEGER:
				return core::gfx::format_t::r32g32b32_sint;
			// case GL_BGR_INTEGER: return core::gfx::format_t::undefined;
			case GL_RGBA_INTEGER:
				return core::gfx::format_t::r32g32b32a32_sint;
			// case GL_BGRA_INTEGER: return core::gfx::format_t::undefined;
			case GL_STENCIL_INDEX:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_COMPONENT:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_STENCIL:
				return core::gfx::format_t::undefined;
			}
			break;
		}
		case GL_FLOAT:
		{
			switch(format)
			{
			case GL_RED:
				return core::gfx::format_t::r32_sfloat;
			case GL_RG:
				return core::gfx::format_t::r32g32_sfloat;
			case GL_RGB:
				return core::gfx::format_t::r32g32b32_sfloat;
			// case GL_BGR: return core::gfx::format_t::undefined;
			case GL_RGBA:
				return core::gfx::format_t::r32g32b32a32_sfloat;
			// case GL_BGRA: return core::gfx::format_t::undefined;
			case GL_RED_INTEGER:
				return core::gfx::format_t::undefined;
			case GL_RG_INTEGER:
				return core::gfx::format_t::undefined;
			case GL_RGB_INTEGER:
				return core::gfx::format_t::undefined;
			// case GL_BGR_INTEGER: return core::gfx::format_t::undefined;
			case GL_RGBA_INTEGER:
				return core::gfx::format_t::undefined;
			// case GL_BGRA_INTEGER: return core::gfx::format_t::undefined;
			case GL_STENCIL_INDEX:
				return core::gfx::format_t::undefined;
			case GL_DEPTH_COMPONENT:
				return core::gfx::format_t::d32_sfloat;
			case GL_DEPTH_STENCIL:
				return core::gfx::format_t::d32_sfloat_s8_uint;
			}
			break;
		}

		// case GL_UNSIGNED_BYTE_3_3_2: assert(format == GL_RGB || format == GL_RGB_INTEGER); return
		// core::gfx::format_t::undefined; case GL_UNSIGNED_BYTE_2_3_3_REV: 	assert(format == GL_BGR || format ==
		// GL_BGR_INTEGER);
		//	return core::gfx::format_t::undefined;
		case GL_UNSIGNED_SHORT_5_6_5:
			assert(format == GL_RGB || format == GL_RGB_INTEGER);
			return core::gfx::format_t::r5g6b5_unorm_pack16;
		// case GL_UNSIGNED_SHORT_5_6_5_REV:
		//	assert(format == GL_BGR || format == GL_BGR_INTEGER);
		//	return core::gfx::format_t::b5g6r5_unorm_pack16;
		case GL_UNSIGNED_SHORT_4_4_4_4:
			assert(format == GL_RGB /*|| format == GL_BGRA*/ ||
				   format == GL_RGB_INTEGER /*|| format == GL_BGRA_INTEGER*/);
			return core::gfx::format_t::r4g4b4a4_unorm_pack16;
		// case GL_UNSIGNED_SHORT_4_4_4_4_REV:
		//	assert(format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER);
		//	return core::gfx::format_t::b4g4r4a4_unorm_pack16;
		case GL_UNSIGNED_SHORT_5_5_5_1:
			assert(format == GL_RGB /*|| format == GL_BGRA*/ ||
				   format == GL_RGB_INTEGER /*|| format == GL_BGRA_INTEGER*/);
			return core::gfx::format_t::r5g5b5a1_unorm_pack16;
		// case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		//	assert(format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER);
		//	return core::gfx::format_t::a1r5g5b5_unorm_pack16;
		// case GL_UNSIGNED_INT_8_8_8_8:
		//	assert(format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER);
		//	return (format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER) ? core::gfx::format_t::r8g8b8a8_uint
		//																   : core::gfx::format_t::r8g8b8a8_unorm;
		// case GL_UNSIGNED_INT_8_8_8_8_REV:
		//	assert(format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER);
		//	return (format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER) ? core::gfx::format_t::a8b8g8r8_uint_pack32
		//																   : core::gfx::format_t::a8b8g8r8_unorm_pack32;
		// case GL_UNSIGNED_INT_10_10_10_2:
		//	assert(format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER);
		//	return (format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER)
		//			   ? core::gfx::format_t::a2r10g10b10_uint_pack32
		//			   : core::gfx::format_t::a2r10g10b10_unorm_pack32;
		case GL_UNSIGNED_INT_2_10_10_10_REV:
			assert(format == GL_RGB /*|| format == GL_BGRA*/ ||
				   format == GL_RGB_INTEGER /*|| format == GL_BGRA_INTEGER*/);
			return (format == GL_RGB_INTEGER /*|| format == GL_BGRA_INTEGER*/)
					 ? core::gfx::format_t::a2b10g10r10_uint_pack32
					 : core::gfx::format_t::a2b10g10r10_unorm_pack32;
		case GL_UNSIGNED_INT_10F_11F_11F_REV:
			assert(format == GL_RGB /*|| format == GL_BGR*/);
			return core::gfx::format_t::b10g11r11_ufloat_pack32;
		case GL_UNSIGNED_INT_5_9_9_9_REV:
			assert(format == GL_RGB /*|| format == GL_BGR*/);
			return core::gfx::format_t::e5b9g9r9_ufloat_pack32;
		case GL_UNSIGNED_INT_24_8:
			assert(format == GL_DEPTH_STENCIL);
			return core::gfx::format_t::d24_unorm_s8_uint;
		case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
			assert(format == GL_DEPTH_STENCIL);
			return core::gfx::format_t::d32_sfloat_s8_uint;
		}
		switch(internalFormat)
		{
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			return format_t::bc1_rgb_unorm_block;
			break;
		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
			return format_t::bc1_rgb_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			return format_t::bc1_rgba_unorm_block;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
			return format_t::bc1_rgba_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			return format_t::bc2_unorm_block;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
			return format_t::bc2_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			return format_t::bc3_unorm_block;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
			return format_t::bc3_srgb_block;
			break;
		case GL_COMPRESSED_RED_RGTC1_EXT:
			return format_t::bc4_unorm_block;
			break;
		case GL_COMPRESSED_SIGNED_RED_RGTC1_EXT:
			return format_t::bc4_snorm_block;
			break;
		// case GL_COMPRESSED_RG_RGTC2_EXT: return format_t::bc5_unorm_block; break;
		// case GL_COMPRESSED_SIGNED_RG_RGTC2_EXT: return format_t::bc5_snorm_block; break;
		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_EXT:
			return format_t::bc6h_ufloat_block;
			break;
		case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_EXT:
			return format_t::bc6h_sfloat_block;
			break;
		case GL_COMPRESSED_RGBA_BPTC_UNORM_EXT:
			return format_t::bc7_unorm_block;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_EXT:
			return format_t::bc7_srgb_block;
			break;
		case GL_COMPRESSED_RGB8_ETC2:
			return format_t::etc2_r8g8b8_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ETC2:
			return format_t::etc2_r8g8b8_srgb_block;
			break;
		case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
			format_t::etc2_r8g8b8a1_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
			format_t::etc2_r8g8b8a1_srgb_block;
			break;
		case GL_COMPRESSED_RGBA8_ETC2_EAC:
			format_t::etc2_r8g8b8a8_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
			return format_t::etc2_r8g8b8a8_srgb_block;
			break;
		case GL_COMPRESSED_R11_EAC:
			return format_t::eac_r11_unorm_block;
			break;
		case GL_COMPRESSED_SIGNED_R11_EAC:
			return format_t::eac_r11_snorm_block;
			break;
		case GL_COMPRESSED_RG11_EAC:
			return format_t::eac_r11g11_unorm_block;
			break;
		case GL_COMPRESSED_SIGNED_RG11_EAC:
			return format_t::eac_r11g11_snorm_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_4x4:
			return format_t::astc_4x4_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4:
			return format_t::astc_4x4_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_5x4:
			return format_t::astc_5x4_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4:
			return format_t::astc_5x4_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_5x5:
			return format_t::astc_5x5_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5:
			return format_t::astc_5x5_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x5:
			return format_t::astc_6x5_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5:
			return format_t::astc_6x5_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x6:
			return format_t::astc_6x6_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6:
			return format_t::astc_6x6_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x5:
			return format_t::astc_8x5_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5:
			return format_t::astc_8x5_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x6:
			return format_t::astc_8x6_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6:
			return format_t::astc_8x6_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x8:
			return format_t::astc_8x8_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8:
			return format_t::astc_8x8_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x5:
			return format_t::astc_10x5_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5:
			return format_t::astc_10x5_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x6:
			return format_t::astc_10x6_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6:
			return format_t::astc_10x6_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x8:
			return format_t::astc_10x8_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8:
			return format_t::astc_10x8_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x10:
			return format_t::astc_10x10_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10:
			return format_t::astc_10x10_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_12x10:
			return format_t::astc_12x10_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10:
			return format_t::astc_12x10_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_ASTC_12x12:
			return format_t::astc_12x12_unorm_block;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12:
			return format_t::astc_12x12_srgb_block;
			break;
		case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
			return format_t::pvrtc1_2bpp_unorm_block_img;
			break;
		case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
			return format_t::pvrtc1_4bpp_unorm_block_img;
			break;
		case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG:
			return format_t::pvrtc2_2bpp_unorm_block_img;
			break;
		case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG:
			return format_t::pvrtc2_4bpp_unorm_block_img;
		case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT:
			return format_t::pvrtc1_2bpp_srgb_block_img;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT:
			return format_t::pvrtc1_4bpp_srgb_block_img;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG:
			return format_t::pvrtc2_2bpp_srgb_block_img;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG:
			return format_t::pvrtc2_4bpp_srgb_block_img;
			break;
		}


		return format_t::undefined;
	}

	inline GLuint to_gles(sampler_address_mode value) noexcept
	{
		switch(value)
		{
		case sampler_address_mode::repeat:
			return GL_REPEAT;
			break;
		case sampler_address_mode::mirrored_repeat:
			return GL_MIRRORED_REPEAT;
			break;
		case sampler_address_mode::clamp_to_edge:
			return GL_CLAMP_TO_EDGE;
			break;
		case sampler_address_mode::clamp_to_border:
			return GL_CLAMP_TO_BORDER;
			break;
#ifdef GL_EXT_texture_mirror_clamp
		case sampler_address_mode::mirror_clamp_to_edge:
			return GL_MIRROR_CLAMP_TO_EDGE_EXT;
			break;
#else
		case sampler_address_mode::mirror_clamp_to_edge:
			return GL_REPEAT;
			break;
#endif
		}

		assert(false);
		return GL_REPEAT;
	}

	inline void to_gles(border_color value, GLint sampler) noexcept
	{
		float fvalues[] {0.0f, 0.0f, 0.0f, 0.0f};
		int ivalues[] {0, 0, 0, 0};
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

	inline GLuint to_gles(cullmode value) noexcept
	{
		switch(value)
		{
		case cullmode::none:
			return GL_NONE;
			break;
		case cullmode::front:
			return GL_FRONT;
			break;
		case cullmode::back:
			return GL_BACK;
			break;
		case cullmode::all:
			return GL_FRONT_AND_BACK;
			break;
		}
		assert(false);
		return GL_BACK;
	}

	inline GLuint to_gles(compare_op value) noexcept
	{
		switch(value)
		{
		case compare_op::never:
			return GL_NEVER;
			break;
		case compare_op::less:
			return GL_LESS;
			break;
		case compare_op::equal:
			return GL_EQUAL;
			break;
		case compare_op::less_equal:
			return GL_LEQUAL;
			break;
		case compare_op::greater:
			return GL_GREATER;
			break;
		case compare_op::not_equal:
			return GL_NOTEQUAL;
			break;
		case compare_op::greater_equal:
			return GL_GEQUAL;
			break;
		case compare_op::always:
			return GL_ALWAYS;
			break;
		}
		assert(false);
		return GL_NEVER;
	}

	inline GLuint to_gles(filter value, sampler_mipmap_mode mode) noexcept
	{
		switch(value)
		{
		case filter::nearest:
			switch(mode)
			{
			case sampler_mipmap_mode::linear:
				return GL_NEAREST_MIPMAP_LINEAR;
			case sampler_mipmap_mode::nearest:
				return GL_NEAREST_MIPMAP_NEAREST;
			}
		case filter::linear:
			switch(mode)
			{
			case sampler_mipmap_mode::linear:
				return GL_LINEAR_MIPMAP_LINEAR;
			case sampler_mipmap_mode::nearest:
				return GL_LINEAR_MIPMAP_NEAREST;
			}
#ifdef GL_IMG_texture_filter_cubic
		case filter::cubic:
			return CUBIC_IMG;
			break;
#else
		case filter::cubic:
			return GL_LINEAR;
			break;
#endif
		}
		assert(false);
		return GL_LINEAR;
	}

	inline GLuint to_gles(blend_op value) noexcept
	{
		switch(value)
		{
		case blend_op::add:
			return GL_FUNC_ADD;
			break;
		case blend_op::subtract:
			return GL_FUNC_SUBTRACT;
			break;
		case blend_op::reverse_subtract:
			return GL_FUNC_REVERSE_SUBTRACT;
			break;
		case blend_op::min:
			return GL_MIN;
			break;
		case blend_op::max:
			return GL_MAX;
			break;
		}
		assert(false);
		return GL_FUNC_ADD;
	}

	inline GLuint to_gles(blend_factor value) noexcept
	{
		switch(value)
		{
		case blend_factor::zero:
			return GL_ZERO;
			break;
		case blend_factor::one:
			return GL_ONE;
			break;
		case blend_factor::source_color:
			return GL_SRC_COLOR;
			break;
		case blend_factor::one_minus_source_color:
			return GL_ONE_MINUS_SRC_COLOR;
			break;
		case blend_factor::dst_color:
			return GL_DST_COLOR;
			break;
		case blend_factor::one_minus_destination_color:
			return GL_ONE_MINUS_DST_COLOR;
			break;
		case blend_factor::source_alpha:
			return GL_SRC_ALPHA;
			break;
		case blend_factor::one_minus_source_alpha:
			return GL_ONE_MINUS_SRC_ALPHA;
			break;
		case blend_factor::dst_alpha:
			return GL_DST_ALPHA;
			break;
		case blend_factor::one_minus_destination_alpha:
			return GL_ONE_MINUS_DST_ALPHA;
			break;
		case blend_factor::constant_color:
			return GL_CONSTANT_COLOR;
			break;
		case blend_factor::one_minus_constant_color:
			return GL_ONE_MINUS_CONSTANT_COLOR;
			break;
		case blend_factor::constant_alpha:
			return GL_CONSTANT_ALPHA;
			break;
		case blend_factor::one_minus_constant_alpha:
			return GL_ONE_MINUS_CONSTANT_ALPHA;
			break;
		case blend_factor::source_alpha_saturate:
			return GL_SRC_ALPHA_SATURATE;
			break;
		}
		assert(false);
		return GL_FUNC_ADD;
	}
}	 // namespace core::gfx::conversion