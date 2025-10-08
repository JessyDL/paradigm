#include "psl/reflection.hpp"

#include <fmt/core.h>

#include <chrono>
#include <string>
#include <string_view>

namespace core::gfx {
enum class graphics_backend { undefined, vulkan, gles, webgpu };
enum class format_t {
	undefined,
	r8g8b8a8_unorm,
	r16g16b16a16_unorm,
	r32g32b32a32_float,
};
}	 // namespace core::gfx


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
	fmt::println("Printing all enum values for enum '{}'", std::meta::identifier_of(^^E));

	if constexpr(Enumerable)
		template for(constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E)))
		  fmt::println("    {} = {}", std::meta::identifier_of(e), std::to_underlying([:e:]));
}


struct[[= serialization::container_t {}]] TextureMeta {
	void print() {
		fmt::println("TextureMeta: {}", to_string());
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
		fmt::println("PrivTextureMeta: width={}, height={}, format={}, mipmaps={}",
					 m_Width,
					 height,
					 enum_to_string(format),
					 mipmaps);
	}

  private:
	[[= serialization::field_t {.optional = false, .version = 0}, = serialization::name_t<"width"> {}]] int m_Width;
	[[= serialization::field_t {}, = serialization::alternative_names_t<"heighthhhh", "height2_0"> {}]] int height;
	[[= serialization::field_t {.optional = true}]] core::gfx::format_t format = core::gfx::format_t::r8g8b8a8_unorm;
	[[= serialization::field_t {}]] bool mipmaps							   = false;
};

struct ContainerTest {
	friend struct serialization::accessor;
	void print() {
		fmt::println("ContainerTest: name={}, meta={{{}}}", name, meta.to_string());
	}

  private:
	[[= serialization::field_t {}]] std::string name;
	[[= serialization::field_t {}]] TextureMeta meta;
};

struct[[= serialization::container_t {.mode = serialization::mode_t::opt_out}]] OptOutTest {
	void print() {
		fmt::println("OptOutTest: x={}, y={}", x, y);
	}

	int x;
	int y;
};


struct ComplexType {
	friend struct serialization::accessor;

  public:
	ComplexType(int w, int h) : width(w), height(h) {}
	ComplexType(serialization::IsSerializationInstance<ComplexType> auto&& instance)
		: width(instance.width), height(instance.height) {}

	void print() {
		fmt::println("ComplexType: width={}, height={}", width, height);
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
	fmt::println("Texture Meta: width={}, height={}, format={}, mipmaps={}",
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

void opt_out_test() {
	auto opt_out = serialization::parse<OptOutTest>({{"x", "10"}, {"y", "20"}});
	opt_out.print();
}

int main(int argc, char** argv) {
	container_test();
	complex_type();
	texture_meta();
	priv_texture_meta();
	opt_out_test();

	fmt::println("Meta info:\n{}", serialization::meta_info<TextureMeta>().to_string());

	print_all_enum_values<core::gfx::graphics_backend>();
	auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	auto val		  = to_enum<core::gfx::graphics_backend>(current_time % 2 == 0 ? "vulkan" : "gles");

	if(val)
		fmt::println("Converted string to enum: {}", int(*val));
	else
		fmt::println("Could not convert string to enum");
	return 0;
}
