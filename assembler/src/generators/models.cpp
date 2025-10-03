#include "generators/models.hpp"
#include "cli/value.hpp"
#include "psl/library.hpp"
#include "psl/math/math.hpp"
#include "psl/meta.hpp"
#include "psl/serialization/serializer.hpp"
#include "psl/terminal_utils.hpp"
#include "stdafx.hpp"
#include <iostream>
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

// #include "data/animation.h"
#include "core/data/geometry.hpp"
// #include "data/skeleton.h"
using namespace assembler::generators;
using namespace core::data;
using namespace psl::math;

template <typename T>
using cli_value = psl::cli::value<T>;
using namespace psl;

constexpr psl::string_view MODEL_FORMAT		= "pgf";
constexpr psl::string_view SKELETON_FORMAT	= "psf";
constexpr psl::string_view ANIMATION_FORMAT = "paf";

models::models() {
	m_Pack =
	  psl::cli::pack {std::bind(&assembler::generators::models::on_invoke, this, std::placeholders::_1),
					  cli_value<psl::string> {"input", "location of the input file", {"input", "i"}, "", false},
					  cli_value<psl::string> {"output", "location of the output file", {"output", "o"}, "", true},
					  cli_value<bool> {"tangents", "generate tangent information", {"tangents", "t"}, true},
					  cli_value<bool> {"normals", "generate normal information", {"normals", "n"}, true},
					  cli_value<bool> {"snormals", "generate smooth normal information", {"snormals", "s"}, true},
					  cli_value<bool> {"uvs", "generate uv information", {"uvs"}, true},
					  cli_value<bool> {"optimize", "optimize the mesh", {"optimize", "O"}, true},
					  cli_value<bool> {"LH", "left handed coordinate system", {"LH"}, true},
					  cli_value<bool> {"fuvs", "flip uv coorinates", {"fuvs"}, false},
					  cli_value<bool> {"fwinding", "flip triangle winding", {"fwinding"}, false},
					  cli_value<bool> {"flatten", "", {"flatten", "f"}, false},
					  cli_value<psl::string> {"axis", "what should be left, up, and forward?", {"axis"}, "xzy"},
					  cli_value<bool> {"sparse_skeleton", "compress the skeleton information", {"sparse"}, true},
					  cli_value<bool> {"binary", "outputs the file in binary form", {"bin", "b"}, false}};
}

bool proccess_flags(cli::pack& pack, unsigned int& flags) {
	flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
			aiProcess_ValidateDataStructure | aiProcess_OptimizeGraph;

	if(pack["tangents"]->as<bool>().get())
		flags |= aiProcess_CalcTangentSpace;
	if(pack["snormals"]->as<bool>().get())
		flags |= aiProcess_GenSmoothNormals;
	if(pack["normals"]->as<bool>().get() && !pack["snormals"]->as<bool>().get())
		flags |= aiProcess_GenNormals;
	if(pack["uvs"]->as<bool>().get())
		flags |= aiProcess_GenUVCoords;
	if(pack["optimize"]->as<bool>().get())
		flags |= aiProcess_OptimizeMeshes;
	if(pack["LH"]->as<bool>().get())
		flags |= aiProcess_MakeLeftHanded;
	if(pack["fuvs"]->as<bool>().get())
		flags |= aiProcess_FlipUVs;
	if(pack["fwinding"]->as<bool>().get())
		flags |= aiProcess_FlipWindingOrder;
	if(pack["flatten"]->as<bool>().get())
		flags |= aiProcess_JoinIdenticalVertices;


	return true;
}

template <typename T>
bool write_meta(T& data, psl::string output_file, psl::string_view const& extension, bool binary) {
	format::container cont {};
	format::settings settings {};
	if(binary) {
		settings.compact_string = true;
		settings.binary_value	= true;
	}
	cont.set_settings(settings);
	serialization::serializer s;

	s.serialize<serialization::encode_to_format>(data, cont);
	output_file += "." + extension;
	auto output_meta = output_file + "." + psl::from_string8_t(meta::META_EXTENSION);

	if(!utility::platform::file::write(output_file, cont.to_string())) {
		assembler::log->error("could not write the output file.");
		return false;
	}

	{
		UID uid = UID::generate();
		if(utility::platform::file::exists(output_meta)) {
			::meta::file* original = nullptr;
			serialization::serializer temp_s;
			temp_s.deserialize<serialization::decode_from_format>(original, output_meta);
			uid = original->ID();
		}

		meta::file metaFile {uid};
		cont = format::container {};

		s.serialize<serialization::encode_to_format>(metaFile, cont);
		if(!utility::platform::file::write(output_meta, cont.to_string())) {
			assembler::log->error("could not write the output file.");
			return false;
		}
	}
	return true;
}

