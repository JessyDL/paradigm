#include "stdafx.h"
#include "meta/texture.h"

using namespace core::meta;

const uint64_t texture::polymorphic_identity{serialization::register_polymorphic<texture>()};


uint32_t texture::width() const noexcept { return m_Width.value; }
void texture::width(uint32_t width) { m_Width.value = width; }

uint32_t texture::height() const noexcept { return m_Height.value; }
void texture::height(uint32_t height) { m_Height.value = height; }

uint32_t texture::depth() const noexcept { return m_Depth.value; }
void texture::depth(uint32_t depth) { m_Depth.value = depth; }

uint32_t texture::mip_levels() const noexcept { return m_MipLevels.value; }
void texture::mip_levels(uint32_t mip_levels) { m_MipLevels.value = mip_levels; }

uint32_t texture::layers() const noexcept { return m_LayerCount.value; }
void texture::layers(uint32_t layers) { m_LayerCount.value = layers; }

vk::Format texture::format() const noexcept { return m_Format.value; }
void texture::format(vk::Format format) { m_Format.value = format; }

vk::ImageViewType texture::image_type() const noexcept { return m_ImageType.value; }
void texture::image_type(vk::ImageViewType type) { m_ImageType.value = type; }

vk::ImageUsageFlags texture::usage() const noexcept { return m_UsageFlags.value; }
void texture::usage(vk::ImageUsageFlags usage) { m_UsageFlags.value = usage; }

vk::ImageAspectFlags texture::aspect_mask() const noexcept { return m_AspectMask.value; }
void texture::aspect_mask(vk::ImageAspectFlags aspect) { m_AspectMask.value = aspect; }

bool texture::validate() const noexcept
{
	bool valid = true;
	if(m_Format == vk::Format::eUndefined)
	{
		valid = false;
		LOG_ERROR("undefined format detected in: ", this->ID());
	}

	return valid;
}
