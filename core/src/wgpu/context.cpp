#include "core/wgpu/context.hpp"

#include "core/os/surface.hpp"
#include "core/resource/cache.hpp"

using namespace core::iwgpu;

void WebGPUErrorCallback(WGPUErrorType type, const char* message, void* userdata) {
	switch(type) {
	case WGPUErrorType_Validation: {
		core::iwgpu::log->error("[validation] {}", message);
	} break;
	case WGPUErrorType_OutOfMemory: {
		core::iwgpu::log->error("[OOM] {}", message);
	} break;
	default:
	case WGPUErrorType_Unknown: {
		core::iwgpu::log->error("[unknown] {}", message);
	} break;
	case WGPUErrorType_DeviceLost: {
	} break;
	case WGPUErrorType_Internal: {
		core::iwgpu::log->error("[internal] {}", message);
	} break;
	case WGPUErrorType_NoError:
		break;
	}
}

context::context(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 psl::string8::view name,
				 core::resource::handle<core::os::surface> surface) {
	wgpu::InstanceDescriptor desc = {};
	desc.nextInChain			  = nullptr;
	m_Instance					  = wgpu::CreateInstance(&desc);

	if(!m_Instance) {
		core::iwgpu::log->critical("Failed to create WebGPU instance");
	}
	wgpu::SurfaceDescriptor surfaceDesc = {};

#if defined(SURFACE_WIN32)
	wgpu::SurfaceDescriptorFromWindowsHWND winSurfaceDesc = {};
	winSurfaceDesc.nextInChain							  = nullptr;
	winSurfaceDesc.hinstance							  = surface->surface_instance();
	winSurfaceDesc.hwnd									  = surface->surface_handle();

	surfaceDesc.nextInChain = &winSurfaceDesc;
#else
	#pragma error "Surface not implemented"
#endif

	surfaceDesc.label = "Surface";
	m_Surface		  = m_Instance.CreateSurface(&surfaceDesc);

	wgpu::RequestAdapterOptions adapterOptions = {};
	adapterOptions.nextInChain				   = nullptr;
	adapterOptions.powerPreference			   = wgpu::PowerPreference::HighPerformance;
	adapterOptions.compatibleSurface		   = m_Surface;

	wgpu::RequestAdapter(
	  m_Instance,
	  adapterOptions,
	  [&](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, char const* message, void* userData) {
		  if(status == wgpu::RequestAdapterStatus::Success) {
			  m_Adapter = adapter;
		  } else {
			  core::iwgpu::log->critical("Failed to create WebGPU adapter: {}", message);
		  }
	  });

	// todo: query the adapter for the limits, and then set up the required limits in the device

	wgpu::DeviceDescriptor device_desc = {};
	const auto deviceName			   = fmt::format("{} WebGPU Device", name);
	device_desc.label				   = deviceName.c_str();
	device_desc.defaultQueue.label	   = "WebGPU Queue";
	wgpu::RequestDevice(
	  m_Adapter,
	  device_desc,
	  [&](wgpu::RequestDeviceStatus status, wgpu::Device device, const char* message, void* userdata) {
		  if(status == wgpu::RequestDeviceStatus::Success) {
			  m_Device = device;
		  } else {
			  core::log->critical("Could not create a WebGPU device: {}", message);
		  }
	  });

	m_Device.SetUncapturedErrorCallback(WebGPUErrorCallback, nullptr);

	m_Queue = m_Device.GetQueue();

	wgpu::SupportedLimits supportedLimits;
	m_Device.GetLimits(&supportedLimits);

	m_Limits.storage.alignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	m_Limits.storage.size	   = supportedLimits.limits.maxStorageBufferBindingSize;
	m_Limits.uniform.alignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
	m_Limits.uniform.size	   = supportedLimits.limits.maxUniformBufferBindingSize;

	// todo: verify these numbers
	m_Limits.memorymap.size		 = std::numeric_limits<size_t>::max();
	m_Limits.memorymap.alignment = 4;

	m_Limits.supported_depthformat = core::gfx::format_t::d32_sfloat;

	m_Limits.compute.workgroup.count[0] = supportedLimits.limits.maxComputeWorkgroupsPerDimension;
	m_Limits.compute.workgroup.count[1] = supportedLimits.limits.maxComputeWorkgroupsPerDimension;
	m_Limits.compute.workgroup.count[2] = supportedLimits.limits.maxComputeWorkgroupsPerDimension;

	m_Limits.compute.workgroup.size[0] = supportedLimits.limits.maxComputeWorkgroupSizeX;
	m_Limits.compute.workgroup.size[1] = supportedLimits.limits.maxComputeWorkgroupSizeY;
	m_Limits.compute.workgroup.size[2] = supportedLimits.limits.maxComputeWorkgroupSizeZ;

	m_Limits.compute.workgroup.invocations = supportedLimits.limits.maxComputeInvocationsPerWorkgroup;
}
