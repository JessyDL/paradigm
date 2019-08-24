#pragma once
#include "vk/ivk.h"
#include "gfx/types.h"

namespace core::gfx::conversion
{
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
	inline vk::BufferUsageFlags to_vk(memory_usage memory) noexcept
	{
		using vk_type  = std::underlying_type_t<vk::BufferUsageFlagBits>;
		using gfx_type = std::underlying_type_t<memory_usage>;

		vk_type destination{0};
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::transfer_source)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eTransferSrc)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::transfer_destination)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eTransferDst)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::uniform_texel_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eUniformTexelBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::storage_texel_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eStorageTexelBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::uniform_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eUniformBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::storage_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eStorageBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::index_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eIndexBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::vertex_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eVertexBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::indirect_buffer)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eIndirectBuffer)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_usage::conditional_rendering)
						   ? static_cast<vk_type>(vk::BufferUsageFlagBits::eConditionalRenderingEXT)
						   : 0;
		return vk::BufferUsageFlags{vk::BufferUsageFlagBits{destination}};
	}

	inline memory_usage to_memory_usage(vk::BufferUsageFlags flags) noexcept
	{
		using gfx_type = std::underlying_type_t<memory_usage>;

		gfx_type res{0};
		res += (flags & vk::BufferUsageFlagBits::eConditionalRenderingEXT)
				   ? static_cast<gfx_type>(memory_usage::conditional_rendering)
				   : 0;
		res +=
			(flags & vk::BufferUsageFlagBits::eTransferSrc) ? static_cast<gfx_type>(memory_usage::transfer_source) : 0;
		res += (flags & vk::BufferUsageFlagBits::eTransferDst)
				   ? static_cast<gfx_type>(memory_usage::transfer_destination)
				   : 0;

		res += (flags & vk::BufferUsageFlagBits::eUniformTexelBuffer)
				   ? static_cast<gfx_type>(memory_usage::uniform_texel_buffer)
				   : 0;

		res += (flags & vk::BufferUsageFlagBits::eStorageTexelBuffer)
				   ? static_cast<gfx_type>(memory_usage::storage_texel_buffer)
				   : 0;

		res +=
			(flags & vk::BufferUsageFlagBits::eUniformBuffer) ? static_cast<gfx_type>(memory_usage::uniform_buffer) : 0;

		res +=
			(flags & vk::BufferUsageFlagBits::eStorageBuffer) ? static_cast<gfx_type>(memory_usage::storage_buffer) : 0;

		res += (flags & vk::BufferUsageFlagBits::eIndexBuffer) ? static_cast<gfx_type>(memory_usage::index_buffer) : 0;

		res += (flags & vk::BufferUsageFlagBits::eVertexBuffer) ? static_cast<gfx_type>(memory_usage::vertex_buffer) : 0;

		res += (flags & vk::BufferUsageFlagBits::eIndirectBuffer) ? static_cast<gfx_type>(memory_usage::indirect_buffer)
																  : 0;

		return memory_usage{res};
	}

	
	inline vk::MemoryPropertyFlags to_vk(memory_property memory) noexcept
	{
		using vk_type  = std::underlying_type_t<vk::MemoryPropertyFlagBits>;
		using gfx_type = std::underlying_type_t<memory_property>;

		vk_type destination{0};
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_property::device_local)
						   ? static_cast<vk_type>(vk::MemoryPropertyFlagBits::eDeviceLocal)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_property::host_cached)
						   ? static_cast<vk_type>(vk::MemoryPropertyFlagBits::eHostCached)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_property::host_visible)
						   ? static_cast<vk_type>(vk::MemoryPropertyFlagBits::eHostVisible)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_property::lazily_allocated)
						   ? static_cast<vk_type>(vk::MemoryPropertyFlagBits::eLazilyAllocated)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_property::protected_memory)
						   ? static_cast<vk_type>(vk::MemoryPropertyFlagBits::eProtected)
						   : 0;
		destination += static_cast<gfx_type>(memory) & static_cast<gfx_type>(memory_property::host_coherent)
						   ? static_cast<vk_type>(vk::MemoryPropertyFlagBits::eHostCoherent)
						   : 0;
		return vk::MemoryPropertyFlags{vk::MemoryPropertyFlagBits{destination}};
	}

	inline memory_property to_memory_property(vk::MemoryPropertyFlags flags) noexcept
	{
		using gfx_type = std::underlying_type_t<memory_property>;

		gfx_type res{0};
		res += (flags & vk::MemoryPropertyFlagBits::eDeviceLocal) ? static_cast<gfx_type>(memory_property::device_local)
				   : 0;
		res +=
			(flags & vk::MemoryPropertyFlagBits::eHostCached) ? static_cast<gfx_type>(memory_property::host_cached)
																 : 0;
		res += (flags & vk::MemoryPropertyFlagBits::eHostVisible) ? static_cast<gfx_type>(memory_property::host_visible)
				   : 0;

		res += (flags & vk::MemoryPropertyFlagBits::eLazilyAllocated)
				   ? static_cast<gfx_type>(memory_property::lazily_allocated)
				   : 0;

		res += (flags & vk::MemoryPropertyFlagBits::eProtected)
				   ? static_cast<gfx_type>(memory_property::protected_memory)
				   : 0;

		res += (flags & vk::MemoryPropertyFlagBits::eHostCoherent)
				   ? static_cast<gfx_type>(memory_property::host_coherent)
																   : 0;


		return memory_property{res};
	}


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
	inline vk::ImageAspectFlags to_vk(image_aspect value) noexcept
	{
		return vk::ImageAspectFlags{vk::ImageAspectFlagBits{static_cast<std::underlying_type_t<image_aspect>>(value)}};
	}

	inline image_aspect to_image_aspect(vk::ImageAspectFlags value) noexcept
	{
		return image_aspect(static_cast<VkImageAspectFlags>(value));
	}

	inline vk::ImageUsageFlags to_vk(image_usage value) noexcept
	{
		return vk::ImageUsageFlags{vk::ImageUsageFlagBits{static_cast<std::underlying_type_t<image_usage>>(value)}};
	}

	inline image_usage to_image_usage(vk::ImageUsageFlags value) noexcept
	{
		return image_usage(static_cast<VkImageUsageFlags>(value));
	}

	inline vk::ColorComponentFlags to_vk(component_bits value) noexcept
	{
		return vk::ColorComponentFlags{
			vk::ColorComponentFlagBits{static_cast<std::underlying_type_t<component_bits>>(value)}};
	}
	inline component_bits to_component_bits(vk::ColorComponentFlags value) noexcept
	{
		return component_bits(static_cast<VkColorComponentFlags>(value));
	}


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
	inline vk::SamplerMipmapMode to_vk(sampler_mipmap_mode value) noexcept
	{
		return vk::SamplerMipmapMode(static_cast<std::underlying_type_t<sampler_mipmap_mode>>(value));
	}

	inline vk::SamplerAddressMode to_vk(sampler_address_mode value) noexcept
	{
		return vk::SamplerAddressMode(static_cast<std::underlying_type_t<sampler_address_mode>>(value));
	}

	inline vk::BorderColor to_vk(border_color value) noexcept
	{
		return vk::BorderColor(static_cast<std::underlying_type_t<border_color>>(value));
	}

	inline vk::CullModeFlags to_vk(cullmode value) noexcept
	{
		return vk::CullModeFlags(static_cast<std::underlying_type_t<cullmode>>(value));
	}

	inline cullmode to_cullmode(vk::CullModeFlags value) noexcept
	{
		return cullmode(static_cast<VkCullModeFlags>(value));
	}


	inline vk::CompareOp to_vk(compare_op value) noexcept
	{
		return vk::CompareOp(static_cast<std::underlying_type_t<compare_op>>(value));
	}

	inline compare_op to_compare_op(vk::CompareOp value) noexcept
	{
		return compare_op(static_cast<std::underlying_type_t<vk::CompareOp>>(value));
	}

	inline vk::Filter to_vk(filter value) noexcept
	{
		return vk::Filter(static_cast<std::underlying_type_t<filter>>(value));
	}

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
	inline vk::BlendOp to_vk(blend_op value) noexcept
	{
		return vk::BlendOp{static_cast<std::underlying_type_t<blend_op>>(value)};
	}

	inline blend_op to_blend_op(vk::BlendOp value) noexcept
	{
		return blend_op(static_cast<std::underlying_type_t<vk::BlendOp>>(value));
	}

	inline vk::BlendFactor to_vk(blend_factor value) noexcept
	{
		return vk::BlendFactor{static_cast<std::underlying_type_t<blend_factor>>(value)};
	}

	inline blend_factor to_blend_factor(vk::BlendFactor value) noexcept
	{
		return blend_factor(static_cast<std::underlying_type_t<vk::BlendFactor>>(value));
	}
} // namespace core::gfx::conversion