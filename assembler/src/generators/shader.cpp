#include "generators/shader.hpp"
#include "core/gfx/types.hpp"
#include "details/spirv.hpp"
#include "psl/application_utils.hpp"
#include "psl/library.hpp"
#include "psl/meta.hpp"
#include "psl/platform_utils.hpp"
#include "psl/serialization/serializer.hpp"
#include "stdafx.hpp"
#include "utf8.h"
#include "utils.hpp"
#include <filesystem>
#include <iostream>

#if defined(AS_ENABLE_WGSL)
	#include "tint/tint.h"
#endif
// todo we should figure out the bindings dynamically instead of having them written into the files, and then
// potentially safeguard the user from accidentally merging shaders together that do not work together

using namespace assembler::generators;
using namespace psl;


bool shader::parse(file_data& data) {
	auto inc_n = data.content.find(("#include"));
	while(inc_n != psl::string_view::npos) {
		auto endline_n	  = data.content.find(("\n"), inc_n);
		auto file_begin_n = data.content.find(("\""), inc_n);
		if(file_begin_n > endline_n)
			file_begin_n = data.content.find(("\'"), inc_n);
		if(file_begin_n > endline_n) {
			psl::string_view current_view {data.content.data(), endline_n};
			auto line_number = utility::string::count(current_view, ("\n"));
			utility::terminal::set_color(utility::terminal::color::RED);
			assembler::log->error("ERROR: could not deduce the input file's path.");
			assembler::log->error("\tthe include's opening directive was not found, are you missing a \" or ' ?");
			assembler::log->error("\tat line '" + utility::to_string(line_number) + "' of '" + data.filename);
			utility::terminal::set_color(utility::terminal::color::WHITE);
			return false;
		}

		file_begin_n += 1;
		auto file_end_n = data.content.find(("\""), file_begin_n);
		if(file_end_n > endline_n)
			file_end_n = data.content.find(("\'"), file_begin_n);

		if(file_end_n > endline_n) {
			psl::string_view current_view {data.content.data(), endline_n};
			auto line_number = utility::string::count(current_view, ("\n"));
			utility::terminal::set_color(utility::terminal::color::RED);
			assembler::log->error("ERROR: could not deduce the input file's path.");
			assembler::log->error(
			  "\tthe include directive was opened, but not closed, please add a \" or ' at the end of the "
			  "line.");
			assembler::log->error("\tat line '" + utility::to_string(line_number) + "' of '" + data.filename);
			utility::terminal::set_color(utility::terminal::color::WHITE);
			return false;
		}
		psl::string_view include {&data.content[file_begin_n], file_end_n - file_begin_n};
		psl::string_view include_def {&data.content[file_begin_n], file_end_n - file_begin_n};
		psl::string include_path = data.filename.substr(0, data.filename.find_last_of(("/")));

		while(include.substr(0, 3) == ("../")) {
			include_path = include_path.substr(0, include_path.find_last_of(("/")));
			include		 = include.substr(3);
		}
		include_path = include_path + ('/') + include;
		include_path = utility::platform::file::to_platform(include_path);
		if(!std::filesystem::exists(include_path)) {
			psl::string_view current_view {data.content.data(), endline_n};
			auto line_number = utility::string::count(current_view, ("\n"));
			utility::terminal::set_color(utility::terminal::color::RED);
			assembler::log->error("ERROR: include file not found.\n");
			assembler::log->error("\tit was defined as '" + include_def + "' and transformed into '" + include_path +
								  "'");
			assembler::log->error("\tat line '" + utility::to_string(line_number) + "' of '" + data.filename);
			utility::terminal::set_color(utility::terminal::color::WHITE);
			return false;
		}

		if(!cache_file(include_path))
			return false;

		data.includes.emplace_back(std::make_pair(include_path, inc_n));
		data.content.erase(std::begin(data.content) + inc_n, std::begin(data.content) + endline_n);

		inc_n = data.content.find(("#include"), inc_n);
	}

	return true;
}

