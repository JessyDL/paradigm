// assembler.cpp : Defines the entry point for the console application.
//

#include "psl/string_utils.hpp"
#include "psl/ustring.hpp"
#include "stdafx.hpp"
#include <array>
#include <iostream>

#ifdef WIN32
	#include <fcntl.h>
	#include <io.h>
	#include <windows.h>
#endif

#include "cli/value.hpp"
#include "generators/meta.hpp"
#include "generators/models.hpp"
#include "generators/project.hpp"
#include "generators/shader.hpp"

#include "core/resource/cache.hpp"

#include "core/paradigm.hpp"
#include "psl/collections/spmc.hpp"

using psl::cli::pack;
using psl::cli::value;

using namespace core::resource;
using namespace core::gfx;

#ifdef WIN32
BOOL CtrlHandler(DWORD fdwCtrlType) {
	switch(fdwCtrlType) {
	// Handle the CTRL-C signal.
	case CTRL_C_EVENT:
		// entry.pop();
		std::cout << "^C" << std::endl;
		return (TRUE);

	// CTRL-CLOSE: confirm that the user wants to exit.
	case CTRL_CLOSE_EVENT:
		std::cout << "exiting..." << std::endl;
		return (TRUE);

	// Pass other signals to the next handler.
	case CTRL_BREAK_EVENT:
		return FALSE;

	case CTRL_LOGOFF_EVENT:
		return FALSE;

	case CTRL_SHUTDOWN_EVENT:
		return FALSE;

	default:
		return FALSE;
	}
}
#endif

psl::string_view get_input(int argc, char* argv[]) {
	static psl::pstring_t input(4096, ('\0'));
	static bool firstRun = true;
	std::memset(input.data(), ('\0'), sizeof(psl::platform::char_t) * input.size());

	if(!firstRun) {
#if defined(WIN32) && defined(UNICODE)
		std::wcin.getline(input.data(), input.size() - 1);
#else
		std::cin.getline(input.data(), input.size() - 1);
#endif
	} else {
		firstRun		 = false;
		size_t offset	 = 0;
		auto const space = psl::to_pstring(psl::string8::view(" "));
		for(auto i = 1; i < argc; ++i) {
			auto str = psl::to_pstring(psl::string8::view(argv[i]));
			std::memcpy(input.data() + offset, str.data(), str.size() * sizeof(psl::pchar_t));
			offset += str.size();
			std::memcpy(input.data() + offset, space.data(), space.size() * sizeof(psl::pchar_t));
			offset += space.size();
		}
	}

#if defined(WIN32) && defined(UNICODE)
	static psl::string storage(8192, ('\0'));
	auto intermediate = psl::to_string8_t(psl::platform::view(input.data(), std::wcslen(input.data())));
	std::memcpy(storage.data(), intermediate.data(), intermediate.size());
	return {storage.data(), intermediate.size()};
#else
	return {input.data(), psl::strlen(input.data())};
#endif
}

#include "core/paradigm.hpp"
#include "psl/application_utils.hpp"
#include "psl/literals.hpp"

#include "core/data/buffer.hpp"
#include "core/data/material.hpp"
#include "core/data/window.hpp"
#include "core/os/context.hpp"
#include "core/os/surface.hpp"

#include "core/gfx/buffer.hpp"
#include "core/gfx/computepass.hpp"
#include "core/gfx/context.hpp"
#include "core/gfx/drawpass.hpp"
#include "core/gfx/material.hpp"
#include "core/gfx/render_graph.hpp"
#include "core/gfx/swapchain.hpp"


#include "core/logging.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#ifdef _MSC_VER
	#include "spdlog/sinks/msvc_sink.h"
#endif

#include <atomic>

std::atomic<graphics_backend> gBackend {graphics_backend::undefined};

using namespace core;

#include "core/data/sampler.hpp"
#include "core/gfx/sampler.hpp"
#include "core/gfx/texture.hpp"

