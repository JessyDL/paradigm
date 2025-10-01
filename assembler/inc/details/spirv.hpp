#pragma once
#include <core/meta/shader.hpp>
#include <optional>
#include <psl/ustring.hpp>

namespace tools {
namespace _internal {
	struct glslang_manager_t {
		glslang_manager_t();
		~glslang_manager_t();
	};
	extern glslang_manager_t glslang_manager;

}	 // namespace _internal
enum class shader_stage_t : uint8_t { unknown = 100, vert = 0, tesc = 1, tese = 2, geom = 3, frag = 4, comp = 5 };
inline constexpr psl::string_view shader_stage_str[] {"vert", ("tesc"), ("tese"), ("geom"), ("frag"), ("comp")};

struct glsl_compile_result_t {
	constexpr glsl_compile_result_t() = default;
	constexpr glsl_compile_result_t(bool value) : success(value) {};

	constexpr operator bool() const noexcept {
		return success;
	}
	struct message_t {
		psl::string8_t message {};
		bool error {false};
	};
	psl::array<message_t> messages {};
	struct shader_t {
		psl::array<core::meta::shader::attribute> inputs {};
		psl::array<core::meta::shader::attribute> outputs {};
		psl::array<core::meta::shader::descriptor> descriptors {};
		core::gfx::shader_stage stage {core::gfx::shader_stage {0}};
	} shader {};
	psl::string8_t spirv {};
	psl::string8_t gles {};
	bool success {false};
};

glsl_compile_result_t glsl_compile(psl::string_view source,
								   shader_stage_t type,
								   bool optimize					  = false,
								   std::optional<size_t> gles_version = std::nullopt);
}	 // namespace tools