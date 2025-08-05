#pragma once
#include "core/gfx/types.hpp"
#include "iwgpu.hpp"

namespace core::gfx::conversion {
constexpr wgpu::ShaderStage to_wgpu(core::gfx::shader_stage stage) noexcept {
	switch(stage) {
	case core::gfx::shader_stage::vertex:
		return wgpu::ShaderStage::Vertex;
	case core::gfx::shader_stage::fragment:
		return wgpu::ShaderStage::Fragment;
	case core::gfx::shader_stage::compute:
		return wgpu::ShaderStage::Compute;
	default:
		return wgpu::ShaderStage::None;
	}
}
constexpr core::gfx::shader_stage to_gfx(wgpu::ShaderStage stage) noexcept {
	switch(stage) {
	case wgpu::ShaderStage::Vertex:
		return core::gfx::shader_stage::vertex;
	case wgpu::ShaderStage::Fragment:
		return core::gfx::shader_stage::fragment;
	case wgpu::ShaderStage::Compute:
		return core::gfx::shader_stage::compute;
	default:
		return core::gfx::shader_stage {0};
	}
}

constexpr core::gfx::vertex_input_rate to_gfx(wgpu::VertexStepMode mode) noexcept {
	switch(mode) {
	case wgpu::VertexStepMode::Vertex:
		return core::gfx::vertex_input_rate::vertex;
	case wgpu::VertexStepMode::Instance:
		return core::gfx::vertex_input_rate::instance;
	default:
		return core::gfx::vertex_input_rate {0};
	}
}

constexpr wgpu::VertexStepMode to_wgpu(core::gfx::vertex_input_rate rate) noexcept {
	switch(rate) {
	case core::gfx::vertex_input_rate::vertex:
		return wgpu::VertexStepMode::Vertex;
	case core::gfx::vertex_input_rate::instance:
		return wgpu::VertexStepMode::Instance;
	default:
		return wgpu::VertexStepMode::Vertex;
	}
}

constexpr core::gfx::format_t to_gfx(wgpu::VertexFormat format) noexcept {
	switch(format) {
	case wgpu::VertexFormat::Float32:
		return core::gfx::format_t::r32_sfloat;
	case wgpu::VertexFormat::Float32x2:
		return core::gfx::format_t::r32g32_sfloat;
	case wgpu::VertexFormat::Float32x3:
		return core::gfx::format_t::r32g32b32_sfloat;
	case wgpu::VertexFormat::Float32x4:
		return core::gfx::format_t::r32g32b32a32_sfloat;
	case wgpu::VertexFormat::Uint32:
		return core::gfx::format_t::r32_uint;
	case wgpu::VertexFormat::Uint32x2:
		return core::gfx::format_t::r32g32_uint;
	case wgpu::VertexFormat::Uint32x3:
		return core::gfx::format_t::r32g32b32_uint;
	case wgpu::VertexFormat::Uint32x4:
		return core::gfx::format_t::r32g32b32a32_uint;
	case wgpu::VertexFormat::Sint32:
		return core::gfx::format_t::r32_sint;
	case wgpu::VertexFormat::Sint32x2:
		return core::gfx::format_t::r32g32_sint;
	case wgpu::VertexFormat::Sint32x3:
		return core::gfx::format_t::r32g32b32_sint;
	case wgpu::VertexFormat::Sint32x4:
		return core::gfx::format_t::r32g32b32a32_sint;
	case wgpu::VertexFormat::Sint16x2:
		return core::gfx::format_t::r16g16_sint;
	case wgpu::VertexFormat::Sint16x4:
		return core::gfx::format_t::r16g16b16a16_sint;
	case wgpu::VertexFormat::Sint8x2:
		return core::gfx::format_t::r8g8_sint;
	case wgpu::VertexFormat::Sint8x4:
		return core::gfx::format_t::r8g8b8a8_sint;
	case wgpu::VertexFormat::Uint16x2:
		return core::gfx::format_t::r16g16_uint;
	case wgpu::VertexFormat::Uint16x4:
		return core::gfx::format_t::r16g16b16a16_uint;
	case wgpu::VertexFormat::Uint8x2:
		return core::gfx::format_t::r8g8_uint;
	case wgpu::VertexFormat::Uint8x4:
		return core::gfx::format_t::r8g8b8a8_uint;
	case wgpu::VertexFormat::Unorm8x2:
		return core::gfx::format_t::r8g8_unorm;
	case wgpu::VertexFormat::Unorm8x4:
		return core::gfx::format_t::r8g8b8a8_unorm;
	case wgpu::VertexFormat::Unorm10_10_10_2:
		return core::gfx::format_t::a2r10g10b10_unorm_pack32;
	case wgpu::VertexFormat::Snorm8x2:
		return core::gfx::format_t::r8g8_snorm;
	case wgpu::VertexFormat::Snorm8x4:
		return core::gfx::format_t::r8g8b8a8_snorm;
	case wgpu::VertexFormat::Snorm16x2:
		return core::gfx::format_t::r16g16_snorm;
	case wgpu::VertexFormat::Snorm16x4:
		return core::gfx::format_t::r16g16b16a16_snorm;
	case wgpu::VertexFormat::Unorm16x2:
		return core::gfx::format_t::r16g16_unorm;
	case wgpu::VertexFormat::Unorm16x4:
		return core::gfx::format_t::r16g16b16a16_unorm;
	default:
		return core::gfx::format_t::undefined;
	}
}

constexpr wgpu::VertexFormat to_wgpu(core::gfx::format_t format) noexcept {
	switch(format) {
	case core::gfx::format_t::r32_sfloat:
		return wgpu::VertexFormat::Float32;
	case core::gfx::format_t::r32g32_sfloat:
		return wgpu::VertexFormat::Float32x2;
	case core::gfx::format_t::r32g32b32_sfloat:
		return wgpu::VertexFormat::Float32x3;
	case core::gfx::format_t::r32g32b32a32_sfloat:
		return wgpu::VertexFormat::Float32x4;
	case core::gfx::format_t::r32_uint:
		return wgpu::VertexFormat::Uint32;
	case core::gfx::format_t::r32g32_uint:
		return wgpu::VertexFormat::Uint32x2;
	case core::gfx::format_t::r32g32b32_uint:
		return wgpu::VertexFormat::Uint32x3;
	case core::gfx::format_t::r32g32b32a32_uint:
		return wgpu::VertexFormat::Uint32x4;
	case core::gfx::format_t::r32_sint:
		return wgpu::VertexFormat::Sint32;
	case core::gfx::format_t::r32g32_sint:
		return wgpu::VertexFormat::Sint32x2;
	case core::gfx::format_t::r32g32b32_sint:
		return wgpu::VertexFormat::Sint32x3;
	case core::gfx::format_t::r32g32b32a32_sint:
		return wgpu::VertexFormat::Sint32x4;
	case core::gfx::format_t::r16g16_sint:
		return wgpu::VertexFormat::Sint16x2;
	case core::gfx::format_t::r16g16b16a16_sint:
		return wgpu::VertexFormat::Sint16x4;
	case core::gfx::format_t::r8g8_sint:
		return wgpu::VertexFormat::Sint8x2;
	case core::gfx::format_t::r8g8b8a8_sint:
		return wgpu::VertexFormat::Sint8x4;
	case core::gfx::format_t::r16g16_uint:
		return wgpu::VertexFormat::Uint16x2;
	case core::gfx::format_t::r16g16b16a16_uint:
		return wgpu::VertexFormat::Uint16x4;
	case core::gfx::format_t::r8g8_uint:
		return wgpu::VertexFormat::Uint8x2;
	case core::gfx::format_t::r8g8b8a8_uint:
		return wgpu::VertexFormat::Uint8x4;
	case core::gfx::format_t::r8g8_unorm:
		return wgpu::VertexFormat::Unorm8x2;
	case core::gfx::format_t::r8g8b8a8_unorm:
		return wgpu::VertexFormat::Unorm8x4;
	case core::gfx::format_t::a2r10g10b10_unorm_pack32:
		return wgpu::VertexFormat::Unorm10_10_10_2;
	case core::gfx::format_t::r8g8_snorm:
		return wgpu::VertexFormat::Snorm8x2;
	case core::gfx::format_t::r8g8b8a8_snorm:
		return wgpu::VertexFormat::Snorm8x4;
	case core::gfx::format_t::r16g16_snorm:
		return wgpu::VertexFormat::Snorm16x2;
	case core::gfx::format_t::r16g16b16a16_snorm:
		return wgpu::VertexFormat::Snorm16x4;
	case core::gfx::format_t::r16g16_unorm:
		return wgpu::VertexFormat::Unorm16x2;
	case core::gfx::format_t::r16g16b16a16_unorm:
		return wgpu::VertexFormat::Unorm16x4;
	default:
		return {};
	}
}
}	 // namespace core::gfx::conversion
