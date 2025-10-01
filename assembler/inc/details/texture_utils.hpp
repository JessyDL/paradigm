#pragma once

#include "core/gles/conversion.hpp"
#include "psl/array_view.hpp"
#include <cstdint>

namespace utility::dds {
constexpr uint32_t identifier {0x20534444};


enum class pixelformat_flags : uint32_t {
	alphapixels = 0x1,
	alpha		= 0x2,
	fourcc		= 0x4,
	rgb			= 0x40,
	yuv			= 0x200,
	luminance	= 0x20000
};
struct pixelformat {
	uint32_t dwSize;
	pixelformat_flags dwFlags;
	uint32_t dwFourCC;
	uint32_t dwRGBBitCount;
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;
};

enum class dds_flags : uint32_t {
	caps		= 0x1,
	height		= 0x2,
	width		= 0x3,
	pitch		= 0x8,
	pixelformat = 0x1000,
	mipmapcount = 0x20000,
	linearsize	= 0x80000,
	depth		= 0x800000
};

enum class dds_dxt_flag : uint32_t {
	DXT1 = '1TXD',
	DXT2 = '2TXD',
	DXT3 = '3TXD',
	DXT4 = '4TXD',
	DXT5 = '5TXD',

};

inline bool operator&(dds_flags bit0, dds_flags bit1) {
	using type = core::gfx::enum_flag<dds_flags>;
	return (type(bit0) & bit1) == bit1;
}

enum class DXGI_FORMAT : uint32_t {
	UNKNOWN					   = 0,
	R32G32B32A32_TYPELESS	   = 1,
	R32G32B32A32_FLOAT		   = 2,
	R32G32B32A32_UINT		   = 3,
	R32G32B32A32_SINT		   = 4,
	R32G32B32_TYPELESS		   = 5,
	R32G32B32_FLOAT			   = 6,
	R32G32B32_UINT			   = 7,
	R32G32B32_SINT			   = 8,
	R16G16B16A16_TYPELESS	   = 9,
	R16G16B16A16_FLOAT		   = 10,
	R16G16B16A16_UNORM		   = 11,
	R16G16B16A16_UINT		   = 12,
	R16G16B16A16_SNORM		   = 13,
	R16G16B16A16_SINT		   = 14,
	R32G32_TYPELESS			   = 15,
	R32G32_FLOAT			   = 16,
	R32G32_UINT				   = 17,
	R32G32_SINT				   = 18,
	R32G8X24_TYPELESS		   = 19,
	D32_FLOAT_S8X24_UINT	   = 20,
	R32_FLOAT_X8X24_TYPELESS   = 21,
	X32_TYPELESS_G8X24_UINT	   = 22,
	R10G10B10A2_TYPELESS	   = 23,
	R10G10B10A2_UNORM		   = 24,
	R10G10B10A2_UINT		   = 25,
	R11G11B10_FLOAT			   = 26,
	R8G8B8A8_TYPELESS		   = 27,
	R8G8B8A8_UNORM			   = 28,
	R8G8B8A8_UNORM_SRGB		   = 29,
	R8G8B8A8_UINT			   = 30,
	R8G8B8A8_SNORM			   = 31,
	R8G8B8A8_SINT			   = 32,
	R16G16_TYPELESS			   = 33,
	R16G16_FLOAT			   = 34,
	R16G16_UNORM			   = 35,
	R16G16_UINT				   = 36,
	R16G16_SNORM			   = 37,
	R16G16_SINT				   = 38,
	R32_TYPELESS			   = 39,
	D32_FLOAT				   = 40,
	R32_FLOAT				   = 41,
	R32_UINT				   = 42,
	R32_SINT				   = 43,
	R24G8_TYPELESS			   = 44,
	D24_UNORM_S8_UINT		   = 45,
	R24_UNORM_X8_TYPELESS	   = 46,
	X24_TYPELESS_G8_UINT	   = 47,
	R8G8_TYPELESS			   = 48,
	R8G8_UNORM				   = 49,
	R8G8_UINT				   = 50,
	R8G8_SNORM				   = 51,
	R8G8_SINT				   = 52,
	R16_TYPELESS			   = 53,
	R16_FLOAT				   = 54,
	D16_UNORM				   = 55,
	R16_UNORM				   = 56,
	R16_UINT				   = 57,
	R16_SNORM				   = 58,
	R16_SINT				   = 59,
	R8_TYPELESS				   = 60,
	R8_UNORM				   = 61,
	R8_UINT					   = 62,
	R8_SNORM				   = 63,
	R8_SINT					   = 64,
	A8_UNORM				   = 65,
	R1_UNORM				   = 66,
	R9G9B9E5_SHAREDEXP		   = 67,
	R8G8_B8G8_UNORM			   = 68,
	G8R8_G8B8_UNORM			   = 69,
	BC1_TYPELESS			   = 70,
	BC1_UNORM				   = 71,
	BC1_UNORM_SRGB			   = 72,
	BC2_TYPELESS			   = 73,
	BC2_UNORM				   = 74,
	BC2_UNORM_SRGB			   = 75,
	BC3_TYPELESS			   = 76,
	BC3_UNORM				   = 77,
	BC3_UNORM_SRGB			   = 78,
	BC4_TYPELESS			   = 79,
	BC4_UNORM				   = 80,
	BC4_SNORM				   = 81,
	BC5_TYPELESS			   = 82,
	BC5_UNORM				   = 83,
	BC5_SNORM				   = 84,
	B5G6R5_UNORM			   = 85,
	B5G5R5A1_UNORM			   = 86,
	B8G8R8A8_UNORM			   = 87,
	B8G8R8X8_UNORM			   = 88,
	R10G10B10_XR_BIAS_A2_UNORM = 89,
	B8G8R8A8_TYPELESS		   = 90,
	B8G8R8A8_UNORM_SRGB		   = 91,
	B8G8R8X8_TYPELESS		   = 92,
	B8G8R8X8_UNORM_SRGB		   = 93,
	BC6H_TYPELESS			   = 94,
	BC6H_UF16				   = 95,
	BC6H_SF16				   = 96,
	BC7_TYPELESS			   = 97,
	BC7_UNORM				   = 98,
	BC7_UNORM_SRGB			   = 99,
	AYUV					   = 100,
	Y410					   = 101,
	Y416					   = 102,
	NV12					   = 103,
	P010					   = 104,
	P016					   = 105,
	OPAQUE_420				   = 106,
	YUY2					   = 107,
	Y210					   = 108,
	Y216					   = 109,
	NV11					   = 110,
	AI44					   = 111,
	IA44					   = 112,
	P8						   = 113,
	A8P8					   = 114,
	B4G4R4A4_UNORM			   = 115,

