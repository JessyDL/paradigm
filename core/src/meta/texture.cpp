#include "core/meta/texture.hpp"
#include "core/resource/resource.hpp"

using namespace core::meta;
using namespace psl::serialization;

const uint64_t texture_t::polymorphic_identity {register_polymorphic<texture_t>()};


uint32_t texture_t::width() const noexcept {
	return m_Width.value;
}
void texture_t::width(uint32_t width) {
	m_Width.value = width;
}

uint32_t texture_t::height() const noexcept {
	return m_Height.value;
}
void texture_t::height(uint32_t height) {
	m_Height.value = height;
}

uint32_t texture_t::depth() const noexcept {
	return m_Depth.value;
}
void texture_t::depth(uint32_t depth) {
	m_Depth.value = depth;
}

uint32_t texture_t::mip_levels() const noexcept {
	return m_MipLevels.value;
}
void texture_t::mip_levels(uint32_t mip_levels) {
	m_MipLevels.value = mip_levels;
}

uint32_t texture_t::layers() const noexcept {
	return m_LayerCount.value;
}
void texture_t::layers(uint32_t layers) {
	m_LayerCount.value = layers;
}

core::gfx::format_t texture_t::format() const noexcept {
	return m_Format.value;
}
void texture_t::format(core::gfx::format_t format) {
	m_Format.value = format;
}

core::gfx::image_type texture_t::image_type() const noexcept {
	return m_ImageType.value;
}
void texture_t::image_type(core::gfx::image_type type) {
	m_ImageType.value = type;
}

core::gfx::image_usage texture_t::usage() const noexcept {
	return m_UsageFlags.value;
}
void texture_t::usage(core::gfx::image_usage usage) {
	m_UsageFlags.value = usage;
}

core::gfx::image_aspect texture_t::aspect_mask() const noexcept {
	return m_AspectMask.value;
}
void texture_t::aspect_mask(core::gfx::image_aspect aspect) {
	m_AspectMask.value = aspect;
}

bool texture_t::validate() const noexcept {
	bool valid = true;
	if(m_Format == core::gfx::format_t::undefined || !core::gfx::is_texture_format(m_Format.value)) {
		valid = false;
		LOG_ERROR("undefined format detected in: ", this->ID());
	}

	return valid;
}
