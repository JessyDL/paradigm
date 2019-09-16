#include "gles/compute.h"
#include "resource/resource.hpp"
#include "gles/igles.h"
#include "meta/shader.h"
#include "gles/conversion.h"

using namespace core::igles;
using namespace core::resource;
using namespace psl::meta;
using namespace core::gfx::conversion;

compute::compute(cache& cache, const metadata& metaData, file* metaFile)
{
	auto meta   = cache.library().get<core::meta::shader>(metaFile->ID()).value_or(nullptr);
	m_Meta		= meta;
	auto result = cache.library().load(meta->ID());
	if(!result)
	{
		core::ivk::log->error("could not load igles::compute [{0}] from resource UID [{1}]", metaData.uid.to_string(),
							  meta->ID().to_string());
		return;
	}
	assert(to_gles(meta->stage()) == GL_COMPUTE_SHADER || "shader is required to be of type GL_COMPUTE_SHADER");


	GLint status;
	m_Shader = glCreateShader(GL_COMPUTE_SHADER);
	if(m_Shader == 0) throw std::runtime_error("could not load the given shader");

	auto shaderData = result.value().data();
	glShaderSource(m_Shader, 1, &shaderData, NULL);
	glCompileShader(m_Shader);
	glGetShaderiv(m_Shader, GL_COMPILE_STATUS, &status);
	if(!status)
	{
		GLint infoLen = 0;

		glGetShaderiv(m_Shader, GL_INFO_LOG_LENGTH, &infoLen);

		if(infoLen > 1)
		{
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);

			glGetShaderInfoLog(m_Shader, infoLen, NULL, infoLog);
			// esLogMessage("Error compiling shader:\n%s\n", infoLog);

			core::igles::log->error("could not compile igles::compute [{0}] from resource UID [{1}] with message: {2}",
									metaData.resource_uid.to_string(), meta->ID().to_string(), infoLog);
			free(infoLog);
		}
		else
			core::igles::log->error("could not compile igles::compute [{0}] from resource UID [{1}]",
									metaData.resource_uid.to_string(), meta->ID().to_string());

		glDeleteShader(m_Shader);


		m_Shader = 0;
		return;
	}

	status	= {};
	m_Program = glCreateProgram();
	glAttachShader(m_Program, m_Shader);
	glLinkProgram(m_Program);
	glGetProgramiv(m_Program, GL_LINK_STATUS, &status);

	if(!status)
	{
		GLint infoLen = 0;

		glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &infoLen);

		if(infoLen > 1)
		{
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);

			glGetProgramInfoLog(m_Program, infoLen, NULL, infoLog);

			core::igles::log->error("could not link igles::compute [{0}] from resource UID [{1}] with message: {2}",
									metaData.resource_uid.to_string(), meta->ID().to_string(), infoLog);
			free(infoLog);
		}
		else
			core::igles::log->error("could not link igles::compute [{0}] from resource UID [{1}]",
									metaData.resource_uid.to_string(), meta->ID().to_string());


		glDeleteProgram(m_Program);
		m_Program = 0;
		return;
	}
}

compute::~compute()
{
	if(m_Program != 0) glDeleteProgram(m_Program);
	if(m_Shader != 0) glDeleteShader(m_Shader);
}


void compute::dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z) const noexcept
{
	glUseProgram(m_Program);
	glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}