bool shader::cache_file(psl::string const& file) {
	auto it = m_Cache.find(file);
	if(it != m_Cache.end() &&
	   std::filesystem::last_write_time(file).time_since_epoch().count() == it->second.last_modified) {
		if(m_Verbose) {
			assembler::log->info("{0} was found in the cache", file);
			assembler::log->info("\tchecking its dependencies..\n");
		}
		for(auto const& include : it->second.includes) {
			cache_file(include.first);
		}
		return true;
	} else {
		if(auto res = utility::platform::file::read(file); !res) {
			utility::terminal::set_color(utility::terminal::color::RED);
			assembler::log->error("ERROR: missing file, no file was found at the location: " + file);
			utility::terminal::set_color(utility::terminal::color::WHITE);
			return false;
		} else if(!utf8::is_valid(res.value().begin(), res.value().end())) {
			utility::terminal::set_color(utility::terminal::color::RED);
			assembler::log->error("ERROR: the encoding was not valid UTF-8 for the file: " + file);
			utility::terminal::set_color(utility::terminal::color::WHITE);
			return false;
		} else {
			if(m_Verbose)
				assembler::log->info(file + " is being loaded into the cache\n");
			file_data fdata;
			fdata.content		= std::move(res.value());
			fdata.last_modified = std::filesystem::last_write_time(file).time_since_epoch().count();
			fdata.filename		= file;
			auto& data = m_Cache[file] = fdata;
			parse(data);
		}
	}
	return true;
}

psl::string shader::construct(file_data const& fdata, std::set<psl::string_view>& includes) const {
	psl::string res = fdata.content;

	size_t offset = 0u;
	for(auto const& inc : fdata.includes) {
		if(includes.find(inc.first) == std::end(includes)) {
			includes.insert(inc.first);
			auto it = m_Cache.find(inc.first);
			if(it == std::end(m_Cache)) {
				assembler::log->error("the include" + inc.first + " is not present in the cache.");
				continue;
			}
			auto sub_res {construct(it->second, includes)};
			res.insert(inc.second + offset, sub_res);
			offset += sub_res.size();
		}
	}
	return res;
}

bool shader::generate(assembler::pathstring ifile,
					  assembler::pathstring ofile,
					  bool compiled_glsl,
					  bool optimize,
					  psl::array<psl::string> types) {
	auto directory = utility::platform::file::to_platform(ofile->substr(0, ofile->find_last_of("/")));
	if(!utility::platform::directory::exists(directory))
		utility::platform::directory::create(directory, true);

	auto index	   = ifile->find_last_of(("."));
	auto extension = ifile->substr(index + 1);

	tools::shader_stage_t type = tools::shader_stage_t::unknown;
	if(extension == ("vert"))
		type = tools::shader_stage_t::vert;
	else if(extension == ("frag"))
		type = tools::shader_stage_t::frag;
	else if(extension == ("tesc"))
		type = tools::shader_stage_t::tesc;
	else if(extension == ("geom"))
		type = tools::shader_stage_t::geom;
	else if(extension == ("comp"))
		type = tools::shader_stage_t::comp;
	else if(extension == ("tese"))
		type = tools::shader_stage_t::tese;

	if(type == tools::shader_stage_t::unknown)
		return false;

	auto ofile_meta = ofile.platform() + "-" + tools::shader_stage_str[(uint8_t)type] + "." + ::meta::META_EXTENSION;

	psl::timer timer;

	try {
		if(!cache_file(ifile.platform())) {
			assembler::log->error(
			  "something went wrong when loading the file in the cache, please consult the output to see why");
			return false;
		}
	} catch(std::exception e) {
		assembler::log->error("exception happened during caching! \n" + psl::string8_t(e.what()));
	}

	auto const& data = m_Cache[ifile.platform()];

	// emplace include data
	psl::string content = data.content;
	std::set<psl::string_view> includes;
	size_t offset = 0u;
	for(auto const& inc : data.includes) {
		if(includes.find(inc.first) == std::end(includes)) {
			includes.insert(inc.first);
			auto it = m_Cache.find(inc.first);
			if(it == std::end(m_Cache)) {
				assembler::log->error("the include" + inc.first + " is not present in the cache.");
				continue;
			}
			auto sub_res {construct(it->second, includes)};
			content.insert(std::begin(content) + inc.second + offset, std::begin(sub_res), std::end(sub_res));
			offset += sub_res.size();
		}
	}

	std::optional<size_t> gles_version;
	if(std::find(std::begin(types), std::end(types), "gles") != std::end(types)) {
		gles_version = (type == tools::shader_stage_t::comp) ? 310 : 300;
	}

	if(auto result = tools::glsl_compile(content, (tools::shader_stage_t)type, optimize, gles_version); !result) {
		assembler::log->error("errors while generating:");
		for(auto const& message : result.messages) {
			if(message.error) {
				assembler::log->error(message.message);
			} else {
				assembler::log->info(message.message);
			}
		}

		return false;
	} else {
		for(auto const& message : result.messages) {
			if(message.error) {
				assembler::log->error(message.message);
			} else {
				assembler::log->info(message.message);
			}
		}

		auto output_file = ofile.platform();
		if(!result.spirv.empty() && !utility::platform::file::write(output_file + ".spv", result.spirv)) {
			assembler::log->error("failed to write the spirv to: {}", output_file + ".spv");
		}
		if(!result.gles.empty() && !utility::platform::file::write(output_file + ".gles", result.gles)) {
			assembler::log->error("failed to write the gles to: {}", output_file + ".gles");
		}
#if defined(AS_ENABLE_WGSL)
		if(!result.spirv.empty()) {
			auto spirv = std::vector<uint32_t>(result.spirv.size() / sizeof(uint32_t));
			std::memcpy(spirv.data(), result.spirv.data(), result.spirv.size());
			auto spirvReadOption = tint::spirv::reader::Options {};
			auto tintIr			 = tint::spirv::reader::Read(spirv, spirvReadOption);
			if(tintIr.Diagnostics().ContainsErrors()) {
				assembler::log->error("failed to convert the spirv to tint-ir: {}", tintIr.Diagnostics().Str());
				return false;
			}
			auto wgslOptions = tint::wgsl::writer::Options();
			auto tintWgslRes = tint::wgsl::writer::Generate(tintIr, wgslOptions);
			if(tintWgslRes != tint::Success) {
				assembler::log->error("failed to convert the tint-ir to wgsl: {}", tintWgslRes.Failure().reason);
				return false;
			}
			auto wgsl = tintWgslRes.Move();
			if(!utility::platform::file::write(output_file + ".wgsl", wgsl.wgsl)) {
				assembler::log->error("failed to write the wgsl to: {}", output_file + ".wgsl");
			}
		} else {
			assembler::log->error("no spirv was generated, so no wgsl can be generated");
		}
#endif
		if(result.shader.stage != core::gfx::shader_stage {0}) {
			auto output_meta_file = output_file + "." + psl::meta::META_EXTENSION;
			psl::UID uid		  = psl::UID::generate();
			if(utility::platform::file::exists(output_meta_file)) {
				meta::file* original = nullptr;
				serialization::serializer temp_s;
				temp_s.deserialize<serialization::decode_from_format>(original, output_meta_file);
				uid = original->ID();
			}
			core::meta::shader shaderMeta {uid};
			shaderMeta.inputs(result.shader.inputs);
			shaderMeta.outputs(result.shader.outputs);
			shaderMeta.descriptors(result.shader.descriptors);
			shaderMeta.stage(result.shader.stage);
			serialization::serializer s;
			format::container container;
			s.serialize<serialization::encode_to_format>(&shaderMeta, container);
			utility::platform::file::write(output_meta_file, psl::from_string8_t(container.to_string()));
		}
	}
	return true;
}