bool import_model(aiMesh* pAIMesh, psl::string output_file, std::array<uint8_t, 3> axis_setup, bool binary) {
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
								  psl::from_string8_t(utility::to_string((uvChannel))),
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
								  psl::from_string8_t(utility::to_string((c))),
								vec4_stream);
			else
				result.vertices(geometry_t::constants::COLOR, vec4_stream);
		}
	}

	return write_meta(result, std::move(output_file), MODEL_FORMAT, binary);
}

template <typename T>
psl::mat4x4 convert(aiMatrix4x4t<T> const& source) {
	return mat4x4 {source.a1,
				   source.a2,
				   source.a3,
				   source.a4,
				   source.b1,
				   source.b2,
				   source.b3,
				   source.b4,
				   source.c1,
				   source.c2,
				   source.c3,
				   source.c4,
				   source.d1,
				   source.d2,
				   source.d3,
				   source.d4};
}

bool import_skeleton(aiMesh* pAIMesh, psl::string output_file, bool binary) {
	throw std::runtime_error("not implemented");
	return false;

	/*if(pAIMesh->mNumBones == 0) return true;
	psl::array<skeleton::bone> bones{};
	bones.reserve(pAIMesh->mNumBones);
	for(auto i = 0; i < pAIMesh->mNumBones; ++i)
	{
		const auto& aiBone = *pAIMesh->mBones[i];
		auto& bone		   = bones.emplace_back();

		bone.name(aiBone.mName.C_Str());
		bone.inverse_bindpose(convert(aiBone.mOffsetMatrix));
		psl::array<core::data::geometry::index_size_t> weight_ids{};
		psl::array<float> weights{};
		weight_ids.reserve(aiBone.mNumWeights);
		weights.reserve(aiBone.mNumWeights);
		for(auto w = 0; w < aiBone.mNumWeights; ++w)
		{
			weight_ids.emplace_back(aiBone.mWeights[w].mVertexId);
			weights.emplace_back(aiBone.mWeights[w].mWeight);
		}

		bone.weights(weights);
		bone.weight_ids(weight_ids);
	}
	core::data::skeleton skeleton{};
	skeleton.bones(bones);

	return write_meta(skeleton, std::move(output_file), SKELETON_FORMAT, binary);*/
}

bool import_animation(aiAnimation const& aiAnim, psl::string output_file, bool binary) {
	throw std::runtime_error("not implemented");
	return false;
	/*
	core::data::animation anim;
	psl::string name = aiAnim.mName.C_Str();
	utility::string::replace_all(name, " ", "_");
	utility::string::to_lower(name);
	anim.name(name);
	anim.duration(aiAnim.mDuration);
	anim.fps(aiAnim.mTicksPerSecond);
	psl::array<core::data::animation::bone> bones;
	for(auto i = 0; i < aiAnim.mNumChannels; ++i)
	{
		const auto& aiChannel = *aiAnim.mChannels[i];
		auto& bone			  = bones.emplace_back();
		bone.name(aiChannel.mNodeName.C_Str());
	}
	anim.bones(std::move(bones));
	return write_meta(anim, std::move(output_file) + ((name.empty())?"":"_" + name), ANIMATION_FORMAT, binary);
	*/
}

