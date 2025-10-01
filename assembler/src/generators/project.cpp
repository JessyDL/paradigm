#include "generators/project.hpp"
#include "utils.h"
#include <filesystem>

#include "psl/format.hpp"
#include "psl/platform_utils.hpp"
#include "psl/serialization/serializer.hpp"

#include "data/project.hpp"

#include "importers/audio.hpp"
#include "importers/importer.hpp"
#include "importers/meta.hpp"
#include "importers/model.hpp"
#include "importers/shader.hpp"

#include <future>

#include <fstream>

std::uint64_t get_file_time(std::filesystem::path const& path) {
	return std::filesystem::last_write_time(path).time_since_epoch().count();
}

namespace assembler::generators {
void project::generate_resource_library(std::filesystem::path path, assembler::data::project_t const& project) {
	struct metalib_entry_t {
		std::filesystem::path path;
		std::uint64_t time;
	};

	struct metafile_pair_t {
		std::filesystem::path meta;
		std::filesystem::path data;
		std::uint64_t meta_time;
		std::uint64_t data_time;
		psl::UID uid;
	};

	auto const metalib_dir = std::filesystem::absolute(path.parent_path());
	if(!std::filesystem::exists(metalib_dir)) {
		std::filesystem::create_directories(metalib_dir);
	}
	psl::serialization::serializer s;

	// using filesystem get all files in the buildDir that have the extension .meta
	psl::array<metafile_pair_t> files {};

	auto extension = "." + psl::meta::META_EXTENSION;
	auto buildDir  = std::filesystem::absolute(std::filesystem::path {project.project_directory()} /
											   std::filesystem::path {project.build_directory()});
	for(auto& p : std::filesystem::recursive_directory_iterator(buildDir)) {
		// todo: what to do with files who have no pair?
		if(p.path().extension() == ".meta") {
			auto relative_path = std::filesystem::relative(p.path(), metalib_dir);
			auto data_path	   = std::filesystem::path {relative_path}.replace_extension("");

			if(!std::filesystem::exists(metalib_dir / data_path)) {
				assembler::log->warn(
				  "The meta file at '{}' is orphaned. It's accompanying data file could not be found at {}. Skipping..",
				  relative_path.string(),
				  data_path.string());
				continue;
			}

			psl::meta::file meta {};
			s.deserialize<psl::serialization::decode_from_format>(meta, p.path().string());
			psl::UID uid = meta.ID();

			files.push_back({relative_path, data_path, get_file_time(p), get_file_time(metalib_dir / data_path), uid});
		}
	}

	psl::meta::metalib metalib {};
	// todo: figure out a way to update existing metalibs. The biggest issue here is that we have environment dependent
	//       files which have no idea of their environment themselves.
	if(false && std::filesystem::exists(path)) {
		s.deserialize<psl::serialization::decode_from_format>(metalib, path.string());
	}

	//// erase all entries that are not present in the files
	//{
	//	auto missing_entries_it = std::remove_if(
	//	  std::begin(metalib.entries.value), std::end(metalib.entries.value), [&files](auto const& entry) {
	//		  return std::find_if(std::begin(files), std::end(files), [&entry](auto const& file) {
	//					 return file.uid == entry.id;
	//				 }) == std::end(files);
	//	  });
	//	for(auto const& entry : metalib.entries.value) {
	//		assembler::log->warn("Found orphaned meta file '{}'. Removing it from the metalib", entry.meta->path.value);
	//	}
	//	metalib.entries->erase(missing_entries_it, std::end(metalib.entries.value));
	//}

	//// fix all entries who have moved
	// std::for_each(std::begin(metalib.entries.value), std::end(metalib.entries.value), [&files](auto& entry) {
	//	auto it = std::find_if(std::begin(files), std::end(files), [&entry](auto const& file) {
	//		return file.uid == entry.id && (file.meta != entry.meta->path || file.data != entry.data->path);
	//	});
	//	if(it != std::end(files)) {
	//		assembler::log->warn(
	//		  "Found orphaned file '{}' at '{}' and '{}'. Updating it in the metalib to '{}' and '{}'.",
	//		  entry.id->to_string(),
	//		  entry.meta->path.value,
	//		  entry.data->path.value,
	//		  it->meta.string(),
	//		  it->data.string());
	//		entry.meta->path = it->meta.string();
	//		entry.data->path = it->data.string();
	//		entry.meta->time = it->meta_time;
	//		entry.data->time = it->data_time;
	//	}
	// });
	std::unordered_map<psl::string, psl::string> extension_to_environment {};
	{
		auto const& default_environments = project.meta_mapping().environments();
		for(auto const& entry : default_environments) {
			for(auto const& env : entry.second) {
				extension_to_environment["." + entry.first] = env;
			}
		}
	}
	// add all entries that are not present in the metalib
	std::for_each(
	  std::begin(files), std::end(files), [&metalib, &s, &metalib_dir, &extension_to_environment](auto const& file) {
		  auto it = std::find_if(
			std::begin(metalib.entries.value), std::end(metalib.entries.value), [&file](auto const& entry) {
				return file.uid == entry.id && file.meta == entry.meta->path.value;
			});
		  if(it == std::end(metalib.entries.value)) {
			  psl::format::container data {};
			  auto meta = psl::meta::file();

			  psl::array<psl::string> environments {};

			  if(auto it = extension_to_environment.find(file.data.extension().string());
				 it != std::end(extension_to_environment)) {
				  environments.push_back(it->second);
			  }

			  s.deserialize<psl::serialization::decode_from_format>(meta, (metalib_dir / file.meta).string());
			  metalib.entries->push_back(psl::meta::metalib::entry {.id	  = meta.ID(),
																	.data = {file.data.string(), file.data_time},
																	.meta = {file.meta.string(), file.meta_time},
																	.environments = environments});
		  }
	  });


	psl::format::container cont {};
	s.serialize<psl::serialization::encode_to_format>(metalib, cont);
	if(psl::utility::platform::file::write(path.string(), cont.to_string()) == false) {
		assembler::log->error("Failed to write the metalib file '{}'", path.string());
		return;
	}
}

void project::on_generate(psl::cli::pack& pack) {
	auto projectFile	= pathstring {pack["input"]->as<psl::string>().get()}.platform();
	auto outputDir		= pathstring {pack["output"]->as<psl::string>().get()}.platform();
	auto only_models	= pack["models"]->as<bool>().get();
	auto only_shaders	= pack["shaders"]->as<bool>().get();
	auto only_audio		= pack["audio"]->as<bool>().get();
	auto import_audio	= !(only_shaders && only_models) || only_audio;
	auto import_models	= !(only_shaders && only_audio) || only_models;
	auto import_shaders = !(only_models && only_audio) || only_shaders;
	auto import_default = !(only_models || only_shaders || only_audio);
	auto force_import	= pack["force"]->as<bool>().get();

	using assembler::data::project_t;

	project_t project {};


	psl::string projectDir		= {};
	psl::string projectFilename = {};
	psl::string projectExt		= {};

	// Deconstruct, or default initialize the path parts for the project file.
	if(psl::utility::platform::directory::is_directory(projectFile)) {
		projectDir = projectFile.substr(
		  0,
		  psl::utility::string::rfind_first_of(projectFile,
											   psl::string(psl::utility::platform::directory::seperator) +
												 psl::utility::platform::directory::seperator_platform));
		projectFilename = project_t::DEFAULT_NAME;
		projectExt		= project_t::DEFAULT_EXTENSION;
	} else {
		auto lastSlash =
		  psl::utility::string::rfind_first_of(projectFile,
											   psl::string(psl::utility::platform::directory::seperator) +
												 psl::utility::platform::directory::seperator_platform);
		projectDir		   = projectFile.substr(0, lastSlash);
		auto has_extension = projectFile.find_last_of('.');
		if(has_extension != psl::string::npos) {
			projectExt		= projectFile.substr(has_extension + 1);
			projectFilename = projectFile.substr(lastSlash + 1, has_extension - lastSlash - 1);
		} else {
			projectFilename =
			  (lastSlash + 1 == projectFile.length()) ? project_t::DEFAULT_NAME : projectFile.substr(lastSlash + 1);
			projectExt = project_t::DEFAULT_EXTENSION;
		}
	}

	// Reconstruct the project file path using the just deconstructed parts.
	projectFile = projectDir + psl::utility::platform::directory::seperator + projectFilename + "." + projectExt;

	if(!psl::utility::platform::file::exists(projectFile)) {
		assembler::log->info("The project file '{}' does not exist. Generating one now..", projectFile);
		psl::serialization::serializer s;
		psl::format::container cont {};
		s.serialize<psl::serialization::encode_to_format>(project, cont);
		assembler::log->info("Generated project file '{}'", projectFile);
		if(psl::utility::platform::file::write(projectFile, cont.to_string()) == false) {
			assembler::log->error("Failed to write the project file '{}'", projectFile);
			return;
		}
	} else {
		psl::serialization::serializer s;
		s.deserialize<psl::serialization::decode_from_format>(project, projectFile);
	}

	project.project_directory(projectDir);
	if(!outputDir.empty()) {
		project.build_directory(outputDir);
	}

	assembler::log->info("Project file '{}' loaded", projectFile);

	std::filesystem::path projectPath = projectDir;

	std::filesystem::path sourceDir = (projectPath / project.source_directory()).lexically_normal();
	std::filesystem::path buildDir	= std::filesystem::path {project.build_directory()}.lexically_normal();
	if(buildDir.is_relative()) {
		// If the build directory is relative, we need to resolve it against the project path
		buildDir = (projectPath / buildDir).lexically_normal();
	}

	if(!std::filesystem::exists(sourceDir)) {
		assembler::log->error("The source directory '{}' does not exist", sourceDir.string());
		return;
	}

	if(!std::filesystem::exists(buildDir)) {
		assembler::log->info("The build directory '{}' does not exist, creating it now", buildDir.string());
		std::filesystem::create_directories(buildDir);
	}

	auto files = psl::utility::platform::directory::all_files(sourceDir.string(), true);

	// check the files that were found, compare to their versions in the output directory, if they exist and are newer,
	// skip them
	if(!force_import) {
		files.erase(std::remove_if(std::begin(files),
								   std::end(files),
								   [&sourceDir, &buildDir](auto const& file) {
									   auto ipath = std::filesystem::path(file);
									   auto rel	  = std::filesystem::relative(ipath, sourceDir);
									   auto opath = buildDir / ".assembler" / rel;
									   // add .TIMESTAMP as extension to the output path
									   opath += ".TIMESTAMP";
									   if(std::filesystem::exists(opath)) {
										   auto input_time	= get_file_time(ipath);
										   auto output_time = get_file_time(opath);
										   return output_time == input_time;
									   }
									   return false;
								   }),
					std::end(files));
	}

	auto run_importer = [&sourceDir, &buildDir, import_audio, import_models, import_shaders, import_default](
						  assembler::data::project_t project, psl::array_view<psl::string> files) {
		importer::importer_t importer {project};
		if(!project.is_latest_version()) {
			assembler::log->warn("The project is version {}, but expected is {}. Please consider updating it.",
								 project.version(),
								 project.CURRENT_VERSION);
		}
		importer.ignore_extension(".meta");

		auto get_meta_file_extensions = [&project](psl::string_view meta) {
			auto filetypes = project.meta_mapping().mapping(meta);
			if(filetypes.empty()) {
				assembler::log->warn("No file types found for meta type '{}'", meta);
				throw std::runtime_error(
				  "No file types found for meta type during importation, check logs for more info.");
			}
			std::transform(std::begin(filetypes), std::end(filetypes), std::begin(filetypes), [](auto const& str) {
				return "." + str;
			});
			return filetypes;
		};
		if(import_audio && project.version() > 1) {
			auto audio_importer = importer.register_importer<importer::audio_t>();
			importer.map_extension(get_meta_file_extensions("AUDIO_META"), audio_importer);
		}
		if(import_shaders) {
			auto shader_importer = importer.register_importer<importer::shader_t>();
			importer.map_extension(get_meta_file_extensions("SHADER_META"), shader_importer);
		}
		if(import_models) {
			auto model_importer = importer.register_importer<importer::model_t>();
			importer.map_extension(psl::array<psl::string> {".dae", ".gltf", ".fbx", ".md5mesh"}, model_importer);
		}
		if(import_default) {
			auto meta_importer = importer.register_importer<importer::meta_t>();
			importer.default_importer(meta_importer);
		}

		for(auto const& file : files) {
			auto ipath = std::filesystem::path(file);
			auto rel   = std::filesystem::relative(ipath, sourceDir);
			auto opath = buildDir / ".assembler" / rel;
			opath += ".TIMESTAMP";

			if(!importer.import(ipath)) {
				assembler::log->error("Failed to import file '{}', inspect log for more details", file);
			} else {
				// make TIMESTAMP file in the output directory, and give it the timestamp of the input file
				if(!std::filesystem::exists(opath.parent_path())) {
					std::filesystem::create_directories(opath.parent_path());
				}
				std::ofstream timestamp {opath.string(), std::ios::out | std::ios::trunc};
				timestamp.close();
				auto time = std::filesystem::last_write_time(ipath);
				std::filesystem::last_write_time(opath, time);
			}
		}
	};

	if(files.empty()) {
		assembler::log->info("No files need to be imported, everything is up to date");
	} else {
		run_importer(project, files);
		generate_resource_library(buildDir / "resources.metalib", project);
	}
	assembler::log->info("Project generation complete");
}
}	 // namespace assembler::generators
