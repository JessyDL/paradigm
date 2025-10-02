#include "generators/meta.h"

#if !defined(PE_GLES)
	#include "GLES3/gl32.h"
#endif

#include "core/gles/conversion.hpp"

namespace assembler::generators {
meta::meta() {
	if(psl::utility::platform::file::exists("metamapping.txt")) {
		mapping_table table;
		psl::serialization::serializer s;
		s.deserialize<psl::serialization::decode_from_format>(table, "metamapping.txt");
		m_FileMaps = table.mappings();
		m_EnvMaps  = table.environments();
	}
}

meta::cli_pack meta::meta_pack() {
	return cli_pack {
	  std::bind(&assembler::generators::meta::on_meta_generate, this, std::placeholders::_1),
	  cli_value<psl::string> {"input", "target file or folder to generate meta for", {"input", "i"}, "", false},
	  cli_value<psl::string> {"output", "target output", {"output", "o"}, "*." + psl::meta::META_EXTENSION},
	  cli_value<psl::string> {"type", "type of meta data to generate", {"type", "t"}, ""},
	  cli_value<bool> {"recursive",
					   "when true, and the source path is a directory, it will recursively go through it",
					   {"recursive", "r"},
					   false},
	  cli_value<bool> {"force", "remove existing and regenerate them from scratch", {"force", "f"}, false, true},
	  cli_value<bool> {"update",
					   "update is a specialization of force, where everything will be regenerated except the UID",
					   {"update", "u"},
					   false}

	};
}
meta::cli_pack meta::library_pack() {
	return cli_pack {
	  std::bind(&assembler::generators::meta::on_library_generate, this, std::placeholders::_1),
	  cli_value<psl::string> {"directory", "location of the library", {"directory", "d"}, "", false},
	  cli_value<psl::string> {"name", "the name of the library", {"name", "n"}, "resources.metalib"},
	  cli_value<psl::string> {
		"resource", "data folder that is the root to generate the library from", {"resource", "r"}, "", false},
	  cli_value<bool> {"clean",
					   "cleanup dangling .meta files that no longer have a corresponding file, and "
					   "dangling library entries",
					   {"clean"},
					   false}};
}

void meta::generate_library(psl::string lib_dir, psl::string lib_name, psl::string res_dir, bool clean) {
	size_t relative_position = 0u;

	lib_dir = psl::utility::platform::directory::to_unix(lib_dir);
	if(lib_dir[lib_dir.size() - 1] != '/')
		lib_dir += '/';
	res_dir = psl::utility::platform::directory::to_unix(res_dir);
	if(res_dir[res_dir.size() - 1] != '/')
		res_dir += '/';
	auto const max_index = std::min(lib_dir.find_last_of(('/')), res_dir.find_last_of(('/')));
	for(auto i = max_index; i > 0; --i) {
		if(psl::string_view(lib_dir.data(), i) == psl::string_view(res_dir.data(), i)) {
			relative_position = (max_index == i) ? i : i - 1;
			break;
		}
	}

	if(!psl::utility::platform::directory::exists(res_dir)) {
		psl::utility::terminal::set_color(psl::utility::terminal::color::RED);
		assembler::log->error("resource directory does not exist, so nothing to generate. This was likely unintended?");
		psl::utility::terminal::set_color(psl::utility::terminal::color::WHITE);
		return;
	}

	if(!psl::utility::platform::file::exists(lib_dir + lib_name)) {
		psl::utility::platform::file::write(lib_dir + lib_name, "");
		assembler::log->info("file not found, created one.");
	}

	auto all_files = psl::utility::platform::directory::all_files(res_dir, true);

	psl::string meta_ext = "." + psl::meta::META_EXTENSION;
	auto meta_end =
	  std::partition(std::begin(all_files), std::end(all_files), [&meta_ext](psl::string_view const& file) {
		  return (file.size() >= meta_ext.size()) ? file.substr(file.size() - meta_ext.size()) == meta_ext : false;
	  });


	psl::meta::metalib lib {};
	psl::string content;
	psl::serialization::serializer s;
	for(auto file_it = std::begin(all_files); file_it != meta_end; ++file_it) {
		auto const& file {*file_it};
		if(auto file_content = psl::utility::platform::file::read(file); file_content) {
			psl::format::container cont {psl::to_string8_t(file_content.value())};
			psl::string UID;

			psl::meta::file* metaPtr = nullptr;
			try {
				if(!s.deserialize<psl::serialization::decode_from_format>(metaPtr, cont) || !metaPtr) {
					assembler::log->info("error: could not decode the meta file at: {}", file.c_str());
					continue;
				}
			} catch(...) {
				debug_break();
			}

			UID = metaPtr->ID().to_string();
			delete(metaPtr);
			psl::array<psl::string> files;
			auto metapath = psl::utility::platform::directory::to_unix(file);
			{
				auto filepath =
				  psl::utility::platform::file::to_platform(metapath.substr(0, metapath.find_last_of('.')));
				if(filepath.find('.') == filepath.npos) {
					std::copy_if(
					  meta_end, std::end(all_files), std::back_inserter(files), [&filepath](auto const& file) {
						  return file.size() > filepath.size() &&
								 filepath == psl::string_view {file.data(), filepath.size()};
					  });
				} else {
					files.emplace_back(filepath);
				}
			}

			bool generated = false;
			for(auto const& platform_filepath : files) {
				auto filepath = psl::utility::platform::file::to_generic(platform_filepath);
				if(!psl::utility::platform::file::exists(filepath)) {
					assembler::log->error("meta {0} pointing to unexisting file {1}", metapath, filepath);
					continue;
				}
				generated = true;
				auto dot  = filepath.find_last_of('.');
				if(dot != psl::string::npos)
					dot += 1;
				auto extension = filepath.substr(dot, filepath.size() - dot);
				auto final_filepath =
				  psl::utility::platform::directory::to_generic(std::filesystem::relative(filepath, lib_dir).string());
				auto final_metapath =
				  psl::utility::platform::directory::to_generic(std::filesystem::relative(metapath, lib_dir).string());

				auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(
							  std::filesystem::last_write_time(psl::utility::platform::directory::to_platform(filepath))
								.time_since_epoch())
							  .count();
				auto metatime =
				  std::chrono::duration_cast<std::chrono::nanoseconds>(
					std::filesystem::last_write_time(psl::utility::platform::directory::to_platform(metapath))
					  .time_since_epoch())
					.count();

				psl::string env = {};

				psl::meta::metalib::entry entry {};
				if(auto it = m_EnvMaps.find(extension); it != std::end(m_EnvMaps)) {
					env = "[ENV=";
					env += std::accumulate(std::begin(it->second),
										   std::end(it->second),
										   psl::string {},
										   [](psl::string env, psl::string const& ext) {
											   return (env.empty() ? ext : std::move(env) + ", " + ext);
										   });
					env.append("]");

					for(auto const& env : it->second) {
						entry.environments->push_back(env);
					}
				}
				entry.id		 = UID;
				entry.data->path = final_filepath;
				entry.data->time = time;
				entry.meta->path = final_metapath;
				entry.meta->time = metatime;

				lib.entries->push_back(entry);

				/*content += "[UID=" + UID + "][PATH=" + final_filepath + "][METAPATH=" + final_metapath +
						   "][TIME=" + time.substr(0, time.size() - 1) +
						   "][METATIME=" + metatime.substr(0, metatime.size() - 1) + "]" + env + "\n";*/
			}

			if(!generated && clean) {
				std::cout << "erasing dangling meta file at " << metapath << std::endl;
				if(!psl::utility::platform::file::erase(metapath)) {
					psl::utility::terminal::set_color(psl::utility::terminal::color::RED);
					assembler::log->error("could not delete '{}'", metapath);
					psl::utility::terminal::set_color(psl::utility::terminal::color::WHITE);
				}
			}
		}
	}

	s.serialize<psl::serialization::encode_to_format>(lib, lib_dir + lib_name);

	// psl::utility::platform::file::write(lib_dir + lib_name, psl::string_view(content.data(), content.size() -
	// 1));
	assembler::log->info("wrote out a new meta library at: '{}'", psl::to_string8_t(lib_dir + lib_name));
}
void meta::generate_meta(psl::string in_path,
						 psl::string out_path,
						 psl::string meta_type,
						 bool recursive_search,
						 bool force_regenerate,
						 bool update) {
	auto input_path	 = assembler::pathstring(in_path);
	auto output_path = assembler::pathstring(out_path);

	if(update)
		force_regenerate = update;

	if(input_path->size() == 0) {
		psl::utility::terminal::set_color(psl::utility::terminal::color::RED);
		assembler::log->info("error: the input source path did not contain any characters.");
		psl::utility::terminal::set_color(psl::utility::terminal::color::WHITE);
		return;
	}

	psl::string const meta_extension = "." + psl::meta::META_EXTENSION;
	auto files						 = assembler::get_files(input_path, output_path);


	if(!force_regenerate) {
		// remove all those with existing meta files
		files.erase(std::remove_if(std::begin(files),
								   std::end(files),
								   [meta_extension](auto const& file_pair) {
									   return psl::utility::platform::file::exists(file_pair.second);
								   }),
					std::end(files));
	}

	for(auto const& [input, output] : files) {
		if(psl::string_view dir {output->data(), output->rfind('/')}; !psl::utility::platform::directory::exists(dir))
			psl::utility::platform::directory::create(dir);

		auto extension = input->substr(input->rfind('.') + 1);

		auto meta_t = meta_type;
		if(meta_t.empty()) {
			meta_t	= "META";
			auto it = m_FileMaps.find(extension);
			if(it != std::end(m_FileMaps))
				meta_t = it->second;
		}

		auto id = psl::utility::crc64(psl::to_string8_t(meta_t));
		if(auto it = psl::serialization::accessor::polymorphic_data().find(id);
		   it != psl::serialization::accessor::polymorphic_data().end()) {
			psl::meta::file* target = (psl::meta::file*)((*it->second->factory)());

			psl::UID uid = psl::UID::generate();
			psl::serialization::serializer s;

			if(psl::utility::platform::file::exists(output.platform()) && update) {
				psl::meta::file* original = nullptr;
				s.deserialize<psl::serialization::decode_from_format>(original, output.platform());
				uid = original->ID();
			}


			switch(id) {
			case psl::utility::crc64("TEXTURE_META"): {
				auto data = psl::utility::platform::file::read(
							  input, std::max(utility::ktx::header_size(), utility::dds::header_size()))
							  .value();
				auto view = psl::array_view<std::byte>((std::byte*)data.data(), data.size());
				if(utility::ktx::is_ktx(view)) {
					auto header =
					  utility::ktx::decode(psl::array_view<std::byte>((std::byte*)data.data(), data.size()));
					core::meta::texture_t* texture_meta = reinterpret_cast<core::meta::texture_t*>(target);
					texture_meta->width(header.pixelWidth);
					texture_meta->height(header.pixelHeight);
					texture_meta->depth(header.pixelDepth);
					texture_meta->mip_levels(header.numberOfMipmapLevels);
					texture_meta->format(
					  core::gfx::conversion::to_format(header.glInternalFormat, header.glFormat, header.glType));
					psl_assert(texture_meta->format() != core::gfx::format_t::undefined);
				} else if(utility::dds::is_dds(view)) {
					auto header =
					  utility::dds::decode(psl::array_view<std::byte>((std::byte*)data.data(), data.size()));
					core::meta::texture_t* texture_meta = reinterpret_cast<core::meta::texture_t*>(target);
					texture_meta->width(header.dwWidth);
					texture_meta->height(header.dwHeight);
					texture_meta->depth(header.dwDepth);
					texture_meta->mip_levels(header.dwMipMapCount);
					texture_meta->format(utility::dds::to_format(header));
					// assert_debug_break(texture_meta->format() != core::gfx::format_t::undefined);
				}
			} break;
			}

			psl::format::container cont;
			s.serialize<psl::serialization::encode_to_format>(target, cont);

			auto metaNode = cont.find("META");
			auto node	  = cont.find(metaNode.get(), "UID");
			cont.remove(node.get());
			cont.add_value(metaNode.get(), "UID", psl::utility::to_string(uid));


			psl::utility::platform::file::write(output, psl::from_string8_t(cont.to_string()));
			assembler::log->info("wrote a {0} file to {1} from {2}", meta_t, output.platform(), input.platform());
		} else {
			psl::utility::terminal::set_color(psl::utility::terminal::color::RED);
			assembler::log->error("error: could not deduce the polymorphic type from the given key '{}'", meta_t);
			assembler::log->error(
			  "  either the given key was incorrect, or the type was not registered to the assembler.");
			psl::utility::terminal::set_color(psl::utility::terminal::color::WHITE);
			continue;
		}
	}
}

void meta::on_meta_generate(cli_pack& pack) {
	/// -g -m -s "c:\\Projects\Paradigm\data\should_see_this\New Text Document.txt" -t "TEXTURE_META"
	// -g -m -i "C:\Projects\github\example_data\source/textures/*"  -o
	// "C:\Projects\github\example_data\data/textures/*.meta" -r -t "TEXTURE_META" -u -g -m -i
	// "C:\Projects\github\example_data\source/*" -o "C:\Projects\github\example_data\data/*.meta" -u
	auto input_path		  = pack["input"]->as<psl::string>().get();
	auto output_path	  = pack["output"]->as<psl::string>().get();
	auto meta_type		  = pack["type"]->as<psl::string>().get();
	auto recursive_search = pack["recursive"]->as<bool>().get();
	auto force_regenerate = pack["force"]->as<bool>().get();
	auto update			  = pack["update"]->as<bool>().get();

	generate_meta(input_path, output_path, meta_type, recursive_search, force_regenerate, update);
}
}	 // namespace assembler::generators
