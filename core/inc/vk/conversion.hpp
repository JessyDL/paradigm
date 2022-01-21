#pragma once
#include "gfx/types.hpp"
#include "vk/ivk.hpp"

namespace core::gfx::conversion
{
	inline vk::ShaderStageFlags to_vk(shader_stage stage) noexcept
	{
		using vk_type  = std::underlying_type_t<vk::ShaderStageFlagBits>;
		using gfx_type = std::underlying_type_t<shader_stage>;

		vk_type destination {0};
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
		return vk::ShaderStageFlags {vk::ShaderStageFlagBits {destination}};
	}

	inline shader_stage to_shader_stage(vk::ShaderStageFlags flags)
	{
		using gfx_type = std::underlying_type_t<shader_stage>;

		gfx_type res {0};
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

		return shader_stage {res};
	}
	inline vk::VertexInputRate to_vk(vertex_input_rate value) noexcept
	{
		switch(value)
		{
		case vertex_input_rate::vertex:
			return vk::VertexInputRate::eVertex;
			break;
		case vertex_input_rate::instance:
			return vk::VertexInputRate::eInstance;
			break;
		}
		assert(false);
		return vk::VertexInputRate::eVertex;
	}

	inline vertex_input_rate to_vertex_input_rate(vk::VertexInputRate value) noexcept
	{
		switch(value)
		{
		case vk::VertexInputRate::eVertex:
			return vertex_input_rate::vertex;
			break;
		case vk::VertexInputRate::eInstance:
			return vertex_input_rate::instance;
			break;
		}
		assert(false);
		return vertex_input_rate::vertex;
	}
	inline vk::BufferUsageFlags to_vk(memory_usage memory) noexcept
	{
		using vk_type  = std::underlying_type_t<vk::BufferUsageFlagBits>;
		using gfx_type = std::underlying_type_t<memory_usage>;

		vk_type destination {0};
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
		return vk::BufferUsageFlags {vk::BufferUsageFlagBits {destination}};
	}

	inline memory_usage to_memory_usage(vk::BufferUsageFlags flags) noexcept
	{
		using gfx_type = std::underlying_type_t<memory_usage>;

		gfx_type res {0};
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

		res +=
		  (flags & vk::BufferUsageFlagBits::eVertexBuffer) ? static_cast<gfx_type>(memory_usage::vertex_buffer) : 0;

		res +=
		  (flags & vk::BufferUsageFlagBits::eIndirectBuffer) ? static_cast<gfx_type>(memory_usage::indirect_buffer) : 0;

		return memory_usage {res};
	}


	inline vk::MemoryPropertyFlags to_vk(memory_property memory) noexcept
	{
		using vk_type  = std::underlying_type_t<vk::MemoryPropertyFlagBits>;
		using gfx_type = std::underlying_type_t<memory_property>;

		vk_type destination {0};
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
		return vk::MemoryPropertyFlags {vk::MemoryPropertyFlagBits {destination}};
	}

	inline memory_property to_memory_property(vk::MemoryPropertyFlags flags) noexcept
	{
		using gfx_type = std::underlying_type_t<memory_property>;

		gfx_type res {0};
		res +=
		  (flags & vk::MemoryPropertyFlagBits::eDeviceLocal) ? static_cast<gfx_type>(memory_property::device_local) : 0;
		res +=
		  (flags & vk::MemoryPropertyFlagBits::eHostCached) ? static_cast<gfx_type>(memory_property::host_cached) : 0;
		res +=
		  (flags & vk::MemoryPropertyFlagBits::eHostVisible) ? static_cast<gfx_type>(memory_property::host_visible) : 0;

		res += (flags & vk::MemoryPropertyFlagBits::eLazilyAllocated)
				 ? static_cast<gfx_type>(memory_property::lazily_allocated)
				 : 0;

		res += (flags & vk::MemoryPropertyFlagBits::eProtected)
				 ? static_cast<gfx_type>(memory_property::protected_memory)
				 : 0;

		res += (flags & vk::MemoryPropertyFlagBits::eHostCoherent)
				 ? static_cast<gfx_type>(memory_property::host_coherent)
				 : 0;


		return memory_property {res};
	}


	inline vk::ImageViewType to_vk(image_type value) noexcept
	{
		switch(value)
		{
		case image_type::planar_1D:
			return vk::ImageViewType::e1D;
			break;
		case image_type::planar_2D:
			return vk::ImageViewType::e2D;
			break;
		case image_type::planar_3D:
			return vk::ImageViewType::e3D;
			break;
		case image_type::cube_2D:
			return vk::ImageViewType::eCube;
			break;
		case image_type::array_1D:
			return vk::ImageViewType::e1DArray;
			break;
		case image_type::array_2D:
			return vk::ImageViewType::e2DArray;
			break;
		case image_type::array_cube:
			return vk::ImageViewType::eCubeArray;
			break;
		}

		assert(false);
		return vk::ImageViewType::e1D;
	}

	inline image_type to_image_type(vk::ImageViewType value) noexcept
	{
		switch(value)
		{
		case vk::ImageViewType::e1D:
			return image_type::planar_1D;
			break;
		case vk::ImageViewType::e2D:
			return image_type::planar_2D;
			break;
		case vk::ImageViewType::e3D:
			return image_type::planar_3D;
			break;
		case vk::ImageViewType::eCube:
			return image_type::cube_2D;
			break;
		case vk::ImageViewType::e1DArray:
			return image_type::array_1D;
			break;
		case vk::ImageViewType::e2DArray:
			return image_type::array_2D;
			break;
		case vk::ImageViewType::eCubeArray:
			return image_type::array_cube;
			break;
		}

		assert(false);
		return image_type::planar_1D;
	}

