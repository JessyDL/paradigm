#include "data/framebuffer.hpp"
#include "gfx/sampler.hpp"
#include "meta/texture.hpp"
#include "resource/resource.hpp"

using namespace psl;
using namespace core::data;
using namespace core::gfx;

framebuffer_t::framebuffer_t(core::resource::cache_t& cache,
						 const core::resource::metadata& metaData,
						 psl::meta::file* metaFile,
						 uint32_t width,
						 uint32_t height,
						 uint32_t layers) noexcept :
	m_Width(width),
	m_Height(height), m_Cache(&cache), m_Count(std::max(layers, 1u))
{}

// framebuffer_t::framebuffer_t(const framebuffer_t& other, const UID& uid, core::resource::cache_t& cache)
//	: m_Width(other.m_Width), m_Height(other.m_Height), m_Cache(other.m_Cache), m_Count(other.m_Count),
//	  m_Sampler(other.m_Sampler), m_Attachments(other.m_Attachments)
//{}

const UID& framebuffer_t::add(uint32_t width,
							uint32_t height,
							uint32_t layerCount,
							core::gfx::image::usage usage,
							core::gfx::clear_value clearValue,
							core::gfx::attachment descr)
{
	core::gfx::image_aspect aspectMask {};

	if(usage & core::gfx::image::usage::color_attachment)
	{
		aspectMask = core::gfx::image_aspect::color;
	}

	// Depth (and/or stencil) attachment
	if(usage & core::gfx::image::usage::dept_stencil_attachment)
	{
		if(core::gfx::has_depth(descr.format))
		{
			aspectMask = core::gfx::image_aspect::depth;
		}
		if(core::gfx::has_stencil(descr.format))
		{
			aspectMask = aspectMask | core::gfx::image_aspect::stencil;
		}
	}
	auto [UID, texture] = m_Cache->library().create<core::meta::texture_t>();
	texture.width(width);
	texture.height(height);
	texture.depth(layerCount);
	texture.aspect_mask(aspectMask);
	texture.format(descr.format);
	texture.image_type(core::gfx::image_type::planar_2D);
	texture.mip_levels(1);
	texture.usage(usage);

	m_Attachments.value.emplace_back(UID, clearValue, descr);

	return UID;
}

bool framebuffer_t::remove(const UID& uid)
{
	auto it = std::find_if(std::begin(m_Attachments.value),
						   std::end(m_Attachments.value),
						   [&uid](const attachment& att) { return att.texture() == uid; });
	if(it == std::end(m_Attachments.value)) return false;

	m_Attachments.value.erase(it);
	return true;
}

void framebuffer_t::set(core::resource::handle<core::gfx::sampler_t> sampler) { m_Sampler.value = sampler; }


uint32_t framebuffer_t::framebuffers() const { return m_Count.value; }
std::optional<UID> framebuffer_t::sampler() const
{
	return ((m_Sampler.value != UID::invalid_uid) ? m_Sampler.value : std::optional<UID> {});
}
uint32_t framebuffer_t::width() const { return m_Width.value; }
uint32_t framebuffer_t::height() const { return m_Height.value; }


framebuffer_t::attachment::attachment(const UID& texture,
									const core::gfx::clear_value& clear_col,
									core::gfx::attachment descr,
									bool shared) :
	m_Texture(texture),
	m_ClearValue(clear_col), m_Description(descr), m_Shared(shared)
{}
const UID& framebuffer_t::attachment::texture() const { return m_Texture.value; }

const core::gfx::clear_value& framebuffer_t::attachment::clear_value() const { return m_ClearValue.value; }
bool framebuffer_t::attachment::shared() const { return m_Shared.value; }

framebuffer_t::attachment::operator core::gfx::attachment() const noexcept { return m_Description.value; }
framebuffer_t::attachment::description::description(core::gfx::attachment descr) noexcept :
	m_SampleCountFlags(descr.sample_bits), m_LoadOp(descr.image_load), m_StoreOp(descr.image_store),
	m_StencilLoadOp(descr.stencil_load), m_StencilStoreOp(descr.stencil_store), m_InitialLayout(descr.initial),
	m_FinalLayout(descr.final), m_Format(descr.format)
{}
framebuffer_t::attachment::description::operator core::gfx::attachment() const noexcept
{
	core::gfx::attachment descr {};
	descr.sample_bits	= m_SampleCountFlags.value;
	descr.image_load	= m_LoadOp.value;
	descr.image_store	= m_StoreOp.value;
	descr.stencil_load	= m_StencilLoadOp.value;
	descr.stencil_store = m_StencilStoreOp.value;
	descr.initial		= m_InitialLayout.value;
	descr.final			= m_FinalLayout.value;
	descr.format		= m_Format.value;
	return descr;
}
