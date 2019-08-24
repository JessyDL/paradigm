#include "gles/program.h"
#include "resource/resource.hpp"
#include "gfx/types.h"
#include "data/material.h"
#include "logging.h"
#include "gles/shader.h"
#include "meta/shader.h"

using namespace core::igles;
using namespace core::resource;

program::program(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				 core::resource::handle<core::data::material> data)
{
	GLint linked;

	std::vector<GLint> shaderStages;
	for(auto& stage : data->stages())
	{
		auto shader_handle = cache.find<core::igles::shader>(stage.shader());
		if(!shader_handle) shader_handle = cache.create_using<core::igles::shader>(stage.shader());
		if((shader_handle.state() == core::resource::state::loaded) &&
		   shader_handle->id() != 0)
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

		if(infoLen > 1)
		{
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);

			glGetProgramInfoLog(m_Program, infoLen, NULL, infoLog);

			free(infoLog);
		}

		glDeleteProgram(m_Program);
		m_Program = 0;
	}
}

program::~program() { glDeleteProgram(m_Program); }