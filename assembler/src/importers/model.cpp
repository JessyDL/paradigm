#include "importers/model.hpp"

#ifdef DBG_NEW
	#undef new
#endif

#include "assimp/DefaultLogger.hpp"
#include "assimp/Importer.hpp"
#include "assimp/Logger.hpp"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#ifdef DBG_NEW
	#define new DBG_NEW
#endif

#include "core/data/geometry.hpp"
#include "psl/library.hpp"
#include "psl/math/math.hpp"
#include "psl/meta.hpp"
#include "psl/serialization/serializer.hpp"
#include "stdafx.hpp"

constexpr psl::string_view MODEL_FORMAT		= "pgf";
constexpr psl::string_view SKELETON_FORMAT	= "psf";
constexpr psl::string_view ANIMATION_FORMAT = "paf";

unsigned int process_flags(assembler::importer::model_t::options_t options) {
	unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
						 aiProcess_ValidateDataStructure | aiProcess_OptimizeGraph;

	if(options.flip_uvs) {
		flags |= aiProcess_FlipUVs;
	}
	if(options.flip_winding) {
		flags |= aiProcess_FlipWindingOrder;
	}
	if(options.optimize) {
		flags |= aiProcess_OptimizeMeshes;
	}
	if(options.tangents) {
		flags |= aiProcess_CalcTangentSpace;
	}
	if(options.normals && !options.snormals) {
		flags |= aiProcess_GenNormals;
	}
	if(options.snormals) {
		flags |= aiProcess_GenSmoothNormals;
	}
	if(options.uvs) {
		flags |= aiProcess_GenUVCoords;
	}
	if(options.left_handed) {
		flags |= aiProcess_MakeLeftHanded;
	}
	if(options.flatten) {
		flags |= aiProcess_JoinIdenticalVertices;
	}
	return flags;
}

template <typename Fn>
auto import_model(aiMesh* pAIMesh,
				  psl::string const& submesh_name,
				  std::array<uint8_t, 3> axis_setup,
				  bool binary,
				  Fn&& fn) -> bool {
	using namespace core::data;
	using namespace psl::math;
	geometry_t result;
	typename std::decay<decltype(result.indices())>::type indices;


	unsigned int nVertices {pAIMesh->mNumVertices};

	if(pAIMesh->HasFaces()) {
		aiFace* pAIFaces;
		pAIFaces			  = pAIMesh->mFaces;
		unsigned int nIndices = pAIMesh->mNumFaces * 3;
		indices.resize(nIndices);

		for(unsigned int iface = 0; iface < pAIMesh->mNumFaces; iface++) {
			if(pAIFaces[iface].mNumIndices != 3) {
				assembler::log->error(
				  "the model has an incorrect number vertices per face. only 3 vertices per face allowed.");
				return false;
			}

			for(uint32_t j = 0; j < 3; j++) {
				indices[iface * 3 + j] = pAIFaces[iface].mIndices[j];
			}
		}

		result.indices(indices);
	}
	core::vertex_stream_t vec4_stream {core::vertex_stream_t::type::vec4};
	core::vertex_stream_t vec3_stream {core::vertex_stream_t::type::vec3};
	core::vertex_stream_t vec2_stream {core::vertex_stream_t::type::vec2};
	std::vector<psl::vec4>& datav4 = vec4_stream.get<core::vertex_stream_t::type::vec4>();
	std::vector<psl::vec3>& datav3 = vec3_stream.get<core::vertex_stream_t::type::vec3>();
	std::vector<psl::vec2>& datav2 = vec2_stream.get<core::vertex_stream_t::type::vec2>();
	datav4.resize(nVertices);
	datav3.resize(nVertices);
	datav2.resize(nVertices);

	if(pAIMesh->HasPositions()) {
		for(unsigned int ivert = 0; ivert < nVertices; ivert++) {
			datav3[ivert] = psl::vec3(pAIMesh->mVertices[ivert][axis_setup[0]],
									  pAIMesh->mVertices[ivert][axis_setup[1]],
									  pAIMesh->mVertices[ivert][axis_setup[2]]);
		}

		result.vertices(geometry_t::constants::POSITION, vec3_stream);
	} else {
		assembler::log->error("the model has no position data.");
		return false;
	}

	if(pAIMesh->HasNormals()) {
		for(unsigned int i = 0; i < nVertices; i++) {
			datav3[i] = normalize(psl::vec3(pAIMesh->mNormals[i][axis_setup[0]],
											pAIMesh->mNormals[i][axis_setup[1]],
											pAIMesh->mNormals[i][axis_setup[2]]));
		}

		result.vertices(geometry_t::constants::NORMAL, vec3_stream);
	}

	if(pAIMesh->HasTangentsAndBitangents()) {
		{
			for(unsigned int i = 0; i < nVertices; i++) {
				datav3[i] = normalize(psl::vec3(pAIMesh->mTangents[i][axis_setup[0]],
												pAIMesh->mTangents[i][axis_setup[1]],
												pAIMesh->mTangents[i][axis_setup[2]]));
			}

			result.vertices(geometry_t::constants::TANGENT, vec3_stream);
		}
		{
			for(unsigned int i = 0; i < nVertices; i++) {
				datav3[i] = normalize(psl::vec3(pAIMesh->mBitangents[i][axis_setup[0]],
												pAIMesh->mBitangents[i][axis_setup[1]],
												pAIMesh->mBitangents[i][axis_setup[2]]));
			}

			result.vertices(geometry_t::constants::BITANGENT, vec3_stream);
		}
	}

	for(uint32_t uvChannel = 0; uvChannel < pAIMesh->GetNumUVChannels(); ++uvChannel) {
		if(pAIMesh->HasTextureCoords(uvChannel)) {
			for(unsigned int i = 0; i < nVertices; i++) {
				datav2[i] = psl::vec2(pAIMesh->mTextureCoords[uvChannel][i].x, pAIMesh->mTextureCoords[uvChannel][i].y);
			}

			if(uvChannel > 1)
				result.vertices(psl::string(geometry_t::constants::TEX) +
								  psl::from_string8_t(psl::utility::to_string((uvChannel))),
								vec2_stream);
			else
				result.vertices(geometry_t::constants::TEX, vec2_stream);
		}
	}

	for(unsigned int c = 0; c < pAIMesh->GetNumColorChannels(); ++c) {
		if(pAIMesh->HasVertexColors(c)) {
			for(unsigned int i = 0; i < nVertices; i++) {
				datav4[i] = psl::vec4(pAIMesh->mColors[c][i].r,
									  pAIMesh->mColors[c][i].g,
									  pAIMesh->mColors[c][i].b,
									  pAIMesh->mColors[c][i].a);
			}

			if(c > 1)
				result.vertices(psl::string(geometry_t::constants::COLOR) +
								  psl::from_string8_t(psl::utility::to_string((c))),
								vec4_stream);
			else
				result.vertices(geometry_t::constants::COLOR, vec4_stream);
		}
	}

	fn(result, submesh_name);
	return true;
}

