// temporary main file for webgpu, will be removed once we have a proper implementation

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "core/resource/resource.hpp"
#include "psl/application_utils.hpp"
#include "psl/library.hpp"
#include "psl/platform_utils.hpp"

#include "core/paradigm.hpp"

#include "core/logging.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#ifdef _MSC_VER
	#include "spdlog/sinks/msvc_sink.h"
#endif

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

#include <webgpu/webgpu_cpp.h>

#include <future>

using namespace core;
using namespace core::resource;
using namespace core::gfx;


inline std::tm localtime_safe(std::time_t timer) {
	std::tm bt {};
#if defined(__unix__)
	localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
	localtime_s(&bt, &timer);
#else
	static std::mutex mtx;
	std::lock_guard<std::mutex> lock(mtx);
	bt = *std::localtime(&timer);
#endif
	return bt;
}

void setup_loggers() {
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c						  = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm							  = localtime_safe(now_c);
	psl::string time;
	time.resize(20);
	strftime(time.data(), 20, "%Y-%m-%d %H-%M-%S", &now_tm);
	time[time.size() - 1] = '/';
	psl::string sub_path  = "logs/" + time;
	if(!psl::utility::platform::file::exists(psl::utility::application::path::get_path() + sub_path + "main.log"))
		psl::utility::platform::file::write(psl::utility::application::path::get_path() + sub_path + "main.log", "");
	std::vector<spdlog::sink_ptr> sinks;

	auto mainlogger = std::make_shared<spdlog::sinks::dist_sink_mt>();
	mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "main.log", true));
	mainlogger->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + "logs/latest.log", true));
#ifdef _MSC_VER
	mainlogger->add_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#else
	auto outlogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	outlogger->set_level(spdlog::level::level_enum::warn);
	mainlogger->add_sink(outlogger);
#endif

	auto ivklogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "ivk.log", true);

	auto igleslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "igles.log", true);

	auto gfxlogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "gfx.log", true);

	auto systemslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "systems.log", true);

	auto oslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "os.log", true);

	auto datalogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "data.log", true);

	auto corelogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
	  psl::utility::application::path::get_path() + sub_path + "core.log", true);

	sinks.push_back(mainlogger);
	sinks.push_back(corelogger);

	auto logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
	spdlog::register_logger(logger);
	core::log = logger;


	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(systemslogger);

	auto system_logger = std::make_shared<spdlog::logger>("systems", begin(sinks), end(sinks));
	spdlog::register_logger(system_logger);
	core::systems::log = system_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(oslogger);

	auto os_logger = std::make_shared<spdlog::logger>("os", begin(sinks), end(sinks));
	spdlog::register_logger(os_logger);
	core::os::log = os_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(datalogger);

	auto data_logger = std::make_shared<spdlog::logger>("data", begin(sinks), end(sinks));
	spdlog::register_logger(data_logger);
	core::data::log = data_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(gfxlogger);

	auto gfx_logger = std::make_shared<spdlog::logger>("gfx", begin(sinks), end(sinks));
	spdlog::register_logger(gfx_logger);
	core::gfx::log = gfx_logger;

#ifdef PE_VULKAN
	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(ivklogger);

	auto ivk_logger = std::make_shared<spdlog::logger>("ivk", begin(sinks), end(sinks));
	spdlog::register_logger(ivk_logger);
	core::ivk::log = ivk_logger;
#endif
#ifdef PE_GLES
	sinks.clear();
	sinks.push_back(mainlogger);
	sinks.push_back(igleslogger);

	auto igles_logger = std::make_shared<spdlog::logger>("igles", begin(sinks), end(sinks));
	spdlog::register_logger(igles_logger);
	core::igles::log = igles_logger;
#endif
	spdlog::set_pattern("%8T.%6f [%=8n] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}

namespace wgpu {

struct RequestAdapterCallbackResult {
	wgpu::Adapter adapter;
	wgpu::RequestAdapterStatus status;
	char const* message;
	void* userdata;
};

[[nodiscard]] auto RequestAdapter(wgpu::Instance instance, wgpu::RequestAdapterOptions options)
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
auto RequestAdapter(wgpu::Instance instance,
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
	wgpu::Device device;
	wgpu::RequestDeviceStatus status;
	char const* message;
	void* userdata;
};

[[nodiscard]] auto RequestDevice(wgpu::Adapter adapter, wgpu::DeviceDescriptor descriptor)
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
auto RequestDevice(wgpu::Adapter adapter, wgpu::DeviceDescriptor descriptor, Fn&& invocable, void* userdata = nullptr)
  -> void
	requires std::is_invocable_v<Fn, wgpu::RequestDeviceStatus, wgpu::Device, char const*, void*>
{
	auto future = RequestDevice(adapter, descriptor);
	auto result = future.get();
	invocable(result.status, std::move(result.device), result.message, userdata);
}
}	 // namespace wgpu

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

		auto color_attachments = std::vector<wgpu::RenderPassColorAttachment>(1);
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
	setup_loggers();

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
			return graphics_backend::undefined;
		}
#if defined(PE_VULKAN)
		return graphics_backend::vulkan;
#elif defined(PE_WEBGPU)
		return graphics_backend::webgpu;
#elif defined(PE_GLES)
		return graphics_backend::gles;
#endif
	}(argc, argv);
	core::os::context context {};
	return entry(backend, context);
}