	inline vk::ImageType to_type(image_type value) noexcept
	{
		switch(value)
		{
		case image_type::planar_1D:
			return vk::ImageType::e1D;
			break;
		case image_type::planar_2D:
			return vk::ImageType::e2D;
			break;
		case image_type::planar_3D:
			return vk::ImageType::e3D;
			break;
		case image_type::cube_2D:
			return vk::ImageType::e2D;
			break;
		case image_type::array_1D:
			return vk::ImageType::e1D;
			break;
		case image_type::array_2D:
			return vk::ImageType::e2D;
			break;
		case image_type::array_cube:
			return vk::ImageType::e2D;
			break;
		}

		assert(false);
		return vk::ImageType::e1D;
	}
	inline vk::ImageAspectFlags to_vk(image_aspect value) noexcept
	{
		return vk::ImageAspectFlags {vk::ImageAspectFlagBits(static_cast<std::underlying_type_t<image_aspect>>(value))};
	}

	inline image_aspect to_image_aspect(vk::ImageAspectFlags value) noexcept
	{
		return image_aspect(static_cast<VkImageAspectFlags>(value));
	}

	inline vk::ImageUsageFlags to_vk(image_usage value) noexcept
	{
		return vk::ImageUsageFlags {vk::ImageUsageFlagBits(static_cast<std::underlying_type_t<image_usage>>(value))};
	}

	inline image_usage to_image_usage(vk::ImageUsageFlags value) noexcept
	{
		return image_usage(static_cast<VkImageUsageFlags>(value));
	}

	inline vk::ColorComponentFlags to_vk(component_bits value) noexcept
	{
		return vk::ColorComponentFlags {
		  vk::ColorComponentFlagBits(static_cast<std::underlying_type_t<component_bits>>(value))};
	}
	inline component_bits to_component_bits(vk::ColorComponentFlags value) noexcept
	{
		return component_bits(static_cast<VkColorComponentFlags>(value));
	}