void models::on_invoke(cli::pack& pack) {
	// --generate --geometry -i "C:\Projects\data_old\Models\Cerberus.FBX"
	// --generate --geometry -i "C:\Projects\data_old\Models\Translate.FBX"
	// --generate --models -i "/c/Projects/github/example_data/source/models/mech_drone/scene.gltf"
	std::array<uint8_t, 3> axis_setup;
	{
		auto axis = pack["axis"]->as<psl::string>().get();
		if(axis.size() != 3) {
			utility::terminal::set_color(utility::terminal::color::RED);
			assembler::log->error(
			  "you provided an invalid axis, you should enter three letters, and the only accepted values are 'x', "
			  "'y', and 'z'. You provided {}",
			  axis);
			utility::terminal::set_color(utility::terminal::color::WHITE);
			return;
		}

		bool xSet = false;
		bool ySet = false;
		bool zSet = false;
		for(uint8_t i = 0; i < 3; ++i) {
			if(axis[i] == ('x') && !xSet) {
				xSet		  = true;
				axis_setup[i] = 0;
			} else if(axis[i] == ('y') && !ySet) {
				ySet		  = true;
				axis_setup[i] = 1;
			} else if(axis[i] == ('z') && !zSet) {
				zSet		  = true;
				axis_setup[i] = 2;
			} else {
				utility::terminal::set_color(utility::terminal::color::RED);
				assembler::log->error(
				  "You provided an invalid axis, the accepted values are 'x', 'y', and 'z', and no duplicates.\nYou "
				  "entered: {}",
				  axis);
				utility::terminal::set_color(utility::terminal::color::WHITE);
				return;
			}
		}
	}

	auto input_file		  = pack["input"]->as<psl::string>().get();
	auto output_file	  = pack["output"]->as<psl::string>().get();
	bool encode_to_binary = pack["binary"]->as<bool>().get();

	Assimp::Importer importer;
	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);


	unsigned int flags = 0u;
	if(!proccess_flags(pack, flags))
		return;

	if(!utility::platform::file::exists(input_file)) {
		utility::terminal::set_color(utility::terminal::color::RED);
		assembler::log->error("the file '{}' does not exist", input_file);
		utility::terminal::set_color(utility::terminal::color::WHITE);
		return;
	}

	if(input_file == output_file || output_file.empty()) {
		output_file = input_file.substr(0, input_file.find_last_of(('.')));
	}
	input_file	= utility::platform::directory::to_platform(input_file);
	output_file = utility::platform::directory::to_unix(output_file);

	aiScene const* pScene = importer.ReadFile(psl::to_string8_t(input_file), flags);
	psl::string errorMessage;
	if(!pScene) {
		utility::terminal::set_color(utility::terminal::color::RED);
		assembler::log->error("the scene of the file '{0}' could not be loaded by assimp", input_file);
		assembler::log->error(importer.GetErrorString());
		utility::terminal::set_color(utility::terminal::color::WHITE);
		return;
	}

	std::unordered_map<size_t, psl::string> meshNames;

	auto& root = *pScene->mRootNode;
	std::function<void(size_t, aiNode&, std::unordered_map<size_t, psl::string>&)> recurse_log;
	recurse_log =
	  [&recurse_log](size_t depth, aiNode& node, std::unordered_map<size_t, psl::string>& meshNames) -> void {
		assembler::log->info(fmt::runtime(psl::string(depth * 2, ' ') + node.mName.C_Str() + " meshes: {0}"),
							 node.mNumMeshes);
		for(auto i = 0u; i < node.mNumMeshes; ++i) {
			meshNames[node.mMeshes[i]] = node.mName.C_Str();
		}
		for(auto i = 0u; i < node.mNumChildren; ++i) {
			std::invoke(recurse_log, depth + 1, *node.mChildren[i], meshNames);
		}
	};

	recurse_log(0, root, meshNames);

	for(unsigned int m = 0; m < pScene->mNumMeshes; ++m) {
		auto output_appendage = (pScene->mNumMeshes > 1) ? "_" + std::to_string(m) : "";
		if(!output_appendage.empty() && m < 10)
			output_appendage.insert(1, "0");

		output_appendage = (pScene->mNumMeshes > 1) ? "_" + meshNames[m] : "";
		if(!import_model(pScene->mMeshes[m], output_file + output_appendage, axis_setup, encode_to_binary) ||
		   !import_skeleton(pScene->mMeshes[m], output_file + output_appendage, encode_to_binary))
			goto error;
	}


	for(auto i = 0u; i < pScene->mNumAnimations; ++i) {
		auto& animation = *pScene->mAnimations[i];
		if(!import_animation(animation, output_file, encode_to_binary))
			goto error;
	}

	importer.FreeScene();
	Assimp::DefaultLogger::kill();
	return;
error:
	importer.FreeScene();
	utility::terminal::set_color(utility::terminal::color::RED);
	assembler::log->error(errorMessage);
	utility::terminal::set_color(utility::terminal::color::WHITE);
	Assimp::DefaultLogger::kill();
	return;
}
