#include "gles/context.hpp"
#include "gfx/limits.hpp"
#include "gles/igles.hpp"

#include <numeric>

using namespace core::igles;

void context::quey_capabilities() noexcept
{
	GLint value {};

	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &value);
	m_Limits.storage.alignment = value;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &value);
	m_Limits.storage.size = value;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &value);
	m_Limits.uniform.alignment = value;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &value);
	m_Limits.uniform.size = value;

#ifdef GL_ARB_map_buffer_alignment
	glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &value);
	m_Limits.memorymap.alignment = value;
#else
	// no real requirements, but let's take something
	m_Limits.memorymap.alignment = 4;
#endif
	m_Limits.memorymap.size		   = std::numeric_limits<uint64_t>::max();
	m_Limits.supported_depthformat = core::gfx::format_t::d32_sfloat;

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &value);
	m_Limits.compute.workgroup.count[0] = value;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &value);
	m_Limits.compute.workgroup.count[1] = value;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &value);
	m_Limits.compute.workgroup.count[2] = value;

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &value);
	m_Limits.compute.workgroup.size[0] = value;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &value);
	m_Limits.compute.workgroup.size[1] = value;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &value);
	m_Limits.compute.workgroup.size[2] = value;

	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &value);
	m_Limits.compute.workgroup.invocations = value;
}