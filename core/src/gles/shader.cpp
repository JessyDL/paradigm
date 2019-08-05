#include "gles/shader.h"
#include "systems/resource.h"
#include "meta/shader.h"
#include "gfx/types.h"
#include "gfx/stdafx.h"

using namespace psl;
using namespace core::igles;
using namespace core::resource;

shader::shader(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile)
{
	auto meta = cache.library().get<core::meta::shader>(metaFile->ID()).value_or(nullptr);

	auto result = cache.library().load(meta->ID());
	if(!result)
	{
		core::ivk::log->error("could not load igles::shader [{0}] from resource UID [{1}]", uid.to_string(),
							  meta->ID().to_string());
		return;
	}

	auto gl_stage = gfx::to_gles(gfx::to_shader_stage(meta->stage()));

	GLuint shader;
	GLint compiled;

	// Create the shader object
	shader = glCreateShader(gl_stage);

	if(shader == 0) throw std::runtime_error("could not load the given shader");

	auto shaderData = result.value().data();
	glShaderSource(shader, 1, &shaderData, NULL);

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if(!compiled)
	{
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if(infoLen > 1)
		{
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);

			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			// esLogMessage("Error compiling shader:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteShader(shader);


		core::ivk::log->error("could not compile igles::shader [{0}] from resource UID [{1}] with message: {2}",
							  uid.to_string(), meta->ID().to_string(), infoLen);
	}

	m_Shader = shader;
}

shader::~shader() { glDeleteShader(m_Shader); }

GLuint shader::id() const noexcept { return m_Shader; }