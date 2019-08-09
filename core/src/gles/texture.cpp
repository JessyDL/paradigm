#include "gles/texture.h"
#include "logging.h"
#include "meta/texture.h"
#include "glad/glad_wgl.h"
#ifdef fseek
#define cached_fseek fseek
#define cached_fclose fclose
#define cached_fwrite fwrite
#define cached_fread fread
#define cached_ftell ftell
#undef fseek
#undef fclose
#undef fwrite
#undef fread
#undef ftell
#endif
#include "gli/gli.hpp"
#ifdef cached_fseek
#define fseek cached_fseek
#define fclose cached_fclose
#define fwrite cached_fwrite
#define fread cached_fread
#define ftell cached_ftell
#endif

using namespace core::resource;
using namespace core::igles;

texture::texture(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile)
	: m_Cache(cache), m_Meta(m_Cache.library().get<core::meta::texture>(metaFile->ID()).value_or(nullptr))
{
	if(!m_Meta)
	{
		core::igles::log->error(
			"texture could not resolve the meta uid: {0}. is the meta file present in the metalibrary?",
			utility::to_string(metaFile->ID()));
		return;
	}

	if(cache.library().is_physical_file(m_Meta->ID()))
	{
		auto result = cache.library().load(m_Meta->ID());
		if(!result) goto fail;
		auto texture  = gli::flip(gli::load(result.value().data(), result.value().size()));
		m_TextureData = new gli::texture(texture);
		switch(m_Meta->image_type())
		{
		case gfx::image_type::planar_2D: load_2D(); break;
		// case vk::ImageViewType::eCube: load_cube(); break;
		default: debug_break();
		}
	}
	else
	{
		
		// this is a generated file;
		switch(m_Meta->image_type())
		{
		case gfx::image_type::planar_2D: create_2D(); break;
		// case vk::ImageViewType::eCube: load_cube(); break;
		default: debug_break();
		}
	}
fail:
	return;
}

texture::~texture()
{
	delete(m_TextureData);
	glDeleteTextures(1, &m_Texture);
}

void texture::load_2D()
{
	gli::texture2d* m_Texture2DData = (gli::texture2d*)m_TextureData;
	if(m_Texture2DData->empty())
	{
		LOG_ERROR("Empty texture");
		debug_break();
	}

	GLint internalFormat, format, type;

	if(!gfx::to_gles(m_Meta->format(), internalFormat, format, type))
	{
		core::igles::log->error("unsupported format detected: {}",
								static_cast<std::underlying_type_t<gfx::format>>(m_Meta->format()));
	}

	if(m_Meta->width() != (uint32_t)(*m_Texture2DData)[0].extent().x)
		m_Meta->width((uint32_t)(*m_Texture2DData)[0].extent().x);

	if(m_Meta->height() != (uint32_t)(*m_Texture2DData)[0].extent().y)
		m_Meta->height((uint32_t)(*m_Texture2DData)[0].extent().y);

	if(m_Meta->depth() != (uint32_t)(*m_Texture2DData)[0].extent().z)
		m_Meta->depth((uint32_t)(*m_Texture2DData)[0].extent().z);

	m_MipLevels = (uint32_t)m_Texture2DData->levels();
	m_Meta->mip_levels(m_MipLevels);


	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);

	glTexStorage2D(GL_TEXTURE_2D, static_cast<GLint>(m_Texture2DData->levels()), internalFormat, m_Meta->width(),
				   m_Meta->height());

	for(std::size_t Level = 0; Level < m_Texture2DData->levels(); ++Level)
	{
		auto extent = m_Texture2DData->extent(Level);
		if(type != 0)
		{

			glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(Level), 0, 0, extent.x, extent.y, format, type,
							m_Texture2DData->data(0, 0, Level));
		}
		else
		{
			auto size = static_cast<GLsizei>(m_Texture2DData->size(Level));
			glCompressedTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(Level), 0, 0, extent.x, extent.y,
									  internalFormat, size, m_Texture2DData->data(0, 0, Level));
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

void texture::create_2D() 
{
	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_Meta->width(), m_Meta->height(), 0, GL_RGBA, GL_FLOAT, nullptr);
}