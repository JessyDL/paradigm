#include "gfx/context.h"

#ifdef PE_VULKAN
#include "vk/context.h"
#include "vk/conversion.h"
#endif
#ifdef PE_GLES
#include "gles/context.h"
#include "gles/igles.h"
#endif

using namespace core;
using namespace core::gfx;
using namespace core::resource;

context::context(core::resource::handle<value_type>& handle) : m_Handle(handle){};
context::context(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				 graphics_backend backend, const psl::string8_t& name)
	: m_Backend(backend)
{
	switch(backend)
	{
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
	{
		m_Handle << cache.create_using<core::ivk::context>(metaData.uid, name);
		auto vkLimits							 = m_Handle.get<core::ivk::context>()->properties().limits;
		m_Limits.storage_buffer_offset_alignment = vkLimits.minStorageBufferOffsetAlignment;
		m_Limits.uniform_buffer_offset_alignment = vkLimits.minUniformBufferOffsetAlignment;
		vk::Format format;
		if(utility::vulkan::supported_depthformat(m_Handle.get<core::ivk::context>()->physical_device(), &format))
			m_Limits.supported_depthformat = core::gfx::conversion::to_format(format);
		else
			m_Limits.supported_depthformat = core::gfx::format::undefined;

		m_Limits.compute_worgroup_count[0] = vkLimits.maxComputeWorkGroupCount[0];
		m_Limits.compute_worgroup_count[1] = vkLimits.maxComputeWorkGroupCount[1];
		m_Limits.compute_worgroup_count[2] = vkLimits.maxComputeWorkGroupCount[2];

		m_Limits.compute_worgroup_size[0] = vkLimits.maxComputeWorkGroupSize[0];
		m_Limits.compute_worgroup_size[1] = vkLimits.maxComputeWorkGroupSize[1];
		m_Limits.compute_worgroup_size[2] = vkLimits.maxComputeWorkGroupSize[2];

		m_Limits.compute_worgroup_invocations = vkLimits.maxComputeWorkGroupInvocations;
	}
	break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles:
	{
		m_Handle << cache.create_using<core::igles::context>(metaData.uid, name);
		m_Limits = m_Handle.get<core::igles::context>()->limits();
	}
	break;
#endif
	}
}


const core::gfx::limits& context::limits() const noexcept { return m_Limits; }