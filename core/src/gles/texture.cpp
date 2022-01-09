#include "gles/texture.h"
#include "gles/conversion.h"
#include "gles/igles.h"
#include "logging.h"
#include "meta/texture.h"
#include "resource/resource.hpp"
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

texture::texture(core::resource::cache& cache,
				 const core::resource::metadata& metaData,
				 core::meta::texture* metaFile) :
	m_Cache(cache),
	m_Meta(m_Cache.library().get<core::meta::texture>(metaFile->ID()).value_or(nullptr))
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
		m_TextureData = new gli::texture(gli::load(result.value().data(), result.value().size()));
		switch(m_Meta->image_type())
		{
		case gfx::image_type::planar_2D:
			load_2D();
			break;
		// case vk::ImageViewType::eCube: load_cube(); break;
		default:
			debug_break();
		}
	}
	else
	{
		auto result = cache.library().load(m_Meta->ID());
		auto data	= (result && !result.value().empty()) ? (void*)result.value().data() : nullptr;

		// this is a generated file;
		switch(m_Meta->image_type())
		{
		case gfx::image_type::planar_2D:
			create_2D(data);
			break;
		// case vk::ImageViewType::eCube: load_cube(); break;
		default:
			debug_break();
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

	if(!gfx::conversion::to_gles(m_Meta->format(), internalFormat, format, type))
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

	core::igles::log->info("loading texture {} with format {} width {} : height {}",
						   m_Meta->ID().to_string(),
						   m_Meta->format(),
						   m_Meta->width(),
						   m_Meta->height());

	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);

	glTexStorage2D(
	  GL_TEXTURE_2D, static_cast<GLint>(m_Texture2DData->levels()), internalFormat, m_Meta->width(), m_Meta->height());

	for(std::size_t Level = 0; Level < m_Texture2DData->levels(); ++Level)
	{
		auto extent = m_Texture2DData->extent(Level);
		if(type != 0)
		{
			glTexSubImage2D(GL_TEXTURE_2D,
							static_cast<GLint>(Level),
							0,
							0,
							extent.x,
							extent.y,
							format,
							type,
							m_Texture2DData->data(0, 0, Level));
		}
		else
		{
			auto size = static_cast<GLsizei>(m_Texture2DData->size(Level));
			glCompressedTexSubImage2D(GL_TEXTURE_2D,
									  static_cast<GLint>(Level),
									  0,
									  0,
									  extent.x,
									  extent.y,
									  internalFormat,
									  size,
									  m_Texture2DData->data(0, 0, Level));
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

#include <malloc.h>

void texture::create_2D(void* data)
{
	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);

	GLint internalFormat, format, type;
	gfx::conversion::to_gles(m_Meta->format(), internalFormat, format, type);
	auto pixel_storage = gfx::packing_size(m_Meta->format());
	if(data != nullptr && pixel_storage != 4)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, pixel_storage);
	}

	glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, m_Meta->width(), m_Meta->height());

	if(data != nullptr)
	{
		if(type != 0)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Meta->width(), m_Meta->height(), format, type, data);
		}
		else
		{
			assert_debug_break(false);
		}

		if(pixel_storage != 4) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glGetError();
}


const core::meta::texture& texture::meta() const noexcept { return *m_Meta; }