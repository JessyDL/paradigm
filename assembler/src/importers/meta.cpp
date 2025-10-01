#include "importers/meta.hpp"
#include "stdafx.h"
#include <psl/crc32.hpp>
#include <psl/library.hpp>
#include <psl/meta.hpp>
#include <psl/serialization/serializer.hpp>

#include "details/texture_utils.hpp"
#include <core/meta/texture.hpp>
#include <psl/terminal_utils.hpp>

namespace assembler::importer {
void generate_texture_meta(std::filesystem::path path, core::meta::texture_t* texture_meta) {
	auto data = psl::utility::platform::file::read(path.string(),
												   std::max(utility::ktx::header_size(), utility::dds::header_size()))
				  .value();

	auto view = psl::array_view<std::byte>((std::byte*)data.data(), data.size());

	if(utility::ktx::is_ktx(view)) {
		auto header = utility::ktx::decode(psl::array_view<std::byte>((std::byte*)data.data(), data.size()));
		texture_meta->width(header.pixelWidth);
		texture_meta->height(header.pixelHeight);
		texture_meta->depth(header.pixelDepth);
		texture_meta->mip_levels(header.numberOfMipmapLevels);
		texture_meta->format(core::gfx::conversion::to_format(header.glInternalFormat, header.glFormat, header.glType));
		psl_assert(texture_meta->format() != core::gfx::format_t::undefined);
	} else if(utility::dds::is_dds(view)) {
		auto header = utility::dds::decode(psl::array_view<std::byte>((std::byte*)data.data(), data.size()));
		texture_meta->width(header.dwWidth);
		texture_meta->height(header.dwHeight);
		texture_meta->depth(header.dwDepth);
		texture_meta->mip_levels(header.dwMipMapCount);
		texture_meta->format(utility::dds::to_format(header));
		// assert_debug_break(texture_meta->format() != core::gfx::format_t::undefined);
	}
}

auto meta_t::import(std::filesystem::path const& file) -> importer_result_t {
	auto extension = file.extension().string();
	auto meta_path = std::filesystem::path {file}.replace_extension(extension + "." + psl::meta::META_EXTENSION);

	bool shouldGenMeta = true;
	// skip the meta files themselves.
	if(!extension.empty() && extension.substr(1) == psl::meta::META_EXTENSION) {
		assembler::log->error("The input file '{}' is a meta file, we cannot generate metadata for metafiles.",
							  file.string());
		return {false};
	}
	// or if they aren't a regular file
	else if(!std::filesystem::exists(file)) {
		assembler::log->error("The input file '{}' does not exist.", file.string());
		return {false};
	}
	// or if the file simply doesn't exist.
	else if(!std::filesystem::is_regular_file(file)) {
		assembler::log->error("The input path '{}' is not a file.", file.string());
		return {false};
	}
	// and lastly, if we don't forcibly regenerate, skip the pre-existing files
	else if(!force_regenerate) {
		// if the option to force regenerate is not set, we should check if the file has a meta
		// file, and if it does, we should skip it.

		if(std::filesystem::exists(meta_path)) {
			// this isn't an error condition, it simply means the settings considered this a no-op.
			shouldGenMeta = false;
		}
	}
	importer_result_t import_result {};

	if(shouldGenMeta) {
		auto meta_t = project().meta_mapping().meta(extension);

		auto id = psl::utility::crc64(psl::to_string8_t(meta_t));

		if(auto it = psl::serialization::accessor::polymorphic_data().find(id);
		   it != psl::serialization::accessor::polymorphic_data().end()) {
			psl::meta::file* target = (psl::meta::file*)((*it->second->factory)());

			psl::UID uid = psl::UID::generate();
			psl::serialization::serializer s;

			// get the UID from the pre-existing meta file, if it exists.
			if(exists(meta_path)) {
				psl::meta::file* original = nullptr;
				s.deserialize<psl::serialization::decode_from_format>(original, meta_path.string());
				uid = original->ID();
			}

			switch(id) {
			case psl::utility::crc64("TEXTURE_META"): {
				generate_texture_meta(file, reinterpret_cast<core::meta::texture_t*>(target));
			} break;
			}

			psl::format::container cont;
			s.serialize<psl::serialization::encode_to_format>(target, cont);

			auto metaNode = cont.find("META");
			auto node	  = cont.find(metaNode.get(), "UID");
			cont.remove(node.get());
			cont.add_value(metaNode.get(), "UID", psl::utility::to_string(uid));

			psl::array<std::byte> data;
			auto cont_string = cont.to_string();
			data.resize(cont_string.size());
			std::memcpy(data.data(), cont_string.data(), cont_string.size());

			import_result.add(std::make_unique<importer::write_file_t>(meta_path, data));
			import_result.add(std::make_unique<importer::write_file_t>(rebase_to_build_dir(meta_path), data));
		} else {
			psl::utility::terminal::set_color(psl::utility::terminal::color::RED);
			assembler::log->error("error: could not deduce the polymorphic type from the given key '{}'", meta_t);
			assembler::log->error(
			  "  either the given key was incorrect, or the type was not registered to the assembler.");
			psl::utility::terminal::set_color(psl::utility::terminal::color::WHITE);
		}
	} else {
		import_result.add(std::make_unique<importer::copy_file_t>(meta_path, rebase_to_build_dir(meta_path)));
	}
	import_result.add(std::make_unique<importer::copy_file_t>(file, rebase_to_build_dir(file)));

	return import_result;
}
}	 // namespace assembler::importer