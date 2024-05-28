#pragma once

#include <webgpu/webgpu_cpp.h>

#if !defined(PE_NO_WEBGPU_EXTENSIONS)
	#include <future>
namespace wgpu {
struct RequestAdapterCallbackResult {
	wgpu::Adapter adapter			  = {};
	wgpu::RequestAdapterStatus status = wgpu::RequestAdapterStatus::Unknown;
	char const* message				  = nullptr;
	void* userdata					  = nullptr;
};

[[nodiscard]] inline auto RequestAdapter(wgpu::Instance instance, wgpu::RequestAdapterOptions options)
  -> std::future<RequestAdapterCallbackResult> {
	auto promise = std::promise<RequestAdapterCallbackResult>();
	instance.RequestAdapter(
	  &options,
	  [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* userdata) -> void {
		  auto promise = static_cast<std::promise<RequestAdapterCallbackResult>*>(userdata);
		  promise->set_value({adapter, static_cast<wgpu::RequestAdapterStatus>(status), message, userdata});
		  wgpuAdapterRelease(adapter);
	  },
	  &promise);

	return promise.get_future();
}

template <typename Fn>
inline auto
RequestAdapter(wgpu::Instance instance, wgpu::RequestAdapterOptions options, Fn&& invocable, void* userdata = nullptr)
  -> void requires std::is_invocable_v<Fn, wgpu::RequestAdapterStatus, wgpu::Adapter, char const*, void*> {
	auto future = RequestAdapter(instance, options);
	auto result = future.get();
	invocable(result.status, std::move(result.adapter), result.message, userdata);
}

struct DeviceCallbackResult {
	wgpu::Device device				 = {};
	wgpu::RequestDeviceStatus status = wgpu::RequestDeviceStatus::Unknown;
	char const* message				 = nullptr;
	void* userdata					 = nullptr;
};

[[nodiscard]] inline auto RequestDevice(wgpu::Adapter adapter, wgpu::DeviceDescriptor descriptor)
  -> std::future<DeviceCallbackResult> {
	auto promise = std::promise<DeviceCallbackResult>();
	adapter.RequestDevice(
	  &descriptor,
	  [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* userdata) -> void {
		  auto promise = static_cast<std::promise<DeviceCallbackResult>*>(userdata);
		  promise->set_value({device, static_cast<wgpu::RequestDeviceStatus>(status), message, userdata});
		  wgpuDeviceRelease(device);
	  },
	  &promise);

	return promise.get_future();
}

template <typename Fn>
inline auto
RequestDevice(wgpu::Adapter adapter, wgpu::DeviceDescriptor descriptor, Fn&& invocable, void* userdata = nullptr)
  -> void requires std::is_invocable_v<Fn, wgpu::RequestDeviceStatus, wgpu::Device, char const*, void*> {
	auto future = RequestDevice(adapter, descriptor);
	auto result = future.get();
	invocable(result.status, std::move(result.device), result.message, userdata);
}
}	 // namespace wgpu

#endif
