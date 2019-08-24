#pragma once
#include <optional>
#include "psl/memory/range.h"
#include "psl/memory/segment.h"

namespace core::gfx
{
	enum class graphics_backend
	{
		vulkan = 1 << 0,
		gles   = 1 << 1
	};

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

	/// \brief description of a memory commit instruction. Tries to offer some safer mechanisms.
	struct commit_instruction
	{

		/// \brief automatically construct the information from the type information of the source.
		/// \tparam T the type you wish to commit in this instruction.
		/// \param[in] source the source we will copy from.
		/// \param[in] segment the memory::segment we will copy to.
		/// \param[in] sub_range optional sub_range offset in the memory::segment, where a sub_range.begin() is
		/// equal to the head of the segment. \warning make sure the source outlives the commit instruction.
		template <typename T>
		commit_instruction(T* source, memory::segment segment, std::optional<memory::range> sub_range = std::nullopt)
			: segment(segment), sub_range(sub_range), source((std::uintptr_t)source), size(sizeof(T)){};
		commit_instruction(){};

		commit_instruction(void* source, size_t size, memory::segment segment,
						   std::optional<memory::range> sub_range = std::nullopt)
			: segment(segment), sub_range(sub_range), source((std::uintptr_t)source), size(size){};
		/// \brief target segment in the buffer
		memory::segment segment{};
		/// \brief possible sub range within the segment.
		///
		/// this is local offsets from the point of view of the segment
		/// (i.e. the sub_range.begin && sub_range.end can never be bigger than the segment.size() )
		std::optional<memory::range> sub_range;

		/// \brief source to copy from
		std::uintptr_t source{0};

		/// \brief sizeof source
		uint64_t size{0};
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

	enum class vertex_input_rate : uint8_t
	{
		vertex   = 0,
		instance = 1
	};

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
	enum class memory_usage
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

	inline memory_usage operator|(memory_usage bit0, memory_usage bit1)
	{
		using type = enum_flag<memory_usage>;
		return type(bit0) | bit1;
	}

	enum class memory_property
	{
		device_local	 = 1 << 0,
		host_visible	 = 1 << 1,
		host_coherent	= 1 << 2,
		host_cached		 = 1 << 3,
		lazily_allocated = 1 << 4,
		protected_memory = 1 << 5,
	};


	inline memory_property operator|(memory_property bit0, memory_property bit1)
	{
		using type = enum_flag<memory_property>;
		return type(bit0) | bit1;
	}

	inline bool operator&(memory_property bit0, memory_property bit1)
	{
		using type = enum_flag<memory_property>;
		return (type(bit0) & bit1) == bit1;
	}
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

	enum class sampler_mipmap_mode
	{
		nearest = 0,
		linear  = 1
	};

	enum class sampler_address_mode
	{
		repeat				 = 0,
		mirrored_repeat		 = 1,
		clamp_to_edge		 = 2,
		clamp_to_border		 = 3,
		mirror_clamp_to_edge = 4
	};
	enum class border_color
	{
		float_transparent_black = 0,
		int_transparent_black   = 1,
		float_opaque_black		= 2,
		int_opaque_black		= 3,
		float_opaque_white		= 4,
		int_opaque_white		= 5
	};

	enum class cullmode
	{
		none  = 0,
		front = 1,
		back  = 2,
		all   = 3
	};


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

	enum class filter
	{
		nearest = 0,
		linear  = 1,
		cubic   = 2
	};

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
	enum class blend_op
	{
		add				 = 0,
		subtract		 = 1,
		reverse_subtract = 2,
		min				 = 3,
		max				 = 4
	};

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

	inline bool has_depth(core::gfx::format format)
	{
		static std::vector<core::gfx::format> formats = {
			core::gfx::format::d16_unorm,  core::gfx::format::x8_d24_unorm_pack32, core::gfx::format::d24_unorm_s8_uint,
			core::gfx::format::d32_sfloat, core::gfx::format::d32_sfloat_s8_uint,
		};
		return std::find(formats.begin(), formats.end(), format) != std::end(formats);
	}

	inline bool has_stencil(core::gfx::format format)
	{
		static std::vector<core::gfx::format> formats = {
			core::gfx::format::s8_uint,
			core::gfx::format::d24_unorm_s8_uint,
			core::gfx::format::d32_sfloat_s8_uint,
		};
		return std::find(formats.begin(), formats.end(), format) != std::end(formats);
	}

	inline bool is_depthstencil(core::gfx::format format) { return (has_depth(format) || has_stencil(format)); }
} // namespace core::gfx