	P208 = 130,
	V208 = 131,
	V408 = 132,


	FORCE_UINT = 0xffffffff
};

struct header {
	uint32_t dwSize;
	dds_flags dwFlags;
	uint32_t dwHeight;
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth;
	uint32_t dwMipMapCount;
	uint32_t dwReserved1[11];
	pixelformat ddspf;
	uint32_t dwCaps;
	uint32_t dwCaps2;
	uint32_t dwCaps3;
	uint32_t dwCaps4;
	uint32_t dwReserved2;


	DXGI_FORMAT dxgiFormat;
	uint32_t resourceDimension;
	uint32_t miscFlag;
	uint32_t arraySize;
	uint32_t miscFlags2;
};

constexpr inline size_t header_size() noexcept {
	return sizeof(header) + sizeof(identifier);
}

inline bool is_dds(psl::array_view<std::byte> data) noexcept {
	if(data.size() > sizeof(identifier)) {
		return memcmp(data.data(), (void*)&identifier, sizeof(identifier)) == 0;
	}
	return false;
}

inline header decode(psl::array_view<std::byte> data) {
	if(!is_dds(data))
		return {0};
	header res {};
	data = psl::array_view<std::byte> {std::next(std::begin(data), sizeof(identifier)),
									   std::size(data) - sizeof(identifier)};
	memcpy(&res, data.data(), sizeof(header));

	if(res.ddspf.dwFourCC != (uint32_t)('D') + (uint32_t)('X') + (uint32_t)('1') + (uint32_t)('0')) {
		res.dxgiFormat		  = (DXGI_FORMAT)0;
		res.resourceDimension = 0;
		res.miscFlag		  = 0;
		res.arraySize		  = 0;
		res.miscFlags2		  = 0;
	}

	return res;
}

inline core::gfx::format_t to_format(header header) noexcept {
	using namespace core::gfx;
	static std::unordered_map<DXGI_FORMAT, core::gfx::format_t> const lookup {
	  {DXGI_FORMAT::R32G32B32A32_TYPELESS, core::gfx::format_t::r32g32b32a32_uint},
	  {DXGI_FORMAT::R32G32B32A32_FLOAT, core::gfx::format_t::r32g32b32a32_sfloat},
	  {DXGI_FORMAT::R32G32B32A32_UINT, core::gfx::format_t::r32g32b32a32_uint},
	  {DXGI_FORMAT::R32G32B32A32_SINT, core::gfx::format_t::r32g32b32a32_sint},
	  {DXGI_FORMAT::R32G32B32_TYPELESS, core::gfx::format_t::r32g32b32_uint},
	  {DXGI_FORMAT::R32G32B32_FLOAT, core::gfx::format_t::r32g32b32_sfloat},
	  {DXGI_FORMAT::R32G32B32_UINT, core::gfx::format_t::r32g32b32_uint},
	  {DXGI_FORMAT::R32G32B32_SINT, core::gfx::format_t::r32g32b32_sint},
	  {DXGI_FORMAT::R16G16B16A16_TYPELESS, core::gfx::format_t::r16g16b16a16_unorm},
	  {DXGI_FORMAT::R16G16B16A16_FLOAT, core::gfx::format_t::r16g16b16a16_sfloat},
	  {DXGI_FORMAT::R16G16B16A16_UNORM, core::gfx::format_t::r16g16b16a16_unorm},
	  {DXGI_FORMAT::R16G16B16A16_UINT, core::gfx::format_t::r16g16b16a16_uint},
	  {DXGI_FORMAT::R16G16B16A16_SNORM, core::gfx::format_t::r16g16b16a16_snorm},
	  {DXGI_FORMAT::R16G16B16A16_SINT, core::gfx::format_t::r16g16b16a16_sint},
	  {DXGI_FORMAT::R32G32_TYPELESS, core::gfx::format_t::r32g32_uint},
	  {DXGI_FORMAT::R32G32_FLOAT, core::gfx::format_t::r32g32_sfloat},
	  {DXGI_FORMAT::R32G32_UINT, core::gfx::format_t::r32g32_uint},
	  {DXGI_FORMAT::R32G32_SINT, core::gfx::format_t::r32g32_sint},
	  {DXGI_FORMAT::R32G8X24_TYPELESS, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::D32_FLOAT_S8X24_UINT, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::R32_FLOAT_X8X24_TYPELESS, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::X32_TYPELESS_G8X24_UINT, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::R10G10B10A2_TYPELESS, core::gfx::format_t::a2b10g10r10_unorm_pack32},
	  {DXGI_FORMAT::R10G10B10A2_UNORM, core::gfx::format_t::a2b10g10r10_unorm_pack32},
	  {DXGI_FORMAT::R10G10B10A2_UINT, core::gfx::format_t::a2b10g10r10_uint_pack32},
	  {DXGI_FORMAT::R11G11B10_FLOAT, core::gfx::format_t::b10g11r11_ufloat_pack32},
	  {DXGI_FORMAT::R8G8B8A8_TYPELESS, core::gfx::format_t::r8g8b8a8_unorm},
	  {DXGI_FORMAT::R8G8B8A8_UNORM, core::gfx::format_t::r8g8b8a8_unorm},
	  {DXGI_FORMAT::R8G8B8A8_UNORM_SRGB, core::gfx::format_t::r8g8b8a8_srgb},
	  {DXGI_FORMAT::R8G8B8A8_UINT, core::gfx::format_t::r8g8b8a8_uint},
	  {DXGI_FORMAT::R8G8B8A8_SNORM, core::gfx::format_t::r8g8b8a8_snorm},
	  {DXGI_FORMAT::R8G8B8A8_SINT, core::gfx::format_t::r8g8b8a8_sint},
	  {DXGI_FORMAT::R16G16_TYPELESS, core::gfx::format_t::r16g16_unorm},
	  {DXGI_FORMAT::R16G16_FLOAT, core::gfx::format_t::r16g16_sfloat},
	  {DXGI_FORMAT::R16G16_UNORM, core::gfx::format_t::r16g16_unorm},
	  {DXGI_FORMAT::R16G16_UINT, core::gfx::format_t::r16g16_uint},
	  {DXGI_FORMAT::R16G16_SNORM, core::gfx::format_t::r16g16_snorm},
	  {DXGI_FORMAT::R16G16_SINT, core::gfx::format_t::r16g16_sint},
	  {DXGI_FORMAT::R32_TYPELESS, core::gfx::format_t::r32_uint},
	  {DXGI_FORMAT::D32_FLOAT, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::R32_FLOAT, core::gfx::format_t::r32_sfloat},
	  {DXGI_FORMAT::R32_UINT, core::gfx::format_t::r32_uint},
	  {DXGI_FORMAT::R32_SINT, core::gfx::format_t::r32_sint},
	  {DXGI_FORMAT::R24G8_TYPELESS, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::D24_UNORM_S8_UINT, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::R24_UNORM_X8_TYPELESS, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::X24_TYPELESS_G8_UINT, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::R8G8_TYPELESS, core::gfx::format_t::r8g8_unorm},
	  {DXGI_FORMAT::R8G8_UNORM, core::gfx::format_t::r8g8_unorm},
	  {DXGI_FORMAT::R8G8_UINT, core::gfx::format_t::r8g8_uint},
	  {DXGI_FORMAT::R8G8_SNORM, core::gfx::format_t::r8g8_snorm},
	  {DXGI_FORMAT::R8G8_SINT, core::gfx::format_t::r8g8_sint},
	  {DXGI_FORMAT::R16_TYPELESS, core::gfx::format_t::r16_unorm},
	  {DXGI_FORMAT::R16_FLOAT, core::gfx::format_t::r16_sfloat},
	  {DXGI_FORMAT::D16_UNORM, core::gfx::format_t::undefined},
	  {DXGI_FORMAT::R16_UNORM, core::gfx::format_t::r16_unorm},
	  {DXGI_FORMAT::R16_UINT, core::gfx::format_t::r16_uint},
	  {DXGI_FORMAT::R16_SNORM, core::gfx::format_t::r16_snorm},
	  {DXGI_FORMAT::R16_SINT, core::gfx::format_t::r16_sint},
	  {DXGI_FORMAT::R8_TYPELESS, core::gfx::format_t::r8_unorm},
	  {DXGI_FORMAT::R8_UNORM, core::gfx::format_t::r8_unorm},
	  {DXGI_FORMAT::R8_UINT, core::gfx::format_t::r8_uint},
	  {DXGI_FORMAT::R8_SNORM, core::gfx::format_t::r8_snorm},
	  {DXGI_FORMAT::R8_SINT, core::gfx::format_t::r8_sint},
	  {DXGI_FORMAT::A8_UNORM, core::gfx::format_t::r8_unorm},
	  {DXGI_FORMAT::R9G9B9E5_SHAREDEXP, core::gfx::format_t::e5b9g9r9_ufloat_pack32},
	  //{DXGI_FORMAT::R8G8_B8G8_UNORM, core::gfx::format_t::b8g8r8g8_422_unorm_khr},
	  //{DXGI_FORMAT::G8R8_G8B8_UNORM, core::gfx::format_t::g8b8g8r8_422_unorm_khr},
	  {DXGI_FORMAT::BC1_TYPELESS, core::gfx::format_t::bc1_rgba_unorm_block},
	  {DXGI_FORMAT::BC1_UNORM, core::gfx::format_t::bc1_rgba_unorm_block},
	  {DXGI_FORMAT::BC1_UNORM_SRGB, core::gfx::format_t::bc1_rgba_srgb_block},
	  {DXGI_FORMAT::BC2_TYPELESS, core::gfx::format_t::bc2_unorm_block},
	  {DXGI_FORMAT::BC2_UNORM, core::gfx::format_t::bc2_unorm_block},
	  {DXGI_FORMAT::BC2_UNORM_SRGB, core::gfx::format_t::bc2_srgb_block},
	  {DXGI_FORMAT::BC3_TYPELESS, core::gfx::format_t::bc3_unorm_block},
	  {DXGI_FORMAT::BC3_UNORM, core::gfx::format_t::bc3_unorm_block},
	  {DXGI_FORMAT::BC3_UNORM_SRGB, core::gfx::format_t::bc3_srgb_block},
	  {DXGI_FORMAT::BC4_TYPELESS, core::gfx::format_t::bc4_unorm_block},
	  {DXGI_FORMAT::BC4_UNORM, core::gfx::format_t::bc4_unorm_block},
	  {DXGI_FORMAT::BC4_SNORM, core::gfx::format_t::bc4_snorm_block},
	  {DXGI_FORMAT::BC5_TYPELESS, core::gfx::format_t::bc5_unorm_block},
	  {DXGI_FORMAT::BC5_UNORM, core::gfx::format_t::bc5_unorm_block},
	  {DXGI_FORMAT::BC5_SNORM, core::gfx::format_t::bc5_snorm_block},
	  {DXGI_FORMAT::B5G6R5_UNORM, core::gfx::format_t::r5g6b5_unorm_pack16},
	  {DXGI_FORMAT::B5G5R5A1_UNORM, core::gfx::format_t::a1r5g5b5_unorm_pack16},
	  {DXGI_FORMAT::B8G8R8A8_UNORM, core::gfx::format_t::b8g8r8a8_unorm},
	  {DXGI_FORMAT::B8G8R8X8_UNORM, core::gfx::format_t::b8g8r8a8_unorm},
	  {DXGI_FORMAT::B8G8R8A8_TYPELESS, core::gfx::format_t::b8g8r8a8_unorm},
	  {DXGI_FORMAT::B8G8R8A8_UNORM_SRGB, core::gfx::format_t::b8g8r8a8_srgb},
	  {DXGI_FORMAT::B8G8R8X8_TYPELESS, core::gfx::format_t::b8g8r8a8_unorm},
	  {DXGI_FORMAT::B8G8R8X8_UNORM_SRGB, core::gfx::format_t::b8g8r8a8_srgb},
	  {DXGI_FORMAT::BC6H_TYPELESS, core::gfx::format_t::bc6h_ufloat_block},
	  {DXGI_FORMAT::BC6H_UF16, core::gfx::format_t::bc6h_ufloat_block},
	  {DXGI_FORMAT::BC6H_SF16, core::gfx::format_t::bc6h_sfloat_block},
	  {DXGI_FORMAT::BC7_TYPELESS, core::gfx::format_t::bc7_unorm_block},
	  {DXGI_FORMAT::BC7_UNORM, core::gfx::format_t::bc7_unorm_block},
	  {DXGI_FORMAT::B4G4R4A4_UNORM, core::gfx::format_t::b4g4r4a4_unorm_pack16}};
	if(header.ddspf.dwFourCC != (uint32_t)('D') + (uint32_t)('X') + (uint32_t)('1') + (uint32_t)('0')) {
		switch((dds_dxt_flag)header.ddspf.dwFourCC) {
		case dds_dxt_flag::DXT1:
			return core::gfx::format_t::bc1_rgba_unorm_block;
			break;
		case dds_dxt_flag::DXT2:
			return core::gfx::format_t::bc1_rgba_unorm_block;
			break;
		case dds_dxt_flag::DXT3:
			return core::gfx::format_t::bc2_unorm_block;
			break;
		case dds_dxt_flag::DXT4:
			return core::gfx::format_t::bc3_unorm_block;
			break;
		case dds_dxt_flag::DXT5:
			return core::gfx::format_t::bc3_unorm_block;
			break;
		default: {
		}
		}
	} else
		return lookup.at(header.dxgiFormat);

	return core::gfx::format_t::undefined;
}
}	 // namespace utility::dds
namespace utility::ktx {
constexpr psl::static_array<std::byte, 12> identifier {std::byte {0xAB},
													   std::byte {0x4B},
													   std::byte {0x54},
													   std::byte {0x58},
													   std::byte {0x20},
													   std::byte {0x31},
													   std::byte {0x31},
													   std::byte {0xBB},
													   std::byte {0x0D},
													   std::byte {0x0A},
													   std::byte {0x1A},
													   std::byte {0x0A}};

struct header {
	uint32_t endianness;
	uint32_t glType;
	uint32_t glTypeSize;
	uint32_t glFormat;
	uint32_t glInternalFormat;
	uint32_t glBaseInternalFormat;
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t pixelDepth;
	uint32_t numberOfArrayElements;
	uint32_t numberOfFaces;
	uint32_t numberOfMipmapLevels;
	uint32_t bytesOfKeyValueData;
};

constexpr inline size_t header_size() noexcept {
	return sizeof(header) + sizeof(identifier);
}

inline bool is_ktx(psl::array_view<std::byte> data) noexcept {
	if(data.size() > sizeof(identifier)) {
		return memcmp(data.data(), identifier.data(), sizeof(identifier)) == 0;
	}
	return false;
}

inline header decode(psl::array_view<std::byte> data) {
	if(!is_ktx(data))
		return {0};
	header res {};
	data = psl::array_view<std::byte> {std::next(std::begin(data), sizeof(identifier)),
									   std::size(data) - sizeof(identifier)};
	memcpy(&res, data.data(), sizeof(header));
	return res;
}

}	 // namespace utility::ktx
