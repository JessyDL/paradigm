#include "core/wgpu/context.hpp"

#include "core/os/surface.hpp"
#include "core/resource/cache.hpp"
#include <functional>

using namespace core::iwgpu;

void WebGPUErrorCallback(wgpu::Device device, wgpu::ErrorType type, wgpu::StringView message, void*) {
	switch(type) {
	case wgpu::ErrorType::Validation: {
		core::iwgpu::log->error("[validation] {}", message.data);
	} break;
	case wgpu::ErrorType::OutOfMemory: {
		core::iwgpu::log->error("[OOM] {}", message.data);
	} break;
	default:
	case wgpu::ErrorType::Unknown: {
		core::iwgpu::log->error("[unknown] {}", message.data);
	} break;
	case wgpu::ErrorType::Internal: {
		core::iwgpu::log->error("[internal] {}", message.data);
	} break;
	case wgpu::ErrorType::NoError:
		break;
	}
}


void WebGPUDeviceLostCallback(wgpu::Device device, wgpu::DeviceLostReason reason, wgpu::StringView message) {
	switch(reason) {
	case wgpu::DeviceLostReason::Destroyed: {
		core::iwgpu::log->error("WebGPU device lost: {}", message.data);
	} break;
	case wgpu::DeviceLostReason::FailedCreation: {
		core::iwgpu::log->error("WebGPU device lost due to failed creation: {}", message.data);
	} break;
	default:
	case wgpu::DeviceLostReason::Unknown: {
		core::iwgpu::log->error("WebGPU device lost due to unknown reason: {}", message.data);
	} break;
	case wgpu::DeviceLostReason::CallbackCancelled: {
		core::iwgpu::log->error("WebGPU device lost due to callback cancellation: {}", message.data);
	} break;
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
	device_desc.SetUncapturedErrorCallback<decltype(WebGPUErrorCallback), void*, decltype(WebGPUErrorCallback)>(
	  WebGPUErrorCallback, nullptr);
	device_desc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, WebGPUDeviceLostCallback);
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
	// m_Device.SetUncapturedErrorCallback(WebGPUErrorCallback, nullptr);

	m_Queue = m_Device.GetQueue();

	wgpu::Limits supportedLimits;
	m_Device.GetLimits(&supportedLimits);

	m_Limits.storage.alignment = supportedLimits.minStorageBufferOffsetAlignment;
	m_Limits.storage.size	   = supportedLimits.maxStorageBufferBindingSize;
	m_Limits.uniform.alignment = supportedLimits.minUniformBufferOffsetAlignment;
	m_Limits.uniform.size	   = supportedLimits.maxUniformBufferBindingSize;

	// todo: verify these numbers
	m_Limits.memorymap.size		 = std::numeric_limits<size_t>::max();
	m_Limits.memorymap.alignment = 4;

	m_Limits.supported_depthformat = core::gfx::format_t::d32_sfloat;

	m_Limits.compute.workgroup.count[0] = supportedLimits.maxComputeWorkgroupsPerDimension;
	m_Limits.compute.workgroup.count[1] = supportedLimits.maxComputeWorkgroupsPerDimension;
	m_Limits.compute.workgroup.count[2] = supportedLimits.maxComputeWorkgroupsPerDimension;

	m_Limits.compute.workgroup.size[0] = supportedLimits.maxComputeWorkgroupSizeX;
	m_Limits.compute.workgroup.size[1] = supportedLimits.maxComputeWorkgroupSizeY;
	m_Limits.compute.workgroup.size[2] = supportedLimits.maxComputeWorkgroupSizeZ;

	m_Limits.compute.workgroup.invocations = supportedLimits.maxComputeInvocationsPerWorkgroup;
}
