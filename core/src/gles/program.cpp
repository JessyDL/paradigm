#include "gles/program.h"
#include "resource/resource.hpp"
#include "gfx/types.h"
#include "data/material.h"
#include "logging.h"
#include "gles/shader.h"
#include "meta/shader.h"
#include "gles/igles.h"

using namespace core::igles;
using namespace core::resource;

program::program(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				 core::resource::handle<core::data::material> data)
{
	GLint linked;

	std::vector<GLint> shaderStages;
	for(const auto& stage : data->stages())
	{
		auto shader_handle = cache.find<core::igles::shader>(stage.shader());
		if(!shader_handle) shader_handle = cache.create_using<core::igles::shader>(stage.shader());
		if((shader_handle.state() == core::resource::state::loaded) && shader_handle->id() != 0)
		{
			shaderStages.push_back(shader_handle->id());
		}
		else
		{
			core::igles::log->error("could not load the shader used in the creation of a pipeline");
			return;
		}
	}

	m_Program = glCreateProgram();

	for(auto shader : shaderStages)
	{
		glAttachShader(m_Program, shader);
	}
	glLinkProgram(m_Program);
	glGetProgramiv(m_Program, GL_LINK_STATUS, &linked);

	if(!linked)
	{
		GLint infoLen = 0;

		glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &infoLen);

		psl::string shader_uids;
		std::for_each(
			std::begin(data->stages()), std::end(data->stages()), [&shader_uids](const auto& stage) noexcept {
				shader_uids += stage.shader().to_string();
				shader_uids += ", ";
			});
		shader_uids.resize(shader_uids.size() - 2);
		if(infoLen > 1)
		{
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);

			glGetProgramInfoLog(m_Program, infoLen, NULL, infoLog);
			core::igles::log->critical("Failed to link program {0} with shaders {1}\n{2}", metaFile->ID().to_string(), shader_uids,
									   infoLog);

			free(infoLog);
		}
		else
		{
			core::igles::log->critical("Failed to link program {0} with shaders {1}", metaFile->ID().to_string(),
									   shader_uids);
		}

		glDeleteProgram(m_Program);
		m_Program = 0;
	}
}

program::~program() { glDeleteProgram(m_Program); }