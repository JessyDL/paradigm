// temporary main file for webgpu, will be removed once we have a proper implementation

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "core/resource/resource.hpp"
#include "psl/application_utils.hpp"
#include "psl/library.hpp"
#include "psl/platform_utils.hpp"

#include "core/paradigm.hpp"

#include "core/logging.hpp"

#include "core/gfx/limits.hpp"
#include "core/gfx/types.hpp"
#include "core/utility/geometry.hpp"

#include "core/data/buffer.hpp"
#include "core/data/geometry.hpp"
#include "core/data/material.hpp"
#include "core/data/sampler.hpp"
#include "core/data/window.hpp"	   // application data

#include "core/os/context.hpp"
#include "core/os/surface.hpp"	  // the OS surface to draw one

#include "core/gfx/context.hpp"
#include "core/gfx/render_graph.hpp"
#include "core/gfx/swapchain.hpp"

#include <core/wgpu/iwgpu.hpp>

using namespace core;
using namespace core::resource;
using namespace core::gfx;

int entry_agnostic(gfx::graphics_backend backend, core::os::context& os_context) {
	core::log->info("Starting the application");
	core::log->info("creating a '{}' backend", gfx::graphics_backend_str(backend));

	core::log->info("creating cache");
	cache_t cache {psl::meta::library {}};
	core::log->info("cache created");

	auto window_data = cache.create<data::window>();
	window_data->name(APPLICATION_FULL_NAME + " { " + gfx::graphics_backend_str(backend) + " }");
	auto surface_handle = cache.create<core::os::surface>(window_data);
	if(!surface_handle) {
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	auto context_handle = cache.create<core::gfx::context>(backend, psl::string8_t {APPLICATION_NAME}, surface_handle);
	auto swapchain_handle = cache.create<core::gfx::swapchain>(surface_handle, context_handle, os_context);

	core::gfx::render_graph renderGraph {};


	auto swapchain_pass = renderGraph.create_drawpass(context_handle, swapchain_handle);

	while(os_context.tick() && surface_handle->tick()) {
		renderGraph.present();
	}
	return 0;
}

int entry(gfx::graphics_backend backend, core::os::context& os_context) {
	core::log->info("Starting the application");

	psl::string libraryPath {psl::utility::application::path::library + "resources.metalib"};
	memory::region resource_region {20_mb, 4u, new memory::default_allocator()};
	psl::string8_t environment = "";
	switch(backend) {
	case graphics_backend::gles:
		environment = "gles";
		break;
	case graphics_backend::vulkan:
		environment = "vulkan";
		break;
	case graphics_backend::webgpu:
		environment = "webgpu";
		break;
	}

	core::log->info("creating a '{}' backend", environment);

	core::log->info("creating cache");
	cache_t cache {psl::meta::library {psl::to_string8_t(libraryPath), {{environment}}}};
	core::log->info("cache created");

	auto window_data = cache.instantiate<data::window>("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
	window_data->name(APPLICATION_FULL_NAME + " { " + environment + " }");
	auto surface_handle = cache.create<core::os::surface>(window_data);
	if(!surface_handle) {
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	wgpu::InstanceDescriptor desc = {};
	desc.nextInChain			  = nullptr;
	wgpu::Instance instance		  = wgpu::CreateInstance(&desc);

	if(!instance) {
		core::log->critical("Could not create a WebGPU instance.");
		return 1;
	}

	std::future<wgpu::Adapter> adapter_future;

	wgpu::SurfaceDescriptorFromWindowsHWND winsurface_desc = {};
	winsurface_desc.nextInChain							   = nullptr;
	winsurface_desc.hinstance							   = surface_handle->surface_instance();
	winsurface_desc.hwnd								   = surface_handle->surface_handle();

	wgpu::SurfaceDescriptor surface_desc = {};
	surface_desc.nextInChain			 = &winsurface_desc;
	surface_desc.label					 = "WebGPU Surface";

	wgpu::Surface surface = instance.CreateSurface(&surface_desc);


	wgpu::Adapter adapter;
	wgpu::RequestAdapterOptions options = {};
	options.compatibleSurface			= surface;
	wgpu::RequestAdapter(
	  instance,
	  options,
	  [&adapter](wgpu::RequestAdapterStatus status, wgpu::Adapter _adapter, const char* message, void* userdata) {
		  if(status == wgpu::RequestAdapterStatus::Success) {
			  adapter = _adapter;
		  } else {
			  core::log->critical("Could not create a WebGPU adapter: {}", message);
		  }
	  },
	  nullptr);


	auto const count = adapter.EnumerateFeatures(nullptr);
	std::vector<wgpu::FeatureName> features(count);
	adapter.EnumerateFeatures(features.data());

	for(auto const& feature : features) {
		core::log->info("Feature: {}", std::to_underlying(feature));
	}

	wgpu::DeviceDescriptor device_desc = {};
	device_desc.label				   = "WebGPU Device";
	device_desc.defaultQueue.label	   = "WebGPU Queue";
	wgpu::Device device;
	wgpu::RequestDevice(
	  adapter,
	  device_desc,
	  [&device](wgpu::RequestDeviceStatus status, wgpu::Device _device, const char* message, void* userdata) {
		  if(status == wgpu::RequestDeviceStatus::Success) {
			  device = _device;
		  } else {
			  core::log->critical("Could not create a WebGPU device: {}", message);
		  }
	  });

	device.SetUncapturedErrorCallback([](WGPUErrorType type,
										 const char* message,
										 void* userdata) -> void { core::log->error("WebGPU error: {}", message); },
									  nullptr);

	auto queue = device.GetQueue();
	queue.OnSubmittedWorkDone(
	  [](WGPUQueueWorkDoneStatus status, void* userdata) -> void {
		  core::log->info("WebGPU queue work done: {}", std::to_underlying(status));
	  },
	  nullptr);

	auto swap_chain_desc		= wgpu::SwapChainDescriptor();
	swap_chain_desc.usage		= wgpu::TextureUsage::RenderAttachment;
	swap_chain_desc.format		= surface.GetPreferredFormat(adapter);
	swap_chain_desc.width		= window_data->width();
	swap_chain_desc.height		= window_data->height();
	swap_chain_desc.presentMode = wgpu::PresentMode::Fifo;

	auto swap_chain = device.CreateSwapChain(surface, &swap_chain_desc);


	while(os_context.tick() && surface_handle->tick()) {
		auto texture_view = swap_chain.GetCurrentTextureView();

		auto color_attachments			= std::vector<wgpu::RenderPassColorAttachment>(1);
		color_attachments[0].view		= texture_view;
		color_attachments[0].loadOp		= wgpu::LoadOp::Clear;
		color_attachments[0].storeOp	= wgpu::StoreOp::Store;
		color_attachments[0].clearValue = {(std::rand() % 255) / 255.f, 0.0f, 0.0f, 1.0f};

		auto pass_descriptor				 = wgpu::RenderPassDescriptor();
		pass_descriptor.colorAttachments	 = color_attachments.data();
		pass_descriptor.colorAttachmentCount = color_attachments.size();

		auto command_encoder = device.CreateCommandEncoder();
		auto render_pass	 = command_encoder.BeginRenderPass(&pass_descriptor);
		render_pass.End();
		auto command_buffer = command_encoder.Finish();
		queue.Submit(1, &command_buffer);
		swap_chain.Present();
	}

	return 0;
}


int main(int argc, char** argv) {
#ifdef PE_PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	core::initialize_loggers();

#ifdef _MSC_VER
	{	 // here to trick the compiler into generating these types to get UUID natvis support
		dummy::hex_dummy_high hex_dummy_high {};
		dummy::hex_dummy_low hex_dummy_lowy {};
	}
#endif
	std::srand(0);
	if(argc > 0) {
		core::log->info("Received the cli args:");
		for(auto i = 0; i < argc; ++i) core::log->info(argv[i]);
	}
	auto backend = [](int argc, char* argv[]) {
		for(auto i = 0; i < argc; ++i) {
			std::string_view text {argv[i]};
			if(text == "--vulkan") {
#if defined(PE_VULKAN)
				return graphics_backend::vulkan;
#else
				throw std::runtime_error("Requested a Vulkan backend, but application does not support Vulkan");
#endif
			} else if(text == "--gles") {
#if defined(PE_GLES)
				return graphics_backend::gles;
#else
				throw std::runtime_error("Requested a GLES backend, but application does not support GLES");
#endif
			} else if(text == "--webgpu") {
#if defined(PE_WEBGPU)
				return graphics_backend::webgpu;
#else
				throw std::runtime_error("Requested a WebGPU backend, but application does not support WebGPU");
#endif
			}
		}
#if defined(PE_VULKAN)
		return graphics_backend::vulkan;
#elif defined(PE_WEBGPU)
		return graphics_backend::webgpu;
#elif defined(PE_GLES)
		return graphics_backend::gles;
#endif
		return graphics_backend::undefined;
	}(argc, argv);
	core::os::context context {};
	return entry_agnostic(backend, context);
}
