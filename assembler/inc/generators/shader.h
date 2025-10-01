#pragma once
#include "cli/value.h"
#include "psl/string_utils.hpp"
#include "psl/terminal_utils.hpp"
#include "psl/timer.hpp"
#include <set>

namespace assembler {
class pathstring;
}
namespace assembler::generators {
class shader {
	template <typename T>
	using cli_value = psl::cli::value<T>;

  public:
	struct file_data {
		uint64_t last_modified;
		psl::string content;
		psl::string filename;
		std::vector<std::pair<psl::string, size_t>> includes;
		uint8_t type;
	};

  private:
	std::unordered_map<psl::string, size_t> m_KnownTypes {{("bool"), 1},
														  {("int"), 4},
														  {("uint"), 4},
														  {("float"), 4},
														  {("double"), 8},
														  {("sampler"), 0},
														  {("sampler2D"), 0},
														  {("sampler3D"), 0},
														  {("samplerCube"), 0}};

	std::unordered_map<psl::string, size_t> m_TypeToFormat {{("vec4"), 72},
															{("mat4"), 72},
															{("vec3"), 69},
															{("mat3"), 69},
															{("vec2"), 66},
															{("mat2"), 66}};

	std::unordered_map<psl::string, size_t> m_DescriptorType {{("ubo"), 6}, {("ssbo"), 7}, {("combined_sampler"), 1}};

  public:
	shader() {
		std::unordered_map<psl::string, size_t> vectoral_types {
		  {("b"), 1}, {("i"), 4}, {("u"), 4}, {(""), 4}, {("d"), 8}};
		for(size_t i = 2; i <= 4; ++i) {
			for(auto const& vType : vectoral_types) {
				psl::string fulltype   = vType.first + ("vec") + psl::from_string8_t(psl::utility::to_string(i));
				size_t size			   = vType.second * i;
				m_KnownTypes[fulltype] = size;
			}
		}

		vectoral_types = std::unordered_map<psl::string, size_t> {{(""), 4}, {("d"), 8}};
		for(size_t i = 2; i <= 4; ++i) {
			for(auto const& vType : vectoral_types) {
				psl::string fulltype   = vType.first + ("mat") + psl::from_string8_t(psl::utility::to_string(i));
				size_t size			   = vType.second * i * i;
				m_KnownTypes[fulltype] = size;
			}
		}

		for(size_t n = 2; n <= 4; ++n) {
			for(size_t m = 2; m <= 4; ++m) {
				for(auto const& vType : vectoral_types) {
					psl::string fulltype = vType.first + ("mat") + psl::from_string8_t(psl::utility::to_string(n)) +
										   ("x") + psl::from_string8_t(psl::utility::to_string(m));
					size_t size			   = vType.second * n * m;
					m_KnownTypes[fulltype] = size;
				}
			}
		}
	}

	psl::cli::pack pack() {
		return psl::cli::pack {
		  std::bind(&assembler::generators::shader::on_generate, this, std::placeholders::_1),
		  cli_value<psl::string> {"input", "location of the input file", {"input", "i"}, "", false},
		  cli_value<psl::string> {"output", "location where to place the file", {"output", "o"}, "", true},
		  cli_value<bool> {"overwrite",
						   "should the output file be overwritten if it already exists?",
						   {"overwrite", "f", "force"},
						   true,
						   true},
		  cli_value<bool> {"optimize", "should we optimize the output", {"optimize", "O"}, false, true},
		  cli_value<bool> {"compiled glsl",
						   "should we instead print the complete constructed GLSL (with includes etc..)?",
						   {"glsl"},
						   false,
						   true},
		  cli_value<bool> {"verbose",
						   "should verbose information be printed about the internal process?",
						   {"verbose", "v"},
						   false,
						   true},
		  cli_value<std::vector<psl::string>> {
			"types", "graphics types to support", {"types"}, {"vulkan", "gles"}, true, {{"vulkan", "gles"}}}};
	}

	bool generate(assembler::pathstring ifile,
				  assembler::pathstring ofile,
				  bool compiled_glsl,
				  bool optimize,
				  psl::array<psl::string> types);

  private:
	bool parse(file_data& data);
	bool cache_file(psl::string const& file);

	psl::string construct(file_data const& fdata, std::set<psl::string_view>& includes) const;

	void on_generate(psl::cli::pack& pack);

	std::unordered_map<psl::string, file_data> m_Cache;	   // Caches the files read. Note that the filepath
														   // will always be Unix style regardless of input
	bool m_Verbose {false};
};
}	 // namespace assembler::generators