bool valid(assembler::pathstring const& path) {
	auto const extension = path->substr(path->find_last_of('.'));
	return (extension == (".vert") || extension == (".frag") || extension == (".tesc") || extension == (".geom") ||
			extension == (".comp") || extension == (".tese"));
}

void shader::on_generate(psl::cli::pack& pack) {
	//-g -s -i "C:\Projects\github\example_data\source\shaders/*" -o "C:\Projects\github\example_data\data\shaders/" -O
	// "C:\Projects\data_old\Shaders\test_output\default-glsl.vert" --glsl -v

	auto ifile		   = assembler::pathstring {pack["input"]->as<psl::string>().get()};
	auto ofile		   = assembler::pathstring {pack["output"]->as<psl::string>().get()};
	auto overwrite	   = pack["overwrite"]->as<bool>().get();
	auto compiled_glsl = pack["compiled glsl"]->as<bool>().get();
	auto optimize	   = pack["optimize"]->as<bool>().get();
	m_Verbose		   = pack["verbose"]->as<bool>().get();
	auto types		   = pack["types"]->as<std::vector<psl::string>>().get();

	auto files = assembler::get_files(ifile, ofile);

	size_t success = 0;
	for(auto const& file : files) {
		if(!valid(file.first)) {
			assembler::log->info("skipping {0}", file.first.platform());
			continue;
		}
		psl::string_view output = file.second;
		if(ofile->empty() || ifile->at(ifile->size() - 1) == '*') {
			if(size_t last_dot = output.find_last_of('.'); last_dot != std::string::npos) {
				output = psl::string_view {file.second.data().data(), last_dot};
			}
		}

		if(generate(file.first, output, compiled_glsl, optimize, types)) {
			assembler::log->info("generated shaders for {0}", file.first.platform());
			++success;
		} else
			assembler::log->error("failed to generate shaders for " + file.first.platform());
	}

	assembler::log->info("generated {0} shaders\n", success);
}
