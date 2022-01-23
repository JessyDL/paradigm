#include "gles/framebuffer.hpp"
#include "gles/igles.hpp"

#include "data/framebuffer.hpp"
#include "gles/sampler.hpp"
#include "gles/texture.hpp"
#include "meta/texture.hpp"
#include "resource/resource.hpp"
using namespace core;
using namespace core::igles;
using namespace core::resource;

// todo support renderbuffer storage

framebuffer_t::framebuffer_t(core::resource::cache_t& cache,
						 const core::resource::metadata& metaData,
						 psl::meta::file* metaFile,
						 handle<core::data::framebuffer_t> data) :
	m_Data(data)
{
	m_Framebuffers.resize(m_Data->framebuffers());

	if(auto sampler = data->sampler(); sampler)
	{
		m_Sampler = cache.find<core::igles::sampler_t>(sampler.value());
		if(!m_Sampler)
		{
			core::ivk::log->error("could not load sampler for framebuffer {0}", metaData.uid);
			assert(false);
		}
	}
	else
	{
		core::ivk::log->error("could not load sampler for framebuffer {0}", metaData.uid);
		assert(false);
	}

	glGenFramebuffers(m_Framebuffers.size(), m_Framebuffers.data());
	size_t index = 0u;
	for(const auto& attach : data->attachments())
	{
		auto texture = cache.find<core::igles::texture_t>(attach.texture());
		if(texture.state() != core::resource::status::loaded)
			texture = cache.create_using<core::igles::texture_t>(attach.texture());

		assert_debug_break(!attach.shared());

		auto count				   = attach.shared() ? 1u : m_Framebuffers.size();
		binding& binding		   = m_Bindings.emplace_back();
		binding.index			   = index;
		binding.description.format = texture.meta()->format();

		binding.attachments.push_back(texture);

		index += m_Framebuffers.size();
	}

	glGetError();
	for(auto i = 0u; i < m_Framebuffers.size(); ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[i]);
		size_t index = 0;
		for(const auto& binding : m_Bindings)
		{
			auto format = (core::gfx::is_depthstencil(binding.description.format)) ? GL_DEPTH_STENCIL_ATTACHMENT
						  : (core::gfx::has_stencil(binding.description.format))   ? GL_STENCIL_ATTACHMENT
						  : (core::gfx::has_depth(binding.description.format))	   ? GL_DEPTH_ATTACHMENT
																				   : GL_COLOR_ATTACHMENT0 + i;
			glFramebufferTexture2D(GL_FRAMEBUFFER, format, GL_TEXTURE_2D, binding.attachments[i]->id(), 0);
		}
	}
	auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		break;
	default:
		core::igles::log->error("Framebuffer generated error: {}", status);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glGetError();
}

framebuffer_t::~framebuffer_t() { glDeleteFramebuffers(m_Framebuffers.size(), m_Framebuffers.data()); }

std::vector<framebuffer_t::texture_handle> framebuffer_t::attachments(uint32_t index) const noexcept
{
	if(index >= m_Bindings.size()) return {};

	std::vector<framebuffer_t::texture_handle> res;
	std::transform(std::begin(m_Bindings), std::end(m_Bindings), std::back_inserter(res), [index](const auto& binding) {
		return (binding.attachments.size() > 1) ? binding.attachments[index] : binding.attachments[0];
	});
	return res;
}

std::vector<framebuffer_t::texture_handle> framebuffer_t::color_attachments(uint32_t index) const noexcept
{
	if(index >= m_Bindings.size()) return {};

	std::vector<framebuffer_t::texture_handle> res;
	auto bindings {m_Bindings};
	auto end = std::remove_if(std::begin(bindings), std::end(bindings), [](const framebuffer_t::binding& binding) {
		return gfx::has_depth(binding.description.format) || gfx::has_stencil(binding.description.format);
	});
	std::transform(std::begin(bindings), end, std::back_inserter(res), [index](const auto& binding) {
		return (binding.attachments.size() > 1) ? binding.attachments[index] : binding.attachments[0];
	});

	return res;
}
core::resource::handle<core::igles::sampler_t> framebuffer_t::sampler() const noexcept { return m_Sampler; }

core::resource::handle<core::data::framebuffer_t> framebuffer_t::data() const noexcept { return m_Data; }

const std::vector<unsigned int>& framebuffer_t::framebuffers() const noexcept { return m_Framebuffers; }