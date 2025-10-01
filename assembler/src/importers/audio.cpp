#include "importers/audio.hpp"
#include "stdafx.h"

#include "core/meta/audio.hpp"

namespace assembler::importer {
auto audio_t::import(std::filesystem::path const& file) -> importer_result_t {
	importer_result_t result {};

	if(!std::filesystem::exists(file)) {
		assembler::log->error("Audio importer: File does not exist: {}", file.string());
		return {false};
	}

	auto output_file = rebase_to_build_dir(file);
	if(!std::filesystem::exists(output_file) ||
	   std::filesystem::last_write_time(output_file) != std::filesystem::last_write_time(file) ||
	   std::filesystem::file_size(output_file) != std::filesystem::file_size(file)) {
		result.add(std::make_unique<copy_file_t>(file, output_file));
	}

	// find if a meta file exists next to the source file
	auto input_meta_file =
	  std::filesystem::path {file}.replace_extension(file.extension().string() + "." + psl::meta::META_EXTENSION);
	auto output_meta_file =
	  output_file.replace_extension(output_file.extension().string() + "." + psl::meta::META_EXTENSION);

	auto get_meta = [](std::filesystem::path const& file) -> std::unique_ptr<core::meta::audio_t> {
		if(!std::filesystem::exists(file))
			return {};

		core::meta::audio_t* original = nullptr;
		psl::serialization::serializer temp_s;
		temp_s.deserialize<psl::serialization::decode_from_format>(original, file.string());
		return std::unique_ptr<core::meta::audio_t> {original};
	};

	std::unique_ptr<core::meta::audio_t> meta = nullptr;

	// first we try to load the existing output meta file, if it exists but the input does not
	// we will copy the output to the input location
	// then we try to load the input meta file if the previous operation failed
	// if that exists we will copy it to the output location
	// if both failed we will create a new meta file and write it to both locations.

	if(std::filesystem::exists(output_meta_file)) {
		meta = get_meta(output_meta_file);
		if(meta && !std::filesystem::exists(input_meta_file)) {
			result.add(std::make_unique<copy_file_t>(output_meta_file, input_meta_file));
		}
	}
	if(!meta && std::filesystem::exists(input_meta_file)) {
		meta = get_meta(input_meta_file);
		if(meta) {
			result.add(std::make_unique<copy_file_t>(input_meta_file, output_meta_file));
		}
	}
	if(!meta) {
		meta = std::make_unique<core::meta::audio_t>(psl::UID::generate());
		meta->format([ext = std::filesystem::path {file}.extension()]() -> core::audio::format_t {
			if(ext == ".wav") {
				return core::audio::format_t::wav;
			} else if(ext == ".mp3") {
				return core::audio::format_t::mp3;
			} else if(ext == ".ogg") {
				return core::audio::format_t::vorbis;
			} else if(ext == ".flac") {
				return core::audio::format_t::flac;
			} else if(ext == ".pcm") {
				return core::audio::format_t::pcm;
			} else {
				assembler::log->error("Audio importer: Unsupported audio format: {}", ext.string());
				return core::audio::format_t::unknown;
			}
		}());
		psl::format::container cont;
		psl::serialization::serializer s;
		s.serialize<psl::serialization::encode_to_format>(meta.get(), cont);

		auto container_string = cont.to_string();
		psl::array<std::byte> res_data {reinterpret_cast<std::byte*>(container_string.data()),
										reinterpret_cast<std::byte*>(container_string.data()) +
										  container_string.size()};

		result.add(std::make_unique<write_file_t>(input_meta_file, res_data));
		result.add(std::make_unique<write_file_t>(output_meta_file, res_data));
	}

	return result;
}
}	 // namespace assembler::importer