	inline format_t to_format(vk::Format value) noexcept
	{
		switch(static_cast<VkFormat>(value))
		{
		case VK_FORMAT_R4G4_UNORM_PACK8:
			return format_t::r4g4_unorm_pack8;
			break;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
			return format_t::r4g4b4a4_unorm_pack16;
			break;
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
			return format_t::b4g4r4a4_unorm_pack16;
			break;
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
			return format_t::r5g6b5_unorm_pack16;
			break;
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
			return format_t::b5g6r5_unorm_pack16;
			break;
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
			return format_t::r5g5b5a1_unorm_pack16;
			break;
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
			return format_t::b5g5r5a1_unorm_pack16;
			break;
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			return format_t::a1r5g5b5_unorm_pack16;
			break;
		case VK_FORMAT_R8_UNORM:
			return format_t::r8_unorm;
			break;
		case VK_FORMAT_R8_SNORM:
			return format_t::r8_snorm;
			break;
		case VK_FORMAT_R8_UINT:
			return format_t::r8_uint;
			break;
		case VK_FORMAT_R8_SINT:
			return format_t::r8_sint;
			break;
		case VK_FORMAT_R8_SRGB:
			return format_t::r8_srgb;
			break;
		case VK_FORMAT_R8G8_UNORM:
			return format_t::r8g8_unorm;
			break;
		case VK_FORMAT_R8G8_SNORM:
			return format_t::r8g8_snorm;
			break;
		case VK_FORMAT_R8G8_UINT:
			return format_t::r8g8_uint;
			break;
		case VK_FORMAT_R8G8_SINT:
			return format_t::r8g8_sint;
			break;
		case VK_FORMAT_R8G8_SRGB:
			return format_t::r8g8_srgb;
			break;
		case VK_FORMAT_R8G8B8_UNORM:
			return format_t::r8g8b8_unorm;
			break;
		case VK_FORMAT_R8G8B8_SNORM:
			return format_t::r8g8b8_snorm;
			break;
		case VK_FORMAT_R8G8B8_UINT:
			return format_t::r8g8b8_uint;
			break;
		case VK_FORMAT_R8G8B8_SINT:
			return format_t::r8g8b8_sint;
			break;
		case VK_FORMAT_R8G8B8_SRGB:
			return format_t::r8g8b8_srgb;
			break;
		case VK_FORMAT_B8G8R8_UNORM:
			return format_t::b8g8r8_unorm;
			break;
		case VK_FORMAT_B8G8R8_SNORM:
			return format_t::b8g8r8_snorm;
			break;
		case VK_FORMAT_B8G8R8_UINT:
			return format_t::b8g8r8_uint;
			break;
		case VK_FORMAT_B8G8R8_SINT:
			return format_t::b8g8r8_sint;
			break;
		case VK_FORMAT_B8G8R8_SRGB:
			return format_t::b8g8r8_srgb;
			break;
		case VK_FORMAT_R8G8B8A8_UNORM:
			return format_t::r8g8b8a8_unorm;
			break;
		case VK_FORMAT_R8G8B8A8_SNORM:
			return format_t::r8g8b8a8_snorm;
			break;
		case VK_FORMAT_R8G8B8A8_UINT:
			return format_t::r8g8b8a8_uint;
			break;
		case VK_FORMAT_R8G8B8A8_SINT:
			return format_t::r8g8b8a8_sint;
			break;
		case VK_FORMAT_R8G8B8A8_SRGB:
			return format_t::r8g8b8a8_srgb;
			break;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return format_t::b8g8r8a8_unorm;
			break;
		case VK_FORMAT_B8G8R8A8_SNORM:
			return format_t::b8g8r8a8_snorm;
			break;
		case VK_FORMAT_B8G8R8A8_UINT:
			return format_t::b8g8r8a8_uint;
			break;
		case VK_FORMAT_B8G8R8A8_SINT:
			return format_t::b8g8r8a8_sint;
			break;
		case VK_FORMAT_B8G8R8A8_SRGB:
			return format_t::b8g8r8a8_srgb;
			break;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
			return format_t::a2r10g10b10_unorm_pack32;
			break;
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
			return format_t::a2r10g10b10_uint_pack32;
			break;
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			return format_t::a2b10g10r10_unorm_pack32;
			break;
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
			return format_t::a2b10g10r10_uint_pack32;
			break;
		case VK_FORMAT_R16_UNORM:
			return format_t::r16_unorm;
			break;
		case VK_FORMAT_R16_SNORM:
			return format_t::r16_snorm;
			break;
		case VK_FORMAT_R16_UINT:
			return format_t::r16_uint;
			break;
		case VK_FORMAT_R16_SINT:
			return format_t::r16_sint;
			break;
		case VK_FORMAT_R16_SFLOAT:
			return format_t::r16_sfloat;
			break;
		case VK_FORMAT_R16G16_UNORM:
			return format_t::r16g16_unorm;
			break;
		case VK_FORMAT_R16G16_SNORM:
			return format_t::r16g16_snorm;
			break;
		case VK_FORMAT_R16G16_UINT:
			return format_t::r16g16_uint;
			break;
		case VK_FORMAT_R16G16_SINT:
			return format_t::r16g16_sint;
			break;
		case VK_FORMAT_R16G16_SFLOAT:
			return format_t::r16g16_sfloat;
			break;
		case VK_FORMAT_R16G16B16_UNORM:
			return format_t::r16g16b16_unorm;
			break;
		case VK_FORMAT_R16G16B16_SNORM:
			return format_t::r16g16b16_snorm;
			break;
		case VK_FORMAT_R16G16B16_UINT:
			return format_t::r16g16b16_uint;
			break;
		case VK_FORMAT_R16G16B16_SINT:
			return format_t::r16g16b16_sint;
			break;
		case VK_FORMAT_R16G16B16_SFLOAT:
			return format_t::r16g16b16_sfloat;
			break;
		case VK_FORMAT_R16G16B16A16_UNORM:
			return format_t::r16g16b16a16_unorm;
			break;
		case VK_FORMAT_R16G16B16A16_SNORM:
			return format_t::r16g16b16a16_snorm;
			break;
		case VK_FORMAT_R16G16B16A16_UINT:
			return format_t::r16g16b16a16_uint;
			break;
		case VK_FORMAT_R16G16B16A16_SINT:
			return format_t::r16g16b16a16_sint;
			break;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return format_t::r16g16b16a16_sfloat;
			break;
		case VK_FORMAT_R32_UINT:
			return format_t::r32_uint;
			break;
		case VK_FORMAT_R32_SINT:
			return format_t::r32_sint;
			break;
		case VK_FORMAT_R32_SFLOAT:
			return format_t::r32_sfloat;
			break;
		case VK_FORMAT_R32G32_UINT:
			return format_t::r32g32_uint;
			break;
		case VK_FORMAT_R32G32_SINT:
			return format_t::r32g32_sint;
			break;
		case VK_FORMAT_R32G32_SFLOAT:
			return format_t::r32g32_sfloat;
			break;
		case VK_FORMAT_R32G32B32_UINT:
			return format_t::r32g32b32_uint;
			break;
		case VK_FORMAT_R32G32B32_SINT:
			return format_t::r32g32b32_sint;
			break;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return format_t::r32g32b32_sfloat;
			break;
		case VK_FORMAT_R32G32B32A32_UINT:
			return format_t::r32g32b32a32_uint;
			break;
		case VK_FORMAT_R32G32B32A32_SINT:
			return format_t::r32g32b32a32_sint;
			break;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return format_t::r32g32b32a32_sfloat;
			break;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			return format_t::b10g11r11_ufloat_pack32;
			break;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			return format_t::e5b9g9r9_ufloat_pack32;
			break;
		case VK_FORMAT_D16_UNORM:
			return format_t::d16_unorm;
			break;
		case VK_FORMAT_X8_D24_UNORM_PACK32:
			return format_t::x8_d24_unorm_pack32;
			break;
		case VK_FORMAT_D32_SFLOAT:
			return format_t::d32_sfloat;
			break;
		case VK_FORMAT_S8_UINT:
			return format_t::s8_uint;
			break;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return format_t::d24_unorm_s8_uint;
			break;
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return format_t::d32_sfloat_s8_uint;
			break;
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
			return format_t::bc1_rgb_unorm_block;
			break;
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
			return format_t::bc1_rgb_srgb_block;
			break;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			return format_t::bc1_rgba_unorm_block;
			break;
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			return format_t::bc1_rgba_srgb_block;
			break;
		case VK_FORMAT_BC2_UNORM_BLOCK:
			return format_t::bc2_unorm_block;
			break;
		case VK_FORMAT_BC2_SRGB_BLOCK:
			return format_t::bc2_srgb_block;
			break;
		case VK_FORMAT_BC3_UNORM_BLOCK:
			return format_t::bc3_unorm_block;
			break;
		case VK_FORMAT_BC3_SRGB_BLOCK:
			return format_t::bc3_srgb_block;
			break;
		case VK_FORMAT_BC4_UNORM_BLOCK:
			return format_t::bc4_unorm_block;
			break;
		case VK_FORMAT_BC4_SNORM_BLOCK:
			return format_t::bc4_snorm_block;
			break;
		case VK_FORMAT_BC5_UNORM_BLOCK:
			return format_t::bc5_unorm_block;
			break;
		case VK_FORMAT_BC5_SNORM_BLOCK:
			return format_t::bc5_snorm_block;
			break;
		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
			return format_t::bc6h_ufloat_block;
			break;
		case VK_FORMAT_BC6H_SFLOAT_BLOCK:
			return format_t::bc6h_sfloat_block;
			break;
		case VK_FORMAT_BC7_UNORM_BLOCK:
			return format_t::bc7_unorm_block;
			break;
		case VK_FORMAT_BC7_SRGB_BLOCK:
			return format_t::bc7_srgb_block;
			break;
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
			return format_t::etc2_r8g8b8_unorm_block;
			break;
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
			return format_t::etc2_r8g8b8_srgb_block;
			break;
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
			return format_t::etc2_r8g8b8a1_unorm_block;
			break;
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
			return format_t::etc2_r8g8b8a1_srgb_block;
			break;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
			return format_t::etc2_r8g8b8a8_unorm_block;
			break;
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
			return format_t::etc2_r8g8b8a8_srgb_block;
			break;
		case VK_FORMAT_EAC_R11_UNORM_BLOCK:
			return format_t::eac_r11_unorm_block;
			break;
		case VK_FORMAT_EAC_R11_SNORM_BLOCK:
			return format_t::eac_r11_snorm_block;
			break;
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
			return format_t::eac_r11g11_unorm_block;
			break;
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
			return format_t::eac_r11g11_snorm_block;
			break;
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
			return format_t::astc_4x4_unorm_block;
			break;
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
			return format_t::astc_4x4_srgb_block;
			break;
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
			return format_t::astc_5x4_unorm_block;
			break;
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
			return format_t::astc_5x4_srgb_block;
			break;
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
			return format_t::astc_5x5_unorm_block;
			break;
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
			return format_t::astc_5x5_srgb_block;
			break;
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
			return format_t::astc_6x5_unorm_block;
			break;
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
			return format_t::astc_6x5_srgb_block;
			break;
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
			return format_t::astc_6x6_unorm_block;
			break;
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
			return format_t::astc_6x6_srgb_block;
			break;
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
			return format_t::astc_8x5_unorm_block;
			break;
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
			return format_t::astc_8x5_srgb_block;
			break;
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
			return format_t::astc_8x6_unorm_block;
			break;
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
			return format_t::astc_8x6_srgb_block;
			break;
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
			return format_t::astc_8x8_unorm_block;
			break;
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
			return format_t::astc_8x8_srgb_block;
			break;
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
			return format_t::astc_10x5_unorm_block;
			break;
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
			return format_t::astc_10x5_srgb_block;
			break;
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
			return format_t::astc_10x6_unorm_block;
			break;
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
			return format_t::astc_10x6_srgb_block;
			break;
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
			return format_t::astc_10x8_unorm_block;
			break;
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
			return format_t::astc_10x8_srgb_block;
			break;
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
			return format_t::astc_10x10_unorm_block;
			break;
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
			return format_t::astc_10x10_srgb_block;
			break;
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
			return format_t::astc_12x10_unorm_block;
			break;
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
			return format_t::astc_12x10_srgb_block;
			break;
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
			return format_t::astc_12x12_unorm_block;
			break;
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
			return format_t::astc_12x12_srgb_block;
			break;
		case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
			return format_t::pvrtc1_2bpp_unorm_block_img;
			break;
		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
			return format_t::pvrtc1_4bpp_unorm_block_img;
			break;
		case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
			return format_t::pvrtc2_2bpp_unorm_block_img;
			break;
		case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
			return format_t::pvrtc2_4bpp_unorm_block_img;
			break;
		case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
			return format_t::pvrtc1_2bpp_srgb_block_img;
			break;
		case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
			return format_t::pvrtc1_4bpp_srgb_block_img;
			break;
		case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
			return format_t::pvrtc2_2bpp_srgb_block_img;
			break;
		case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
			return format_t::pvrtc2_4bpp_srgb_block_img;
			break;
		case VK_FORMAT_UNDEFINED:
			return format_t::undefined;
			break;
		default:
			assert(false);
		}
		return format_t::undefined;
	}
	inline vk::Format to_vk(format_t value) noexcept
	{
		switch(value)
		{
		case format_t::r4g4_unorm_pack8:
			return vk::Format::eR4G4UnormPack8;
			break;
		case format_t::r4g4b4a4_unorm_pack16:
			return vk::Format::eR4G4B4A4UnormPack16;
			break;
		case format_t::b4g4r4a4_unorm_pack16:
			return vk::Format::eB4G4R4A4UnormPack16;
			break;
		case format_t::r5g6b5_unorm_pack16:
			return vk::Format::eR5G6B5UnormPack16;
			break;
		case format_t::b5g6r5_unorm_pack16:
			return vk::Format::eB5G6R5UnormPack16;
			break;
		case format_t::r5g5b5a1_unorm_pack16:
			return vk::Format::eR5G5B5A1UnormPack16;
			break;
		case format_t::b5g5r5a1_unorm_pack16:
			return vk::Format::eB5G5R5A1UnormPack16;
			break;
		case format_t::a1r5g5b5_unorm_pack16:
			return vk::Format::eA1R5G5B5UnormPack16;
			break;
		case format_t::r8_unorm:
			return vk::Format::eR8Unorm;
			break;
		case format_t::r8_snorm:
			return vk::Format::eR8Snorm;
			break;
		case format_t::r8_uint:
			return vk::Format::eR8Uint;
			break;
		case format_t::r8_sint:
			return vk::Format::eR8Sint;
			break;
		case format_t::r8_srgb:
			return vk::Format::eR8Srgb;
			break;
		case format_t::r8g8_unorm:
			return vk::Format::eR8G8Unorm;
			break;
		case format_t::r8g8_snorm:
			return vk::Format::eR8G8Snorm;
			break;
		case format_t::r8g8_uint:
			return vk::Format::eR8G8Uint;
			break;
		case format_t::r8g8_sint:
			return vk::Format::eR8G8Sint;
			break;
		case format_t::r8g8_srgb:
			return vk::Format::eR8G8Srgb;
			break;
		case format_t::r8g8b8_unorm:
			return vk::Format::eR8G8B8Unorm;
			break;
		case format_t::r8g8b8_snorm:
			return vk::Format::eR8G8B8Snorm;
			break;
		case format_t::r8g8b8_uint:
			return vk::Format::eR8G8B8Uint;
			break;
		case format_t::r8g8b8_sint:
			return vk::Format::eR8G8B8Sint;
			break;
		case format_t::r8g8b8_srgb:
			return vk::Format::eR8G8B8Srgb;
			break;
		case format_t::b8g8r8_unorm:
			return vk::Format::eB8G8R8Unorm;
			break;
		case format_t::b8g8r8_snorm:
			return vk::Format::eB8G8R8Snorm;
			break;
		case format_t::b8g8r8_uint:
			return vk::Format::eB8G8R8Uint;
			break;
		case format_t::b8g8r8_sint:
			return vk::Format::eB8G8R8Sint;
			break;
		case format_t::b8g8r8_srgb:
			return vk::Format::eB8G8R8Srgb;
			break;
		case format_t::r8g8b8a8_unorm:
			return vk::Format::eR8G8B8A8Unorm;
			break;
		case format_t::r8g8b8a8_snorm:
			return vk::Format::eR8G8B8A8Snorm;
			break;
		case format_t::r8g8b8a8_uint:
			return vk::Format::eR8G8B8A8Uint;
			break;
		case format_t::r8g8b8a8_sint:
			return vk::Format::eR8G8B8A8Sint;
			break;
		case format_t::r8g8b8a8_srgb:
			return vk::Format::eR8G8B8A8Srgb;
			break;
		case format_t::b8g8r8a8_unorm:
			return vk::Format::eB8G8R8A8Unorm;
			break;
		case format_t::b8g8r8a8_snorm:
			return vk::Format::eB8G8R8A8Snorm;
			break;
		case format_t::b8g8r8a8_uint:
			return vk::Format::eB8G8R8A8Uint;
			break;
		case format_t::b8g8r8a8_sint:
			return vk::Format::eB8G8R8A8Sint;
			break;
		case format_t::b8g8r8a8_srgb:
			return vk::Format::eB8G8R8A8Srgb;
			break;
		case format_t::a2r10g10b10_unorm_pack32:
			return vk::Format::eA2R10G10B10UnormPack32;
			break;
		// case format_t::a2r10g10b10_snorm_pack32: return vk::Format::eA2R10G10B10SnormPack32; break;
		case format_t::a2r10g10b10_uint_pack32:
			return vk::Format::eA2R10G10B10UintPack32;
			break;
		// case format_t::a2r10g10b10_sint_pack32: return vk::Format::eA2R10G10B10SintPack32; break;
		case format_t::a2b10g10r10_unorm_pack32:
			return vk::Format::eA2B10G10R10UnormPack32;
			break;
		// case format_t::a2b10g10r10_snorm_pack32: return vk::Format::eA2B10G10R10SnormPack32; break;
		case format_t::a2b10g10r10_uint_pack32:
			return vk::Format::eA2B10G10R10UintPack32;
			break;
		// case format_t::a2b10g10r10_sint_pack32: return vk::Format::eA2B10G10R10SintPack32; break;
		case format_t::r16_unorm:
			return vk::Format::eR16Unorm;
			break;
		case format_t::r16_snorm:
			return vk::Format::eR16Snorm;
			break;
		case format_t::r16_uint:
			return vk::Format::eR16Uint;
			break;
		case format_t::r16_sint:
			return vk::Format::eR16Sint;
			break;
		case format_t::r16_sfloat:
			return vk::Format::eR16Sfloat;
			break;
		case format_t::r16g16_unorm:
			return vk::Format::eR16G16Unorm;
			break;
		case format_t::r16g16_snorm:
			return vk::Format::eR16G16Snorm;
			break;
		case format_t::r16g16_uint:
			return vk::Format::eR16G16Uint;
			break;
		case format_t::r16g16_sint:
			return vk::Format::eR16G16Sint;
			break;
		case format_t::r16g16_sfloat:
			return vk::Format::eR16G16Sfloat;
			break;
		case format_t::r16g16b16_unorm:
			return vk::Format::eR16G16B16Unorm;
			break;
		case format_t::r16g16b16_snorm:
			return vk::Format::eR16G16B16Snorm;
			break;
		case format_t::r16g16b16_uint:
			return vk::Format::eR16G16B16Uint;
			break;
		case format_t::r16g16b16_sint:
			return vk::Format::eR16G16B16Sint;
			break;
		case format_t::r16g16b16_sfloat:
			return vk::Format::eR16G16B16Sfloat;
			break;
		case format_t::r16g16b16a16_unorm:
			return vk::Format::eR16G16B16A16Unorm;
			break;
		case format_t::r16g16b16a16_snorm:
			return vk::Format::eR16G16B16A16Snorm;
			break;
		case format_t::r16g16b16a16_uint:
			return vk::Format::eR16G16B16A16Uint;
			break;
		case format_t::r16g16b16a16_sint:
			return vk::Format::eR16G16B16A16Sint;
			break;
		case format_t::r16g16b16a16_sfloat:
			return vk::Format::eR16G16B16A16Sfloat;
			break;
		case format_t::r32_uint:
			return vk::Format::eR32Uint;
			break;
		case format_t::r32_sint:
			return vk::Format::eR32Sint;
			break;
		case format_t::r32_sfloat:
			return vk::Format::eR32Sfloat;
			break;
		case format_t::r32g32_uint:
			return vk::Format::eR32G32Uint;
			break;
		case format_t::r32g32_sint:
			return vk::Format::eR32G32Sint;
			break;
		case format_t::r32g32_sfloat:
			return vk::Format::eR32G32Sfloat;
			break;
		case format_t::r32g32b32_uint:
			return vk::Format::eR32G32B32Uint;
			break;
		case format_t::r32g32b32_sint:
			return vk::Format::eR32G32B32Sint;
			break;
		case format_t::r32g32b32_sfloat:
			return vk::Format::eR32G32B32Sfloat;
			break;
		case format_t::r32g32b32a32_uint:
			return vk::Format::eR32G32B32A32Uint;
			break;
		case format_t::r32g32b32a32_sint:
			return vk::Format::eR32G32B32A32Sint;
			break;
		case format_t::r32g32b32a32_sfloat:
			return vk::Format::eR32G32B32A32Sfloat;
			break;
		/*case format_t::r64_uint: return vk::Format::eR64Uint; break;
		case format_t::r64_sint: return vk::Format::eR64Sint; break;
		case format_t::r64_sfloat: return vk::Format::eR64Sfloat; break;
		case format_t::r64g64_uint: return vk::Format::eR64G64Uint; break;
		case format_t::r64g64_sint: return vk::Format::eR64G64Sint; break;
		case format_t::r64g64_sfloat: return vk::Format::eR64G64Sfloat; break;
		case format_t::r64g64b64_uint: return vk::Format::eR64G64B64Uint; break;
		case format_t::r64g64b64_sint: return vk::Format::eR64G64B64Sint; break;
		case format_t::r64g64b64_sfloat: return vk::Format::eR64G64B64Sfloat; break;
		case format_t::r64g64b64a64_uint: return vk::Format::eR64G64B64A64Uint; break;
		case format_t::r64g64b64a64_sint: return vk::Format::eR64G64B64A64Sint; break;
		case format_t::r64g64b64a64_sfloat: return vk::Format::eR64G64B64A64Sfloat; break;*/
		case format_t::b10g11r11_ufloat_pack32:
			return vk::Format::eB10G11R11UfloatPack32;
			break;
		case format_t::e5b9g9r9_ufloat_pack32:
			return vk::Format::eE5B9G9R9UfloatPack32;
			break;
		case format_t::d16_unorm:
			return vk::Format::eD16Unorm;
			break;
		case format_t::x8_d24_unorm_pack32:
			return vk::Format::eX8D24UnormPack32;
			break;
		case format_t::d32_sfloat:
			return vk::Format::eD32Sfloat;
			break;
		case format_t::s8_uint:
			return vk::Format::eS8Uint;
			break;
		// case format_t::d16_unorm_s8_uint: return vk::Format::eD16UnormS8Uint; break;
		case format_t::d24_unorm_s8_uint:
			return vk::Format::eD24UnormS8Uint;
			break;
		case format_t::d32_sfloat_s8_uint:
			return vk::Format::eD32SfloatS8Uint;
			break;
		case format_t::bc1_rgb_unorm_block:
			return vk::Format::eBc1RgbUnormBlock;
			break;
		case format_t::bc1_rgb_srgb_block:
			return vk::Format::eBc1RgbSrgbBlock;
			break;
		case format_t::bc1_rgba_unorm_block:
			return vk::Format::eBc1RgbaUnormBlock;
			break;
		case format_t::bc1_rgba_srgb_block:
			return vk::Format::eBc1RgbaSrgbBlock;
			break;
		case format_t::bc2_unorm_block:
			return vk::Format::eBc2UnormBlock;
			break;
		case format_t::bc2_srgb_block:
			return vk::Format::eBc2SrgbBlock;
			break;
		case format_t::bc3_unorm_block:
			return vk::Format::eBc3UnormBlock;
			break;
		case format_t::bc3_srgb_block:
			return vk::Format::eBc3SrgbBlock;
			break;
		case format_t::bc4_unorm_block:
			return vk::Format::eBc4UnormBlock;
			break;
		case format_t::bc4_snorm_block:
			return vk::Format::eBc4SnormBlock;
			break;
		case format_t::bc5_unorm_block:
			return vk::Format::eBc5UnormBlock;
			break;
		case format_t::bc5_snorm_block:
			return vk::Format::eBc5SnormBlock;
			break;
		case format_t::bc6h_ufloat_block:
			return vk::Format::eBc6HUfloatBlock;
			break;
		case format_t::bc6h_sfloat_block:
			return vk::Format::eBc6HSfloatBlock;
			break;
		case format_t::bc7_unorm_block:
			return vk::Format::eBc7UnormBlock;
			break;
		case format_t::bc7_srgb_block:
			return vk::Format::eBc7SrgbBlock;
			break;
		case format_t::etc2_r8g8b8_unorm_block:
			return vk::Format::eEtc2R8G8B8UnormBlock;
			break;
		case format_t::etc2_r8g8b8_srgb_block:
			return vk::Format::eEtc2R8G8B8SrgbBlock;
			break;
		case format_t::etc2_r8g8b8a1_unorm_block:
			return vk::Format::eEtc2R8G8B8A1UnormBlock;
			break;
		case format_t::etc2_r8g8b8a1_srgb_block:
			return vk::Format::eEtc2R8G8B8A1SrgbBlock;
			break;
		case format_t::etc2_r8g8b8a8_unorm_block:
			return vk::Format::eEtc2R8G8B8A8UnormBlock;
			break;
		case format_t::etc2_r8g8b8a8_srgb_block:
			return vk::Format::eEtc2R8G8B8A8SrgbBlock;
			break;
		case format_t::eac_r11_unorm_block:
			return vk::Format::eEacR11UnormBlock;
			break;
		case format_t::eac_r11_snorm_block:
			return vk::Format::eEacR11SnormBlock;
			break;
		case format_t::eac_r11g11_unorm_block:
			return vk::Format::eEacR11G11UnormBlock;
			break;
		case format_t::eac_r11g11_snorm_block:
			return vk::Format::eEacR11G11SnormBlock;
			break;
		case format_t::astc_4x4_unorm_block:
			return vk::Format::eAstc4x4UnormBlock;
			break;
		case format_t::astc_4x4_srgb_block:
			return vk::Format::eAstc4x4SrgbBlock;
			break;
		case format_t::astc_5x4_unorm_block:
			return vk::Format::eAstc5x4UnormBlock;
			break;
		case format_t::astc_5x4_srgb_block:
			return vk::Format::eAstc5x4SrgbBlock;
			break;
		case format_t::astc_5x5_unorm_block:
			return vk::Format::eAstc5x5UnormBlock;
			break;
		case format_t::astc_5x5_srgb_block:
			return vk::Format::eAstc5x5SrgbBlock;
			break;
		case format_t::astc_6x5_unorm_block:
			return vk::Format::eAstc6x5UnormBlock;
			break;
		case format_t::astc_6x5_srgb_block:
			return vk::Format::eAstc6x5SrgbBlock;
			break;
		case format_t::astc_6x6_unorm_block:
			return vk::Format::eAstc6x6UnormBlock;
			break;
		case format_t::astc_6x6_srgb_block:
			return vk::Format::eAstc6x6SrgbBlock;
			break;
		case format_t::astc_8x5_unorm_block:
			return vk::Format::eAstc8x5UnormBlock;
			break;
		case format_t::astc_8x5_srgb_block:
			return vk::Format::eAstc8x5SrgbBlock;
			break;
		case format_t::astc_8x6_unorm_block:
			return vk::Format::eAstc8x6UnormBlock;
			break;
		case format_t::astc_8x6_srgb_block:
			return vk::Format::eAstc8x6SrgbBlock;
			break;
		case format_t::astc_8x8_unorm_block:
			return vk::Format::eAstc8x8UnormBlock;
			break;
		case format_t::astc_8x8_srgb_block:
			return vk::Format::eAstc8x8SrgbBlock;
			break;
		case format_t::astc_10x5_unorm_block:
			return vk::Format::eAstc10x5UnormBlock;
			break;
		case format_t::astc_10x5_srgb_block:
			return vk::Format::eAstc10x5SrgbBlock;
			break;
		case format_t::astc_10x6_unorm_block:
			return vk::Format::eAstc10x6UnormBlock;
			break;
		case format_t::astc_10x6_srgb_block:
			return vk::Format::eAstc10x6SrgbBlock;
			break;
		case format_t::astc_10x8_unorm_block:
			return vk::Format::eAstc10x8UnormBlock;
			break;
		case format_t::astc_10x8_srgb_block:
			return vk::Format::eAstc10x8SrgbBlock;
			break;
		case format_t::astc_10x10_unorm_block:
			return vk::Format::eAstc10x10UnormBlock;
			break;
		case format_t::astc_10x10_srgb_block:
			return vk::Format::eAstc10x10SrgbBlock;
			break;
		case format_t::astc_12x10_unorm_block:
			return vk::Format::eAstc12x10UnormBlock;
			break;
		case format_t::astc_12x10_srgb_block:
			return vk::Format::eAstc12x10SrgbBlock;
			break;
		case format_t::astc_12x12_unorm_block:
			return vk::Format::eAstc12x12UnormBlock;
			break;
		case format_t::astc_12x12_srgb_block:
			return vk::Format::eAstc12x12SrgbBlock;
			break;
		case format_t::pvrtc1_2bpp_unorm_block_img:
			return vk::Format::ePvrtc12BppUnormBlockIMG;
			break;
		case format_t::pvrtc1_4bpp_unorm_block_img:
			return vk::Format::ePvrtc14BppUnormBlockIMG;
			break;
		case format_t::pvrtc2_2bpp_unorm_block_img:
			return vk::Format::ePvrtc22BppUnormBlockIMG;
			break;
		case format_t::pvrtc2_4bpp_unorm_block_img:
			return vk::Format::ePvrtc24BppUnormBlockIMG;
			break;
		case format_t::pvrtc1_2bpp_srgb_block_img:
			return vk::Format::ePvrtc12BppSrgbBlockIMG;
			break;
		case format_t::pvrtc1_4bpp_srgb_block_img:
			return vk::Format::ePvrtc14BppSrgbBlockIMG;
			break;
		case format_t::pvrtc2_2bpp_srgb_block_img:
			return vk::Format::ePvrtc22BppSrgbBlockIMG;
			break;
		case format_t::pvrtc2_4bpp_srgb_block_img:
			return vk::Format::ePvrtc24BppSrgbBlockIMG;
			break;
		}
		return vk::Format::eUndefined;
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
		case binding_type::sampler:
			return vk::DescriptorType::eSampler;
			break;
		case binding_type::combined_image_sampler:
			return vk::DescriptorType::eCombinedImageSampler;
			break;
		case binding_type::sampled_image:
			return vk::DescriptorType::eSampledImage;
			break;
		case binding_type::storage_image:
			return vk::DescriptorType::eStorageImage;
			break;
		case binding_type::uniform_texel_buffer:
			return vk::DescriptorType::eUniformTexelBuffer;
			break;
		case binding_type::storage_texel_buffer:
			return vk::DescriptorType::eStorageTexelBuffer;
			break;
		case binding_type::uniform_buffer:
			return vk::DescriptorType::eUniformBuffer;
			break;
		case binding_type::storage_buffer:
			return vk::DescriptorType::eStorageBuffer;
			break;
		case binding_type::uniform_buffer_dynamic:
			return vk::DescriptorType::eUniformBufferDynamic;
			break;
		case binding_type::storage_buffer_dynamic:
			return vk::DescriptorType::eStorageBufferDynamic;
			break;
		case binding_type::input_attachment:
			return vk::DescriptorType::eInputAttachment;
			break;
		}
		assert(false);
		return vk::DescriptorType {0};
	}

	inline binding_type to_binding_type(vk::DescriptorType value) noexcept
	{
		switch(value)
		{
		case vk::DescriptorType::eSampler:
			return binding_type::sampler;
			break;
		case vk::DescriptorType::eCombinedImageSampler:
			return binding_type::combined_image_sampler;
			break;
		case vk::DescriptorType::eSampledImage:
			return binding_type::sampled_image;
			break;
		case vk::DescriptorType::eStorageImage:
			return binding_type::storage_image;
			break;
		case vk::DescriptorType::eUniformTexelBuffer:
			return binding_type::uniform_texel_buffer;
			break;
		case vk::DescriptorType::eStorageTexelBuffer:
			return binding_type::storage_texel_buffer;
			break;
		case vk::DescriptorType::eUniformBuffer:
			return binding_type::uniform_buffer;
			break;
		case vk::DescriptorType::eStorageBuffer:
			return binding_type::storage_buffer;
			break;
		case vk::DescriptorType::eUniformBufferDynamic:
			return binding_type::uniform_buffer_dynamic;
			break;
		case vk::DescriptorType::eStorageBufferDynamic:
			return binding_type::storage_buffer_dynamic;
			break;
		case vk::DescriptorType::eInputAttachment:
			return binding_type::input_attachment;
			break;
		}
		assert(false);
		return binding_type {0};
	}
	inline vk::BlendOp to_vk(blend_op value) noexcept
	{
		return vk::BlendOp {static_cast<std::underlying_type_t<blend_op>>(value)};
	}

	inline blend_op to_blend_op(vk::BlendOp value) noexcept
	{
		return blend_op(static_cast<std::underlying_type_t<vk::BlendOp>>(value));
	}

	inline vk::BlendFactor to_vk(blend_factor value) noexcept
	{
		return vk::BlendFactor {static_cast<std::underlying_type_t<blend_factor>>(value)};
	}

	inline blend_factor to_blend_factor(vk::BlendFactor value) noexcept
	{
		return blend_factor(static_cast<std::underlying_type_t<vk::BlendFactor>>(value));
	}

	inline vk::AttachmentLoadOp to_vk(core::gfx::attachment::load_op value) noexcept
	{
		using vk_etype	= vk::AttachmentLoadOp;
		using vk_type	= std::underlying_type_t<vk_etype>;
		using gfx_etype = core::gfx::attachment::load_op;
		using gfx_type	= std::underlying_type_t<gfx_etype>;
		static_assert(static_cast<vk_type>(vk_etype::eLoad) == static_cast<gfx_type>(gfx_etype::load));
		static_assert(static_cast<vk_type>(vk_etype::eClear) == static_cast<gfx_type>(gfx_etype::clear));
		static_assert(static_cast<vk_type>(vk_etype::eDontCare) == static_cast<gfx_type>(gfx_etype::dont_care));

		return vk_etype {static_cast<gfx_type>(value)};
	}

	inline vk::AttachmentStoreOp to_vk(core::gfx::attachment::store_op value) noexcept
	{
		switch(value)
		{
		case core::gfx::attachment::store_op::store:
			return vk::AttachmentStoreOp::eStore;
			break;
		case core::gfx::attachment::store_op::dont_care:
			return vk::AttachmentStoreOp::eDontCare;
			break;
		}

		assert(false);
		return vk::AttachmentStoreOp {0};
	}

	inline vk::ImageLayout to_vk(core::gfx::image::layout layout) noexcept
	{
		using vk_etype	= vk::ImageLayout;
		using vk_type	= std::underlying_type_t<vk_etype>;
		using gfx_etype = core::gfx::image::layout;
		using gfx_type	= std::underlying_type_t<gfx_etype>;
		static_assert(static_cast<vk_type>(vk_etype::eUndefined) == static_cast<gfx_type>(gfx_etype::undefined));
		static_assert(static_cast<vk_type>(vk_etype::eGeneral) == static_cast<gfx_type>(gfx_etype::general));
		static_assert(static_cast<vk_type>(vk_etype::eColorAttachmentOptimal) ==
					  static_cast<gfx_type>(gfx_etype::color_attachment_optimal));
		static_assert(static_cast<vk_type>(vk_etype::eDepthStencilAttachmentOptimal) ==
					  static_cast<gfx_type>(gfx_etype::depth_stencil_attachment_optimal));
		static_assert(static_cast<vk_type>(vk_etype::eDepthStencilReadOnlyOptimal) ==
					  static_cast<gfx_type>(gfx_etype::depth_stencil_read_only_optimal));
		static_assert(static_cast<vk_type>(vk_etype::eShaderReadOnlyOptimal) ==
					  static_cast<gfx_type>(gfx_etype::shader_read_only_optimal));
		static_assert(static_cast<vk_type>(vk_etype::eTransferSrcOptimal) ==
					  static_cast<gfx_type>(gfx_etype::transfer_source_optimal));
		static_assert(static_cast<vk_type>(vk_etype::eTransferDstOptimal) ==
					  static_cast<gfx_type>(gfx_etype::transfer_destination_optimal));
		static_assert(static_cast<vk_type>(vk_etype::ePreinitialized) ==
					  static_cast<gfx_type>(gfx_etype::preinitialized));
		static_assert(static_cast<vk_type>(vk_etype::eDepthReadOnlyStencilAttachmentOptimal) ==
					  static_cast<gfx_type>(gfx_etype::depth_read_only_stencil_attachment_optimal));
		static_assert(static_cast<vk_type>(vk_etype::eDepthAttachmentStencilReadOnlyOptimal) ==
					  static_cast<gfx_type>(gfx_etype::depth_attachment_stencil_read_only_optimal));
		static_assert(static_cast<vk_type>(vk_etype::ePresentSrcKHR) ==
					  static_cast<gfx_type>(gfx_etype::present_src_khr));
		static_assert(static_cast<vk_type>(vk_etype::eSharedPresentKHR) ==
					  static_cast<gfx_type>(gfx_etype::shared_present_khr));
		static_assert(static_cast<vk_type>(vk_etype::eShadingRateOptimalNV) ==
					  static_cast<gfx_type>(gfx_etype::shading_rate_optimal_nv));
		static_assert(static_cast<vk_type>(vk_etype::eFragmentDensityMapOptimalEXT) ==
					  static_cast<gfx_type>(gfx_etype::fragment_density_map_optimal_ext));

		return vk_etype {static_cast<gfx_type>(layout)};
	}

	inline vk::AttachmentDescription to_vk(const core::gfx::attachment& attachment) noexcept
	{
		return {vk::AttachmentDescriptionFlagBits::eMayAlias,
				to_vk(attachment.format),
				vk::SampleCountFlagBits {attachment.sample_bits},
				to_vk(attachment.image_load),
				to_vk(attachment.image_store),
				to_vk(attachment.stencil_load),
				to_vk(attachment.stencil_store),
				to_vk(attachment.initial),
				to_vk(attachment.final)};
	}

	inline vk::ClearValue to_vk(const core::gfx::clear_value& clear_value) noexcept
	{
		return std::visit(
		  [](const auto& value) -> vk::ClearValue {
			  using type = std::decay_t<decltype(value)>;
			  if constexpr(std::is_same_v<type, depth_stencil>)
			  {
				  return vk::ClearDepthStencilValue(value.depth, value.stencil);
			  }
			  else
				  return vk::ClearColorValue(value);
		  },
		  clear_value);
	}
}	 // namespace core::gfx::conversion