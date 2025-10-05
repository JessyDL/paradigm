#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "psl/application_utils.hpp"
#include "psl/library.hpp"
#include "psl/platform_utils.hpp"

#include "core/data/window.hpp"
#include "core/gfx/context.hpp"
#include "core/gfx/render_graph.hpp"
#include "core/gfx/swapchain.hpp"
#include "core/logging.hpp"
#include "core/os/context.hpp"
#include "core/os/surface.hpp"
#include "core/paradigm.hpp"
#include "core/resource/resource.hpp"

// This is the entry point for our agnostic renderer. The following code works the same for all platforms and graphics
// backends.
// note for android you will need to look at the android example, as it requires a different entry point.
int entry(core::gfx::graphics_backend backend, core::os::context& os_context) {
	core::log->info("Starting the application");
	core::log->info("creating a '{}' backend", core::gfx::graphics_backend_str(backend));

	// Here we initialize the cache. The cache is a central place where all resources are stored.
	// The cache internally stores handles to resources, this means if resources go out of scope
	// the cache will automatically clean up the resources if requested.
	// Additionally resources can store references to one another, to protect you from deleting
	// resources that are still in use, and to see where leaks are coming from.
	//
	// Normally we'd load a library from a file, but in this case we're using a empty library.
	core::log->info("creating cache");
	core::resource::cache_t cache {psl::meta::library {}};
	core::log->info("cache created");

	// We create a window data object. This object contains information about the window, such as its name, width,
	// height, etc.. This object is used to create a surface, which is provided by your OS. Most objects have direct
	// counterpoles in the core::data namespace. Such as textures. This namespace contains all the data that is used to
	// create resources, or store them. They are the descriptors for the resources.
	auto window_data = cache.create<core::data::window>();
	window_data->name(APPLICATION_FULL_NAME + " { " + core::gfx::graphics_backend_str(backend) + " }");
	auto surface_handle = cache.create<core::os::surface>(window_data);
	if(!surface_handle) {
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	// Next up we initialize the graphics context. This is the object that is used to create the graphics backend.
	// The graphics context is used to create the swapchain, which is the object that is used to present the rendered
	// images to the screen.
	auto context_handle = cache.create<core::gfx::context>(backend, psl::string8_t {APPLICATION_NAME}, surface_handle);
	auto swapchain_handle = cache.create<core::gfx::swapchain>(surface_handle, context_handle, os_context);

	// We set the clear color of the swapchain. This is the color that is used to clear the screen before rendering.
	// in this example's case this means the screen will be fully magenta.
	swapchain_handle->clear_color({1.0f, 0.0f, 1.0f, 1.0f});

	// All rendering internally is done through a graph, this helps in tracking when and where resources are used and
	// how they are used. So we know where we need to enter the sync points in the rendering. For now it's just
	// important to know that the render graph consists out of N draw/compute passes, and those passes contain the
	// actual objects you wish to render (such as meshes, textures, etc..).
	core::gfx::render_graph renderGraph {};

	// We create a drawpass for the swapchain. This drawpass is used to render to the swapchain.
	auto swapchain_pass = renderGraph.create_drawpass(context_handle, swapchain_handle);

	while(os_context.tick() && surface_handle->tick()) {
		// Finally we present, this will kick off all drawpasses in the render graph, and present the final image to the
		// screen.
		renderGraph.present();
	}
	return 0;
}
#include <experimental/meta>
#include <fmt/ranges.h>
#include <ranges>
#include <utility>

#include "psl/reflection.hpp"

template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
	requires std::is_enum_v<E>
constexpr std::string_view enum_to_string(E value) {
	if constexpr(Enumerable) {
		template for(constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
			if(value == [:e:]) {
				return std::meta::identifier_of(e);
			}
		}
	}
	return "<unnamed>";
}

template <typename T>
consteval std::string_view stringify() {
	return std::meta::identifier_of(^^T);
}

template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
	requires std::is_enum_v<E>
constexpr std::string stringify(E value, bool with_type = true) {
	if(with_type) {
		return std::string {std::meta::identifier_of(^^E)} + "::" + enum_to_string(value);
	}
	return std::string {enum_to_string(value)};
}

template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
	requires std::is_enum_v<E>
constexpr std::optional<E> to_enum(std::string_view name) {
	if(auto split = name.find("::"); split != std::string_view::npos) {
		if(std::meta::identifier_of(^^E) != name.substr(0, split)) {
			return std::nullopt;
		}
		name = name.substr(split + 2);
	}

	template for(constexpr auto e : std::define_static_array(
				   std::meta::enumerators_of(^^E))) if(name == std::meta::identifier_of(e)) return [:e:];

	return std::nullopt;
}

template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
	requires std::is_enum_v<E>
void print_all_enum_values() {
	core::log->info("Printing all enum values for enum '{}'", std::meta::identifier_of(^^E));

	if constexpr(Enumerable)
		template for(constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E)))
		  core::log->info("    {} = {}", std::meta::identifier_of(e), std::to_underlying([:e:]));
}


struct[[= serialization::container_t {}, = serialization::name_t<"OtherName"> {}]] TextureMeta {
	void print() {
		core::log->info("TextureMeta: {}", to_string());
	}

	auto to_string() const -> std::string {
		return fmt::format(
		  "width={}, height={}, format={}, mipmaps={}", width, height, enum_to_string(format), mipmaps);
	}