void load_texture(resource::cache_t& cache, handle<core::gfx::context> context_handle, psl::UID const& texture) {
	if(!cache.contains(texture)) {
		auto textureHandle = cache.instantiate<gfx::texture_t>(texture, context_handle);
		assert(textureHandle);
	}
}

handle<core::data::material_t> setup_gfx_material_data(resource::cache_t& cache,
													   handle<core::gfx::context> context_handle,
													   psl::UID vert,
													   psl::UID frag,
													   psl::UID const& texture) {
	auto vertShaderMeta = cache.library().get<core::meta::shader>(vert).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>(frag).value();

	load_texture(cache, context_handle, texture);

	// create the sampler
	auto samplerData   = cache.create<data::sampler_t>();
	auto samplerHandle = cache.create<gfx::sampler_t>(context_handle, samplerData);

	// load the example material
	auto matData = cache.create<data::material_t>();

	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});

	auto stages = matData->stages();
	for(auto& stage : stages) {
		if(stage.shader_stage() != core::gfx::shader_stage::fragment)
			continue;

		auto bindings = stage.bindings();
		for(auto& binding : bindings) {
			if(binding.descriptor() != core::gfx::binding_type::combined_image_sampler)
				continue;
			binding.texture(texture);
			binding.sampler(samplerHandle);
		}
		stage.bindings(bindings);
		// binding.texture()
	}
	matData->stages(stages);
	matData->blend_states({core::data::material_t::blendstate(0)});
	return matData;
}


handle<core::gfx::material_t> setup_gfx_material(resource::cache_t& cache,
												 handle<core::gfx::context> context_handle,
												 handle<core::gfx::pipeline_cache> pipeline_cache,
												 handle<core::gfx::buffer_t> matBuffer,
												 psl::UID vert,
												 psl::UID frag,
												 psl::UID const& texture) {
	auto matData  = setup_gfx_material_data(cache, context_handle, vert, frag, texture);
	auto material = cache.create<core::gfx::material_t>(context_handle, matData, pipeline_cache, matBuffer);

	return material;
}

void ui_icon() {}

#include "core/ecs/systems/fly.hpp"

#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/input_tag.hpp"

