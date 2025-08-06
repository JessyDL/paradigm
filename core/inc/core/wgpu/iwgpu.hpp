#pragma once

#include <webgpu/webgpu_cpp.h>

#if !defined(PE_NO_WEBGPU_EXTENSIONS)
	#include <future>
namespace wgpu {
struct RequestAdapterCallbackResult {
	wgpu::Adapter adapter			  = {};
	wgpu::RequestAdapterStatus status = wgpu::RequestAdapterStatus::Unavailable;
	char const* message				  = nullptr;
	void* userdata					  = nullptr;
};

[[nodiscard]] inline auto RequestAdapter(wgpu::Instance instance, wgpu::RequestAdapterOptions options)
  -> std::future<RequestAdapterCallbackResult> {
	auto promise = std::make_shared<std::promise<RequestAdapterCallbackResult>>();

	instance.RequestAdapter(
	  &options,
	  CallbackMode::AllowSpontaneous,
	  [promise](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) -> void {
		  promise->set_value({adapter, status, message.data, nullptr});
	  });

	return promise->get_future();
}

template <typename Fn>
inline auto RequestAdapter(wgpu::Instance instance,
						   wgpu::RequestAdapterOptions options,
						   Fn&& invocable,
						   void* userdata = nullptr) -> void
	requires std::is_invocable_v<Fn, wgpu::RequestAdapterStatus, wgpu::Adapter, char const*, void*>
{
	auto future = RequestAdapter(instance, options);
	auto result = future.get();
	invocable(result.status, std::move(result.adapter), result.message, userdata);
}

struct DeviceCallbackResult {
	wgpu::Device device				 = {};
	wgpu::RequestDeviceStatus status = wgpu::RequestDeviceStatus::Error;
	char const* message				 = nullptr;
	void* userdata					 = nullptr;
};

[[nodiscard]] inline auto RequestDevice(wgpu::Adapter adapter,
										wgpu::DeviceDescriptor descriptor) -> std::future<DeviceCallbackResult> {
	auto promise = std::make_shared<std::promise<DeviceCallbackResult>>();
	adapter.RequestDevice(&descriptor,
						  CallbackMode::AllowSpontaneous,
						  [promise](wgpu::RequestDeviceStatus status, wgpu::Device device, const char* message) {
							  promise->set_value({device, status, message, nullptr});
						  });

	return promise->get_future();
}

template <typename Fn>
inline auto RequestDevice(wgpu::Adapter adapter,
						  wgpu::DeviceDescriptor descriptor,
						  Fn&& invocable,
						  void* userdata = nullptr) -> void
	requires std::is_invocable_v<Fn, wgpu::RequestDeviceStatus, wgpu::Device, char const*, void*>
{
	auto future = RequestDevice(adapter, descriptor);
	auto result = future.get();
	invocable(result.status, std::move(result.device), result.message, userdata);
}
}	 // namespace wgpu

#endif