	[[= serialization::field_t {}]] int width;
	[[= serialization::field_t {}, = serialization::alternative_names_t<"heighthhhh", "height2_0"> {}]] int height;
	[[= serialization::field_t {.optional = true}]] core::gfx::format_t format = core::gfx::format_t::r8g8b8a8_unorm;
	[[= serialization::field_t {}]] bool mipmaps							   = false;
};

struct PrivTextureMeta {
	friend struct serialization::accessor;
	void print() {
		core::log->info("PrivTextureMeta: width={}, height={}, format={}, mipmaps={}",
						m_Width,
						height,
						enum_to_string(format),
						mipmaps);
	}

  private:
	[[= serialization::field_t {}, = serialization::name_t<"width"> {}]] int m_Width;
	[[= serialization::field_t {}, = serialization::alternative_names_t<"heighthhhh", "height2_0"> {}]] int height;
	[[= serialization::field_t {.optional = true}]] core::gfx::format_t format = core::gfx::format_t::r8g8b8a8_unorm;
	[[= serialization::field_t {}]] bool mipmaps							   = false;
};

struct ContainerTest {
	friend struct serialization::accessor;
	void print() {
		core::log->info("ContainerTest: name={}, meta={{{}}}", name, meta.to_string());
	}

  private:
	[[= serialization::field_t {}]] std::string name;
	[[= serialization::field_t {}]] TextureMeta meta;
};


struct ComplexType {
	friend struct serialization::accessor;

  public:
	ComplexType(int w, int h) : width(w), height(h) {}
	ComplexType(serialization::IsSerializationInstance<ComplexType> auto&& instance)
		: width(instance.width), height(instance.height) {}

	void print() {
		core::log->info("ComplexType: width={}, height={}", width, height);
	}

  private:
	[[= serialization::field_t {}]] int width;
	[[= serialization::field_t {}]] int height;
};


struct TextureMetaInstance : serialization::serialize_type_t<TextureMeta> {
	int extra_field;
};

void complex_type() {
	auto complex_type = serialization::parse<ComplexType>({{"width", "999"}, {"height", "768"}});
	complex_type.print();
}

void texture_meta() {
	auto texture_meta =
	  serialization::parse<TextureMeta>({{"width", "1024"}, {"height2_0", "768"}, {"mipmaps", "true"}});
	core::log->info("Texture Meta: width={}, height={}, format={}, mipmaps={}",
					texture_meta.width,
					texture_meta.height,
					enum_to_string(texture_meta.format),
					texture_meta.mipmaps);
}

void priv_texture_meta() {
	auto priv_texture_meta = serialization::parse<PrivTextureMeta>(
	  {{"width", "102"}, {"height2_0", "768"}, {"mipmaps", "true"}, {"format", "format_t::r16g16b16a16_unorm"}});
	priv_texture_meta.print();
}

void container_test() {
	auto container = serialization::parse<ContainerTest>(
	  {{"name", "MyTexture"}, {"meta.width", "2048"}, {"meta.height", "1024"}, {"meta.mipmaps", "true"}});
	container.print();
}

int main(int argc, char** argv) {
	core::initialize_loggers();

	container_test();
	complex_type();
	texture_meta();
	priv_texture_meta();

	core::log->info("Meta info:\n{}", serialization::meta_info<TextureMeta>().to_string());

	print_all_enum_values<core::gfx::graphics_backend>();
	auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	auto val		  = to_enum<core::gfx::graphics_backend>(current_time % 2 == 0 ? "vulkan" : "gles");

	if(val)
		core::log->info("Converted string to enum: {}", int(*val));
	else
		core::log->info("Could not convert string to enum");
	return 0;

	// This decodes the flags that can be passed to the application. The flags are used to determine the graphics
	// backend.
	if(argc > 0) {
		core::log->info("Received the cli args:");
		for(auto i = 0; i < argc; ++i) core::log->info(argv[i]);
	}
	auto backend = [](int argc, char* argv[]) {
		for(auto i = 0; i < argc; ++i) {
			std::string_view text {argv[i]};
			if(text == "--vulkan") {
#if defined(PE_VULKAN)
				return core::gfx::graphics_backend::vulkan;
#else
				throw std::runtime_error("Requested a Vulkan backend, but application does not support Vulkan");
#endif
			} else if(text == "--gles") {
#if defined(PE_GLES)
				return core::gfx::graphics_backend::gles;
#else
				throw std::runtime_error("Requested a GLES backend, but application does not support GLES");
#endif
			} else if(text == "--webgpu") {
#if defined(PE_WEBGPU)
				return core::gfx::graphics_backend::webgpu;
#else
				throw std::runtime_error("Requested a WebGPU backend, but application does not support WebGPU");
#endif
			}
		}
#if defined(PE_VULKAN)
		return core::gfx::graphics_backend::vulkan;
#elif defined(PE_WEBGPU)
		return core::gfx::graphics_backend::webgpu;
#elif defined(PE_GLES)
		return core::gfx::graphics_backend::gles;
#endif
		return core::gfx::graphics_backend::undefined;
	}(argc, argv);

	// We create a context object that will be used to manage the OS context.
	// In many cases this is empty, but in some cases it contains vital information.
	// See 'core/os/context_android.cpp' as example.
	core::os::context context {};
	return entry(backend, context);
}
