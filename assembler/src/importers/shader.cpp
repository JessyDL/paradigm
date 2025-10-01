#include "importers/shader.hpp"

#include "details/spirv.hpp"
#include <filesystem>

#include "stdafx.h"
#include <core/meta/shader.hpp>
#include <psl/platform_utils.hpp>

#if defined(AS_ENABLE_WGSL)
	#include "tint/tint.h"
#endif


tools::shader_stage_t shader_stage_from_extension(psl::string_view extension) {
	if(extension == ".vert")
		return tools::shader_stage_t::vert;
	if(extension == ".tesc")
		return tools::shader_stage_t::tesc;
	if(extension == ".tese")
		return tools::shader_stage_t::tese;
	if(extension == ".geom")
		return tools::shader_stage_t::geom;
	if(extension == ".frag")
		return tools::shader_stage_t::frag;
	if(extension == ".comp")
		return tools::shader_stage_t::comp;
	return tools::shader_stage_t::unknown;
}

namespace assembler::importer {
auto shader_t::import(std::filesystem::path const& file) -> importer_result_t {
	if(!std::filesystem::exists(file)) {
		assembler::log->error("error processing '{}': file does not exist.", file.string());
		return {false};
	}

	auto shader_stage = shader_stage_from_extension(file.extension().string());

	if(shader_stage == tools::shader_stage_t::unknown) {
		assembler::log->error("error processing '{}': could not deduce the shader type from the extension.",
							  file.string());
		return {false};
	}

	auto entry = m_ShaderCache.get(file);
	if(!entry) {
		assembler::log->error("error processing '{}': could not read the file, or it has no content.", file.string());
		return {false};
	}

	auto backends = project().graphics_backends();

	auto is_backend_enabled = [&](psl::string_view backend) {
		return std::find(std::begin(backends), std::end(backends), backend) != std::end(backends);
	};

	// todo: this can benefit from some more refining, but for now it's good enough
	std::optional<size_t> gles_version =
	  (std::find(std::begin(project().graphics_backends()), std::end(project().graphics_backends()), "gles") !=
	   std::end(project().graphics_backends()))
		? std::optional<size_t>(m_GlesVersion)
		: std::nullopt;

	auto compiled_result = tools::glsl_compile(entry.value(), shader_stage, m_Optimize, gles_version);
	for(auto const& message : compiled_result.messages) {
		if(message.error) {
			assembler::log->error(message.message);
		} else {
			assembler::log->info(message.message);
		}
	}
	if(!compiled_result) {
		assembler::log->error("error processing '{}': could not compile the shader.", file.string());
		return {false};
	}

	importer_result_t result {true};
	auto output_file = rebase_to_build_dir(file);

	// simple utility script to get the UID from the meta file, or returns a nullopt if it doesn't exist
	auto get_uid = [](std::filesystem::path const& file) -> std::optional<psl::UID> {
		auto meta_path = file;
		meta_path.replace_extension(file.extension().string() + ".meta");
		if(!std::filesystem::exists(meta_path))
			return std::nullopt;

		psl::meta::file* original = nullptr;
		psl::serialization::serializer temp_s;
		temp_s.deserialize<psl::serialization::decode_from_format>(original, meta_path.string());
		return original->ID();
	};

	auto shader_meta_string_for = [&get_uid](std::filesystem::path file,
											 std::filesystem::path output_file,
											 auto const& compiled_result,
											 psl::array<psl::string_view> extensions) {
		psl::array<std::filesystem::path> meta_files {};
		meta_files.emplace_back(file);
		for(auto const& extension : extensions) {
			meta_files.emplace_back(output_file.replace_extension(output_file.extension().string() + extension));
		}

		auto uid = [&meta_files, &get_uid]() -> psl::UID {
			for(auto const& meta_file : meta_files) {
				auto uid = get_uid(meta_file);
				if(uid.has_value()) {
					return uid.value();
				}
			}
			return psl::UID::generate();
		}();
		auto shaderMeta = core::meta::shader {uid};
		shaderMeta.inputs(compiled_result.shader.inputs);
		shaderMeta.outputs(compiled_result.shader.outputs);
		shaderMeta.descriptors(compiled_result.shader.descriptors);
		shaderMeta.stage(compiled_result.shader.stage);
		psl::serialization::serializer s;
		psl::format::container container;
		s.serialize<psl::serialization::encode_to_format>(&shaderMeta, container);
		return container.to_string();
	};
	auto meta_string = shader_meta_string_for(file, output_file, compiled_result, {".spv", ".gles", ".wgsl"});

	auto write_meta_output = [&meta_string, &result](std::filesystem::path const& file) {
		auto output_meta_file = file;
		output_meta_file.replace_extension(file.extension().string() + "." + psl::meta::META_EXTENSION);
		auto size_of_element = sizeof(decltype(meta_string[0]));
		psl::array<std::byte> byte_view {(std::byte*)meta_string.data(),
										 (std::byte*)meta_string.data() +
										   (meta_string.size() * size_of_element / sizeof(std::byte))};
		result.add(std::make_unique<write_file_t>(output_meta_file, byte_view));
	};

	write_meta_output(file);

	auto write_output = [&write_meta_output,
						 &result](std::filesystem::path const& file, auto const& srcData, psl::string_view extension) {
		auto output_file = file;
		output_file.replace_extension(output_file.extension().string() + extension);
		auto size_of_element = sizeof(decltype(srcData[0]));
		psl::array<std::byte> byte_view {(std::byte*)srcData.data(),
										 (std::byte*)srcData.data() +
										   (srcData.size() * size_of_element / sizeof(std::byte))};
		result.add(std::make_unique<write_file_t>(output_file, byte_view));
		write_meta_output(output_file);
	};

	if(is_backend_enabled("vulkan")) {
		if(compiled_result.spirv.empty()) {
			assembler::log->error("error processing '{}': could not compile the shader to spirv.", file.string());
			return {false};
		}
		write_output(output_file, compiled_result.spirv, ".spv");
	}

	if(is_backend_enabled("gles")) {
		if(compiled_result.gles.empty()) {
			assembler::log->error("error processing '{}': could not compile the shader to gles.", file.string());
			return {false};
		}
		write_output(output_file, compiled_result.gles, ".gles");
	}

#if defined(AS_ENABLE_WGSL)
	if(is_backend_enabled("webgpu")) {
		if(compiled_result.spirv.empty()) {
			assembler::log->error("error processing '{}': could not compile the shader to spirv (webgpu).",
								  file.string());
			return {false};
		}

		auto spirv = std::vector<uint32_t>(compiled_result.spirv.size() / sizeof(uint32_t));
		std::memcpy(spirv.data(), compiled_result.spirv.data(), compiled_result.spirv.size());
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
		write_output(output_file, wgsl.wgsl, ".wgsl");
	}
#endif
	return result;
}
}	 // namespace assembler::importer