bool volatile should_exit = false;
void launch_gassembler(graphics_backend backend) {
	using namespace core;
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
	}

	cache_t cache {psl::meta::library {psl::to_string8_t(libraryPath), {{environment}}}};

	auto window_data = cache.instantiate<data::window>("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
	auto window_name = APPLICATION_FULL_NAME + " { " + environment + " }";
	window_data->name(window_name);

	auto surface_handle = cache.create<core::os::surface>(window_data);
	if(!surface_handle) {
		core::log->critical("Could not create a OS surface to draw on.");
		return;
	}

	auto context_handle = cache.create<core::gfx::context>(backend, psl::string8_t {APPLICATION_NAME}, surface_handle);

	// this exists due to Android support.
	/// \todo ideally this is passed into the entry function
	auto os_context		  = core::os::context {};
	auto swapchain_handle = cache.create<core::gfx::swapchain>(surface_handle, context_handle, os_context);

	auto storage_buffer_align = context_handle->limits().storage.alignment;
	auto uniform_buffer_align = context_handle->limits().uniform.alignment;
	auto mapped_buffer_align  = context_handle->limits().memorymap.alignment;

	// create a staging buffer, this is allows for more advantagous resource access for the GPU
	core::resource::handle<gfx::buffer_t> stagingBuffer {};
	if(backend == graphics_backend::vulkan) {
		auto stagingBufferData = cache.create<data::buffer_t>(
		  core::gfx::memory_usage::transfer_source,
		  core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
		  memory::region {(size_t)128_mb, 4, new memory::default_allocator(false)});
		stagingBuffer = cache.create<gfx::buffer_t>(context_handle, stagingBufferData);
	}

	// create the buffers to store the model in
	// - memory region which we'll use to track the allocations, this is supposed to be virtual as we don't care to
	//   have a copy on the CPU
	// - then we create the vulkan buffer resource to interface with the GPU
	auto vertexBufferData = cache.create<data::buffer_t>(
	  core::gfx::memory_usage::vertex_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {256_mb, 4, new memory::default_allocator(false)});
	auto vertexBuffer = cache.create<gfx::buffer_t>(context_handle, vertexBufferData, stagingBuffer);

	auto indexBufferData = cache.create<data::buffer_t>(
	  core::gfx::memory_usage::index_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {128_mb, 4, new memory::default_allocator(false)});
	auto indexBuffer = cache.create<gfx::buffer_t>(context_handle, indexBufferData, stagingBuffer);

	auto dynamicInstanceBufferData =
	  cache.create<data::buffer_t>(core::gfx::memory_usage::vertex_buffer,
								   core::gfx::memory_property::host_visible | core::gfx::memory_property::host_coherent,
								   memory::region {128_mb, 4, new memory::default_allocator(false)});

	// instance buffer for vertex data, these are unique per streamed instance of a geometry in a shader
	auto instanceBufferData = cache.create<data::buffer_t>(
	  core::gfx::memory_usage::vertex_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {128_mb, 4, new memory::default_allocator(false)});
	auto instanceBuffer = cache.create<gfx::buffer_t>(context_handle, instanceBufferData, stagingBuffer);

	// instance buffer for material data, these are shared over all instances of a given material bind (over all
	// instances in the invocation)
	auto instanceMaterialBufferData = cache.create<data::buffer_t>(
	  core::gfx::memory_usage::uniform_buffer | core::gfx::memory_usage::transfer_destination,
	  core::gfx::memory_property::device_local,
	  memory::region {8_mb, uniform_buffer_align, new memory::default_allocator(false)});
	auto instanceMaterialBuffer =
	  cache.create<gfx::buffer_t>(context_handle, instanceMaterialBufferData, stagingBuffer);
	auto intanceMaterialBinding = cache.create<gfx::shader_buffer_binding>(instanceMaterialBuffer, 8_mb);
	cache.library().set(intanceMaterialBinding.uid(), core::data::material_t::MATERIAL_DATA);


	render_graph renderGraph {};
	auto swapchain_pass = renderGraph.create_drawpass(context_handle, swapchain_handle);

	// using psl::ecs::state;
	using namespace core::ecs::components;
	using namespace core::ecs::systems;
	using namespace psl::ecs;
	psl::ecs::state_t ECSState {};
	fly fly_system {ECSState, surface_handle->input()};

	/* create editor camera */
	auto eCam = ECSState.create(
	  1,
	  [](transform& value) {
		  value			 = transform {};
		  value.position = psl::vec3 {40, 15, 150};
		  value.rotation = psl::math::look_at_q(value.position, psl::vec3::zero, psl::vec3::up);
	  },
	  empty<camera> {},
	  empty<input_tag> {});

	size_t frame {0};
	std::chrono::duration<float> elapsed {};
	auto last_tick = std::chrono::high_resolution_clock::now();
	while(!should_exit && surface_handle->tick()) {
		std::cout << frame << std::endl;
		ECSState.tick(elapsed);
		renderGraph.present();

		auto current_time = std::chrono::high_resolution_clock::now();
		elapsed			  = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_tick);
		last_tick		  = current_time;

		++frame;
	}

	std::cout << "[gassembler] closing graphical assembler.." << std::endl;
	gBackend = graphics_backend::undefined;
}

template <typename T>
struct option {
	T value {};
	std::vector<T> options {};
};

core::gfx::graphics_backend parse(std::string const& value) {
	if(value == "")
		return core::gfx::graphics_backend::undefined;
	if(value == "vulkan")
		return core::gfx::graphics_backend::vulkan;
	if(value == "gles")
		return core::gfx::graphics_backend::gles;
	return core::gfx::graphics_backend::undefined;
}

