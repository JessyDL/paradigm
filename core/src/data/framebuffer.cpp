#include "data/framebuffer.h"
#include "meta/texture.h"
#include "resource/resource.hpp"
#include "vk/conversion.h"
#include "gfx/sampler.h"

using namespace psl;
using namespace core::data;
using namespace core::gfx;

framebuffer::framebuffer(core::resource::cache& cache, const core::resource::metadata& metaData,
						 psl::meta::file* metaFile, uint32_t width, uint32_t height, uint32_t layers) noexcept
	: m_Width(width), m_Height(height), m_Cache(&cache), m_Count(std::max(layers, 1u))
{}

//framebuffer::framebuffer(const framebuffer& other, const UID& uid, core::resource::cache& cache)
//	: m_Width(other.m_Width), m_Height(other.m_Height), m_Cache(other.m_Cache), m_Count(other.m_Count),
//	  m_Sampler(other.m_Sampler), m_Attachments(other.m_Attachments)
//{}

const UID& framebuffer::add(uint32_t width, uint32_t height, uint32_t layerCount,
							vk::ImageUsageFlags usage, vk::ClearValue clearValue, vk::AttachmentDescription descr)
{
	vk::ImageAspectFlags aspectMask;

	if(usage & vk::ImageUsageFlagBits::eColorAttachment)
	{
		aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	// Depth (and/or stencil) attachment
	if(usage & vk::ImageUsageFlagBits::eDepthStencilAttachment)
	{
		if(utility::vulkan::has_depth(descr.format))
		{
			aspectMask = vk::ImageAspectFlagBits::eDepth;
		}
		if(utility::vulkan::has_stencil(descr.format))
		{
			aspectMask = aspectMask | vk::ImageAspectFlagBits::eStencil;
		}
	}
	auto [UID, texture] = m_Cache->library().create<core::meta::texture>();
	texture.width(width);
	texture.height(height);
	texture.depth(layerCount);
	texture.aspect_mask(conversion::to_image_aspect(aspectMask));
	texture.format(conversion::to_format(descr.format));
	texture.image_type(core::gfx::image_type::planar_2D);
	texture.mip_levels(1);
	texture.usage(conversion::to_image_usage(usage));

	m_Attachments.value.emplace_back(UID, clearValue, descr);

	return UID;
}

bool framebuffer::remove(const UID& uid)
{
	auto it = std::find_if(std::begin(m_Attachments.value), std::end(m_Attachments.value),
						   [&uid](const attachment& att) { return att.texture() == uid; });
	if(it == std::end(m_Attachments.value)) return false;

	m_Attachments.value.erase(it);
	return true;
}

void framebuffer::set(core::resource::handle<core::gfx::sampler> sampler) { m_Sampler.value = sampler; }


const std::vector<framebuffer::attachment>& framebuffer::attachments() const { return m_Attachments.value; }
uint32_t framebuffer::framebuffers() const { return m_Count.value; }
std::optional<UID> framebuffer::sampler() const
{
	return ((m_Sampler.value != UID::invalid_uid) ? m_Sampler.value : std::optional<UID>{});
}
uint32_t framebuffer::width() const { return m_Width.value; }
uint32_t framebuffer::height() const { return m_Height.value; }


framebuffer::attachment::attachment(const UID& texture, const vk::ClearValue& clear_col,
									vk::AttachmentDescription descr, bool shared)
	: m_Texture(texture), m_ClearValue(clear_col), m_Description(descr), m_Shared(shared)
{}
const UID& framebuffer::attachment::texture() const { return m_Texture.value; }

const vk::ClearValue& framebuffer::attachment::clear_value() const { return m_ClearValue.value; }
bool framebuffer::attachment::shared() const { return m_Shared.value; }

vk::AttachmentDescription framebuffer::attachment::vkDescription() const { return m_Description.value.vkDescription(); }
framebuffer::attachment::description::description(vk::AttachmentDescription descr)
	: m_SampleCountFlags(descr.samples), m_LoadOp(descr.loadOp), m_StoreOp(descr.storeOp),
	  m_StencilLoadOp(descr.stencilLoadOp), m_StencilStoreOp(descr.stencilStoreOp),
	  m_InitialLayout(descr.initialLayout), m_FinalLayout(descr.finalLayout), m_Format(descr.format)
{}
vk::AttachmentDescription framebuffer::attachment::description::vkDescription() const
{
	vk::AttachmentDescription descr;
	descr.samples		 = m_SampleCountFlags.value;
	descr.loadOp		 = m_LoadOp.value;
	descr.storeOp		 = m_StoreOp.value;
	descr.stencilLoadOp  = m_StencilLoadOp.value;
	descr.stencilStoreOp = m_StencilStoreOp.value;
	descr.initialLayout  = m_InitialLayout.value;
	descr.finalLayout	= m_FinalLayout.value;
	descr.format		 = m_Format.value;
	return descr;
}
