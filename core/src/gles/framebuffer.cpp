#include "gles/framebuffer.h"
#include "glad/glad.h"

#include "gles/texture.h"
#include "gles/sampler.h"
#include "meta/texture.h"
#include "resource/resource.hpp"
#include "data/framebuffer.h"
using namespace core;
using namespace core::igles;
using namespace core::resource;

// todo support renderbuffer storage

framebuffer::framebuffer(core::resource::cache& cache, const core::resource::metadata& metaData,
						 psl::meta::file* metaFile, handle<core::data::framebuffer> data)
	: m_Data(data)
{
	m_Framebuffers.resize(m_Data->framebuffers());
	m_Textures.reserve(m_Framebuffers.size() * m_Data->attachments().size());

	glGenFramebuffers(m_Framebuffers.size(), m_Framebuffers.data());

	size_t index = 0u;
	for(const auto& attach : data->attachments())
	{
		auto texture = cache.find<core::igles::texture>(attach.texture());
		if(texture.state() != core::resource::state::loaded)
			texture = cache.create_using<core::igles::texture>(attach.texture());

		assert_debug_break(!attach.shared());

		auto count				   = attach.shared() ? 1u : m_Framebuffers.size();
		binding& binding		   = m_Bindings.emplace_back();
		binding.index			   = index;
		binding.description.format = texture.meta()->format();

		m_Textures.push_back(texture);
		auto& attachment   = binding.attachments.emplace_back();
		attachment.texture = texture->id();

		index += m_Framebuffers.size();
	}

	for(auto i = 0u; i < m_Framebuffers.size(); ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[i]);
		size_t index = 0;
		for(const auto& binding : m_Bindings)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER,
								   (core::gfx::is_depthstencil(binding.description.format))
									   ? GL_DEPTH_STENCIL_ATTACHMENT
									   : (core::gfx::has_depth(binding.description.format)) ? GL_DEPTH_ATTACHMENT
																							: GL_COLOR_ATTACHMENT0 + i,
								   GL_TEXTURE_2D,
								   binding.attachments[i].texture,
								   0);
		}
	}
}

framebuffer::~framebuffer() 
{ 
	glDeleteFramebuffers(m_Framebuffers.size(), m_Framebuffers.data()); 
}