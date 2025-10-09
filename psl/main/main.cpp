#include "psl/reflection.hpp"

#include <fmt/core.h>

#include <chrono>
#include <string>
#include <string_view>

#include "psl/serialization/parser.hpp"

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

struct TextureMeta {
	void print() {
		fmt::println("TextureMeta: {}", to_string());
	}

	auto to_string() const -> std::string {
		return fmt::format(
		  "width={}, height={}, format={}, mipmaps={}", width, height, enum_to_string(format), mipmaps);
	}

	[[= psl::ser::field()]] int width;
	[[= psl::ser::field(), = psl::ser::alternative_names_t<"heighthhhh", "height2_0"> {}]] int height;
	[[= psl::ser::field({.optional = true})]] core::gfx::format_t format = core::gfx::format_t::r8g8b8a8_unorm;
	[[= psl::ser::field()]] bool mipmaps								 = false;
};

struct PrivTextureMeta {
	friend struct psl::ser::accessor;
	void print() {
		fmt::println("PrivTextureMeta: width={}, height={}, format={}, mipmaps={}",
					 m_Width,
					 height,
					 enum_to_string(format),
					 mipmaps);
	}

  private:
	[[= psl::ser::field({.optional = false, .version = 0}), = psl::ser::name_t<"width"> {}]] int m_Width;
	[[= psl::ser::field(), = psl::ser::alternative_names_t<"heighthhhh", "height2_0"> {}]] int height;
	[[= psl::ser::field({.optional = true})]] core::gfx::format_t format = core::gfx::format_t::r8g8b8a8_unorm;
	[[= psl::ser::field()]] bool mipmaps								 = false;
};

struct ContainerTest {
	friend struct psl::ser::accessor;
	void print() {
		fmt::println("ContainerTest: name={}, meta={{{}}}", name, meta.to_string());
	}

  private:
	[[= psl::ser::field()]] std::string name;
	[[= psl::ser::field()]] TextureMeta meta;
};

struct[[= psl::ser::container_t {.mode = psl::ser::mode_t::opt_out}]] OptOutTest {
	void print() {
		fmt::println("OptOutTest: x={}, y={}", x, y);
	}

	int x;
	int y;
	std::vector<int> vec;
};

struct ComplexType {
	friend struct psl::ser::accessor;

  public:
	ComplexType(int w, int h) : width(w), height(h) {}
	ComplexType(psl::ser::IsSerializationInstance<ComplexType> auto&& instance)
		: width(instance.width), height(instance.height) {}

	void print() {
		fmt::println("ComplexType: width={}, height={}", width, height);
	}

  private:
	[[= psl::ser::field()]] int width;
	[[= psl::ser::field()]] int height;
	size_t depth = 1;
};

psl::par::type_database_t g_Database;

void complex_type() {
	auto complex_type = psl::ser::parse<ComplexType>({{"width", "999"}, {"height", "768"}});
	complex_type.print();
}

void texture_meta() {
	auto texture_meta = psl::ser::parse<TextureMeta>({{"width", "1024"}, {"height2_0", "768"}, {"mipmaps", "true"}});
	fmt::println("Texture Meta: width={}, height={}, format={}, mipmaps={}",
				 texture_meta.width,
				 texture_meta.height,
				 enum_to_string(texture_meta.format),
				 texture_meta.mipmaps);
}

void priv_texture_meta() {
	auto priv_texture_meta = psl::ser::parse<PrivTextureMeta>(
	  {{"width", "102"}, {"height2_0", "768"}, {"mipmaps", "true"}, {"format", "format_t::r16g16b16a16_unorm"}});
	priv_texture_meta.print();
}

void container_test() {
	auto container = psl::ser::parse<ContainerTest>(
	  {{"name", "MyTexture"}, {"meta.width", "2048"}, {"meta.height", "1024"}, {"meta.mipmaps", "true"}});
	container.print();
}

void opt_out_test() {
	// auto opt_out = psl::ser::parse<OptOutTest>({{"x", "10"}, {"y", "20"}});
	// opt_out.print();
}

int main(int argc, char** argv) {
	container_test();
	complex_type();
	texture_meta();
	priv_texture_meta();
	opt_out_test();
	g_Database.register_type<PrivTextureMeta>();
	g_Database.register_type<ContainerTest>();
	g_Database.register_type<OptOutTest>();
	g_Database.register_type<ComplexType>();


	g_Database.print();
	return 0;
}