namespace assembler::importer {

auto model_t::import(std::filesystem::path const& file) -> importer_result_t {
	importer_result_t result {};

	psl::static_array<std::uint8_t, 3> axis_setup {};

	switch(m_Options.axis) {
	case options_t::axis_t::xyz:
		axis_setup = {0, 1, 2};
		break;
	case options_t::axis_t::xzy:
		axis_setup = {0, 2, 1};
		break;
	case options_t::axis_t::yxz:
		axis_setup = {1, 0, 2};
		break;
	case options_t::axis_t::yzx:
		axis_setup = {1, 2, 0};
		break;
	case options_t::axis_t::zxy:
		axis_setup = {2, 0, 1};
		break;
	case options_t::axis_t::zyx:
		axis_setup = {2, 1, 0};
		break;
	}

	Assimp::Importer importer;
	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);

	auto flags = process_flags(m_Options);

	if(!std::filesystem::exists(file)) {
		assembler::log->error("File {} does not exist", file.string());
		return {false};
	}

	aiScene const* pScene = importer.ReadFile(file.string(), flags);
	if(!pScene) {
		assembler::log->error("Error parsing file {}: {}", file.string(), importer.GetErrorString());
		return {false};
	}
	std::unordered_map<size_t, psl::string> meshNames;
	auto& root	 = *pScene->mRootNode;
	auto get_uid = [](std::filesystem::path const& file) -> std::optional<psl::UID> {
		if(!std::filesystem::exists(file))
			return std::nullopt;

		psl::meta::file* original = nullptr;
		psl::serialization::serializer temp_s;
		temp_s.deserialize<psl::serialization::decode_from_format>(original, file.string());
		return original->ID();
	};

	auto serialize_asset = [this, &file, &result, &get_uid](core::data::geometry_t& geometry,
															psl::string const& submesh_name = {}) -> void {
		auto output_file = rebase_to_build_dir(file);
		output_file =
		  output_file.replace_filename(output_file.stem().string() + submesh_name + output_file.extension().string());
		output_file = output_file.replace_extension(MODEL_FORMAT);

		psl::format::container cont;
		psl::format::settings settings;
		settings.binary_value	= m_Options.binary_output;
		settings.compact_string = m_Options.binary_output;
		cont.set_settings(settings);

		psl::serialization::serializer s;
		s.serialize<psl::serialization::encode_to_format>(geometry, cont);

		auto container_string = cont.to_string();
		psl::array<std::byte> res_data {reinterpret_cast<std::byte*>(container_string.data()),
										reinterpret_cast<std::byte*>(container_string.data()) +
										  container_string.size()};


		result.add(std::make_unique<write_file_t>(output_file, res_data));

		auto output_meta_file = output_file;
		output_meta_file.replace_extension(MODEL_FORMAT + ".meta");
		auto input_meta_file = file;
		input_meta_file.replace_extension(input_meta_file.extension().string() + ".meta");
		psl::UID uid = get_uid(output_meta_file).value_or(get_uid(input_meta_file).value_or(psl::UID::generate()));

		psl::meta::file metaFile {uid};
		cont = psl::format::container {};
		s.serialize<psl::serialization::encode_to_format>(metaFile, cont);
		container_string = cont.to_string();
		res_data		 = {reinterpret_cast<std::byte*>(container_string.data()),
							reinterpret_cast<std::byte*>(container_string.data()) + container_string.size()};

		result.add(std::make_unique<write_file_t>(output_meta_file, res_data));
	};

	for(unsigned int m = 0; m < pScene->mNumMeshes; ++m) {
		auto output_appendage = (pScene->mNumMeshes > 1) ? "_" + std::to_string(m) : "";
		if(!output_appendage.empty() && m < 10)
			output_appendage.insert(1, "0");

		output_appendage = (pScene->mNumMeshes > 1) ? "_" + meshNames[m] : "";
		if(!import_model(pScene->mMeshes[m], output_appendage, axis_setup, m_Options.binary_output, serialize_asset)) {
			goto error;
		}
	}

	importer.FreeScene();
	Assimp::DefaultLogger::kill();
	return result;
error:
	importer.FreeScene();
	Assimp::DefaultLogger::kill();
	return {false};
}
}	 // namespace assembler::importer