int main(int argc, char* argv[]) {
#if defined(AS_DEVMODE)
	const auto default_interactive = true;
#else
	const auto default_interactive = false;
#endif

	core::initialize_loggers(false);

	assembler::log = std::make_shared<spdlog::logger>("", std::make_shared<spdlog::sinks::stdout_color_sink_st>());
	assembler::log->set_pattern("%v");
#ifdef WIN32
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

	// std::wstring res = L"Стоял";


	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize		 = sizeof cfi;
	cfi.nFont		 = 0;
	cfi.dwFontSize.X = 0;
	cfi.dwFontSize.Y = 14;
	cfi.FontFamily	 = FF_MODERN;
	cfi.FontWeight	 = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"Consolas");
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);

	/*psl::string x2 = _T("Árvíztűrő tükörfúrógép");
	psl::string x3 = _T("кошка 日本国 أَبْجَدِيَّة عَرَبِيَّة‎中文");
	psl::string x4 = _T("你爱我");*/
	//_setmode(_fileno(stdout), _O_U16TEXT);
	//_setmode(_fileno(stdout), _O_U8TEXT);
#endif

	assembler::generators::shader shader_gen {};
	assembler::generators::models model_gen {};
	assembler::generators::meta meta_gen {};
	assembler::generators::project project_gen {};

	psl::cli::pack generator_pack {
	  value<pack> {"shader", "glsl to spir-v compiler", {"shader", "s"}, std::move(shader_gen.pack())},
	  value<pack> {"model", "model importer", {"models", "g"}, std::move(model_gen.pack())},
	  value<pack> {"library", "meta library generator", {"library", "l"}, meta_gen.library_pack()},
	  value<pack> {"meta", "meta file generator", {"meta", "m"}, meta_gen.meta_pack()},
	  value<pack> {"project", "project file generator", {"project", "p"}, project_gen.pack()},
	};

	psl::cli::pack root {
	  value<bool> {
		"interactive-mode", "allows continuous commands to be sent", {"interactive", "i"}, default_interactive},
	  value<bool> {"exit", "quits the application (only when interactive mode is on)", {"exit", "quit", "q"}, false},
	  value<std::string> {"graphical assembler",
						  "launches the graphical editor (only one can be created)",
						  {"geditor", "gassembler"},
						  "",
						  true,
						  {{"vulkan", "gles"}}},
	  value<pack> {"generator", "generator for various data files", {"generate", "g"}, generator_pack},
	  value<pack> {
		"version", "prints the version information", {"version", "v"}, psl::cli::pack {[](psl::cli::pack& pack) {
			assembler::log->info("paradigm version: {}", VERSION);
		}}}};

	std::thread geditor_thread;

	if(root["interactive-mode"]->as<bool>().get()) {
		assembler::log->info(
		  "welcome to assembler interactive mode, use -h or --help to get information on the commands.\nyou can also "
		  "pass "
		  "the specific command (or its chain) after --help to get more information of that specific "
		  "command, such as '--help generate shader'.\n");
	}


	do {
		if(root["graphical assembler"]->as<std::string>().get() != "") {
			if(gBackend == graphics_backend::undefined) {
				gBackend = parse(root["graphical assembler"]->as<std::string>().get());
				if(geditor_thread.joinable())
					geditor_thread.join();
				geditor_thread = std::thread {launch_gassembler, gBackend.load()};
			} else {
				assembler::log->error("ERROR: a graphical editor is already running");
			}
		}
		psl::array<psl::string_view> commands = psl::utility::string::split(get_input(argc, argv), ("|"));
		try {
			root.parse(commands);
		} catch(std::runtime_error const& re) {
			std::cerr << "Runtime error: " << re.what() << std::endl;
		} catch(std::exception const& ex) {
			std::cerr << "Exception occurred: " << ex.what() << std::endl;
		} catch(...) {
			std::cerr << "Unknown failure occurred. Exiting the app" << std::endl;
			break;
		}
	} while(!root["exit"]->as<bool>().get() && root["interactive-mode"]->as<bool>().get());
	should_exit = true;
	if(geditor_thread.joinable())
		geditor_thread.join();

	return 0;
}
