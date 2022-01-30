#include "gles/context.hpp"
#include "gfx/limits.hpp"
#include "gles/igles.hpp"

#include <numeric>

using namespace core::igles;

template <typename T>
void queryIntegerv(auto target, T& storage) requires requires(GLint v)
{
	static_cast<T>(v);
}
{
	GLint value {};
	glGetIntegerv(target, &value);
	storage = static_cast<T>(value);
}

template <typename T>
void queryIntegeri_v(auto target, auto i, T& storage) requires requires(GLint v)
{
	static_cast<T>(v);
}
{
	GLint value {};
	glGetIntegeri_v(target, i, &value);
	storage = static_cast<T>(value);
}

void context::quey_capabilities() noexcept
{
	queryIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, m_Limits.vertex_shader.ssbo);
	queryIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, m_Limits.fragment_shader.ssbo);

	queryIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, m_Limits.storage.alignment);
	queryIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, m_Limits.storage.size);
	queryIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, m_Limits.uniform.alignment);
	queryIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, m_Limits.uniform.size);

#ifdef GL_ARB_map_buffer_alignment
	queryIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, m_Limits.memorymap.alignment);
#else
	// no real requirements, but let's take something
	m_Limits.memorymap.alignment = 4;
#endif
	m_Limits.memorymap.size		   = std::numeric_limits<uint64_t>::max();
	m_Limits.supported_depthformat = core::gfx::format_t::d32_sfloat;

	queryIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, m_Limits.compute.workgroup.count[0]);
	queryIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, m_Limits.compute.workgroup.count[1]);
	queryIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, m_Limits.compute.workgroup.count[2]);

	queryIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, m_Limits.compute.workgroup.size[0]);
	queryIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, m_Limits.compute.workgroup.size[1]);
	queryIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, m_Limits.compute.workgroup.size[2]);

	queryIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, m_Limits.compute.workgroup.invocations);
}