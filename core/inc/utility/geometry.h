#pragma once
#include "stdafx.h"
#include "data/geometry.h"

/// \brief various geometry and geometry related operation utilities.
namespace utility::geometry
{
	/// \brief generates tangent information for the geometry data
	/// \returns the generated tangent information.
	/// \param[in] positions vertex position info
	/// \param[in] uvs vertex UVs
	/// \param[in] indices triangle indices.
	/// \note it is expected this is valid geometry data, only minimal to no data validation is done.
	static std::vector<psl::vec3> generate_tangents(const std::vector<psl::vec3>& positions, const std::vector<psl::vec2>& uvs, const std::vector<uint32_t>& indices)
	{
		std::vector<psl::vec3> tangents;
		tangents.resize(positions.size());

		for(int i = 0; i < indices.size(); i += 3)
		{
			// Shortcuts for vertices
			const auto & v0 = positions[indices[i + 0]];
			const auto & v1 = positions[indices[i + 1]];
			const auto & v2 = positions[indices[i + 2]];

			const auto & u0 = uvs[indices[i + 0]];
			const auto & u1 = uvs[indices[i + 1]];
			const auto & u2 = uvs[indices[i + 2]];

			psl::vec3 edge1 = v1 - v0;
			psl::vec3 edge2 = v2 - v0;
			psl::vec2 deltaUV1 = u1 - u0;
			psl::vec2 deltaUV2 = u2 - u0;

			float r = 1.0f / (deltaUV1[0] * deltaUV2[1] - deltaUV1[1] * deltaUV2[0]);
			psl::vec3 tangent = ((edge1 * deltaUV2[1] - edge2 * deltaUV1[1])*r);
			tangent = psl::math::normalize(tangent);
			tangents[indices[i + 0]] = tangent;
			tangents[indices[i + 1]] = tangent;
			tangents[indices[i + 2]] = tangent;
			/*
			psl::vec3 bitangent = psl::math::normalize((edge2 * deltaUV1[0] - edge1 * deltaUV2[0])*r);
			v0.m_bitangent = bitangent;
			v1.m_bitangent = bitangent;
			v2.m_bitangent = bitangent;
			*/
		}
		return tangents;
	}

	/// \brief generates tangent information for the geometry data.
	///
	/// generates tangent information for the given geometry data, which, on success, is injected into the input
	/// handle.
	/// \returns if the operation was successful (true) or not (false).
	/// \param[in, out] geometry_data the geometry data container.
	/// \note it is expected this is valid geometry data, only minimal to no data validation is done.
	static bool generate_tangents(core::resource::handle<core::data::geometry> geometry_data)
	{
		if(geometry_data.resource_state() != core::resource::state::LOADED)
			return false;

		auto posStreamOpt = geometry_data->vertices(core::data::geometry::constants::POSITION);
		auto uvStreamOpt = geometry_data->vertices(core::data::geometry::constants::TEX);
		if(!posStreamOpt || !uvStreamOpt)
			return false;

		const auto& positions = posStreamOpt.value().get().as_vec3().value().get();
		const auto& uvs = uvStreamOpt.value().get().as_vec2().value().get();
		const auto& indices = geometry_data->indices();

		core::stream stream(core::stream::type::vec3);		
		stream.as_vec3().value().get()  = {generate_tangents(positions, uvs, indices)};
		if(stream.as_vec3().value().get().size() == positions.size())
		{
			geometry_data->vertices(core::data::geometry::constants::TANGENT, stream);
			return true;
		}
		return false;
	}

	/// \brief generates a quad primitive with the bounds coordinates.
	///
	/// the principal difference between this function and utility::geometry::create_plane() is that
	/// this method allows fine control of the extents. This can be handy for generating UI elements.
	/// \param[in, out] cache the cache to store the generated geometry information on.
	/// \param[in] top the extent in the +Y axis.
	/// \param[in] bottom the extent in the -Y axis.
	/// \param[in] left the extent in the +X axis.
	/// \param[in] right the extent in the -X axis.
	/// \returns the handle to the generated geometry data.
	static core::resource::handle<core::data::geometry> create_quad(core::resource::cache& cache, float top, float bottom, float left, float right)
	{
		core::stream vertStream{core::stream::type::vec3};
		core::stream normStream{core::stream::type::vec3};
		core::stream uvStream{core::stream::type::vec2};

		auto& vertices = vertStream.as_vec3().value().get();
		auto& normals = normStream.as_vec3().value().get();
		auto& uvs = uvStream.as_vec2().value().get();

		vertices.emplace_back(psl::vec3{right, top, 0.0f});
		vertices.emplace_back(psl::vec3{left, top, 0.0f});
		vertices.emplace_back(psl::vec3{left, bottom, 0.0f});
		vertices.emplace_back(psl::vec3{right, bottom, 0.0f});

		normals.emplace_back(psl::vec3{1.0f, 1.0f, 1.0f});
		normals.emplace_back(psl::vec3{1.0f, 1.0f, 1.0f});
		normals.emplace_back(psl::vec3{1.0f, 1.0f, 1.0f});
		normals.emplace_back(psl::vec3{1.0f, 1.0f, 1.0f});

		uvs.emplace_back(psl::vec2{1.0f, 1.0f});
		uvs.emplace_back(psl::vec2{0.0f, 1.0f});
		uvs.emplace_back(psl::vec2{0.0f, 0.0f});
		uvs.emplace_back(psl::vec2{1.0f, 0.0f});

		auto boxGeomData = core::resource::create<core::data::geometry>(cache);
		boxGeomData.load();

		boxGeomData->vertices(core::data::geometry::constants::POSITION, vertStream);
		boxGeomData->vertices(core::data::geometry::constants::NORMAL, normStream);
		boxGeomData->vertices(core::data::geometry::constants::TEX, uvStream);

		boxGeomData->indices(std::vector<uint32_t>{0,1,2, 2,3,0});

		generate_tangents(boxGeomData);

		return boxGeomData;
	}

	/// \brief generates a plane primitive with the bounds coordinates.
	///
	/// the principal differences between this method and utility::geometry::create_quad() is that
	/// the plane can be subdivided, and has finer control over the UV's, making it better suited for
	/// 3D geometry.
	/// \param[in, out] cache the cache to store the generated geometry information on.
	/// \param[in] size the extent in the XZ axis.
	/// \param[in] subdivisions the amount of times to subdivide the mesh in the XZ axis.
	/// \param[in] uvScale multiplier on the UV scale, where {1, 1} is the normalized coordinates.
	/// \returns the handle to the generated geometry data.
	/// \note the final size of the object is always 2x the extents. This means that a size {2, 1} results into a min-coordinate {-2, -1} and max-coordinate {2, 1}.
	static core::resource::handle<core::data::geometry> create_plane(core::resource::cache& cache, psl::vec2 size = psl::vec2::one, psl::ivec2 subdivisions = psl::ivec2(1, 1), psl::vec2 uvScale = psl::vec2::one)
	{
		core::stream vertStream{core::stream::type::vec3};
		core::stream normStream{core::stream::type::vec3};
		core::stream uvStream{core::stream::type::vec2};

		auto& vertices = vertStream.as_vec3().value().get();
		auto& normals = normStream.as_vec3().value().get();
		auto& uvs = uvStream.as_vec2().value().get();

		vertices.resize((subdivisions[0] + 1) * (subdivisions[1] + 1));
		normals.resize((subdivisions[0] + 1) * (subdivisions[1] + 1));
		uvs.resize((subdivisions[0] + 1) * (subdivisions[1] + 1));

		psl::vec2 offset = (size * -0.5f);
		for(auto i = 0, y = 0; y <= subdivisions[1]; y++)
		{
			for(auto x = 0; x <= subdivisions[0]; x++, i++)
			{
				vertices[i] = psl::vec3(((float)x / ((float)subdivisions[0]))*size[0] + offset[0], 0, ((float)y / ((float)subdivisions[1]))*size[1] + offset[1]);
				normals[i] = psl::vec3::up;
				uvs[i] = psl::vec2(((float)x / ((float)subdivisions[0]))*uvScale[0], ((float)y / ((float)subdivisions[1]))*uvScale[1]);
			}
		}

		std::vector<uint32_t> indexBuffer((subdivisions[0]) * (subdivisions[1]) * 6);
		for(auto ti = 0, vi = 0, y = 0; y < subdivisions[1]; y++, vi++)
		{
			for(auto x = 0; x < subdivisions[0]; x++, ti += 6, vi++)
			{
				indexBuffer[ti] = vi;
				indexBuffer[ti + 3] = indexBuffer[ti + 2] = vi + 1;
				indexBuffer[ti + 4] = indexBuffer[ti + 1] = vi + subdivisions[0] + 1;
				indexBuffer[ti + 5] = vi + subdivisions[0] + 2;
			}
		}


		auto boxGeomData = core::resource::create<core::data::geometry>(cache);
		boxGeomData.load();

		boxGeomData->vertices(core::data::geometry::constants::POSITION, vertStream);
		boxGeomData->vertices(core::data::geometry::constants::NORMAL, normStream);
		boxGeomData->vertices(core::data::geometry::constants::TEX, uvStream);

		boxGeomData->indices(indexBuffer);

		generate_tangents(boxGeomData);

		return boxGeomData;
	}

	/// \brief generates a box primitive with a pivot of {0,0,0}.
	///
	/// \param[in, out] cache the cache to store the generated geometry information on.
	/// \param[in] scale the scale of the object.
	/// \returns the handle to the generated geometry data.
	/// \note unlike extent, scale implies that the final size is equal to the scale (i.e. an object of 1 unit at a scale of 1 is equal to 1unit).
	static core::resource::handle<core::data::geometry> create_box(core::resource::cache& cache, psl::vec3 scale = psl::vec3::one)
	{
		float length = scale[0];
		float width = scale[1];
		float height = scale[2];

		psl::vec3 p0 = psl::vec3(-length * .5f, -width * .5f, height * .5f);
		psl::vec3 p1 = psl::vec3(length * .5f, -width * .5f, height * .5f);
		psl::vec3 p2 = psl::vec3(length * .5f, -width * .5f, -height * .5f);
		psl::vec3 p3 = psl::vec3(-length * .5f, -width * .5f, -height * .5f);

		psl::vec3 p4 = psl::vec3(-length * .5f, width * .5f, height * .5f);
		psl::vec3 p5 = psl::vec3(length * .5f, width * .5f, height * .5f);
		psl::vec3 p6 = psl::vec3(length * .5f, width * .5f, -height * .5f);
		psl::vec3 p7 = psl::vec3(-length * .5f, width * .5f, -height * .5f);

		std::vector<psl::vec3> vertices
		{
			// Bottom
			p0, p1, p2, p3,

			// Left
			p7, p4, p0, p3,

			// Front
			p4, p5, p1, p0,

			// Back
			p6, p7, p3, p2,

			// Right
			p5, p6, p2, p1,

			// Top
			p7, p6, p5, p4
		};

		std::vector<psl::vec3> normals
		{
			// Bottom
			psl::vec3::down, psl::vec3::down, psl::vec3::down, psl::vec3::down,

			// Right
			psl::vec3::right, psl::vec3::right, psl::vec3::right, psl::vec3::right,

			// Front
			psl::vec3::forward, psl::vec3::forward, psl::vec3::forward, psl::vec3::forward,

			// Back
			psl::vec3::back, psl::vec3::back, psl::vec3::back, psl::vec3::back,

			// Left
			psl::vec3::left, psl::vec3::left, psl::vec3::left, psl::vec3::left,

			// Top
			psl::vec3::up, psl::vec3::up, psl::vec3::up, psl::vec3::up
		};

		psl::vec2 _00(0.f, 0.f);
		psl::vec2 _10(1.f, 0.f);
		psl::vec2 _01(0.f, 1.f);
		psl::vec2 _11(1.f, 1.f);

		std::vector<psl::vec2> uvs
		{
			// Bottom
			_11, _01, _00, _10,

			// Left
			_11, _01, _00, _10,

			// Front
			_11, _01, _00, _10,

			// Back
			_11, _01, _00, _10,

			// Right
			_11, _01, _00, _10,

			// Top
			_11, _01, _00, _10,
		};

		std::vector<uint32_t> triangles
		{
			// Bottom
			3, 1, 0,
				3, 2, 1,

				// Left
				3 + 4 * 1, 1 + 4 * 1, 0 + 4 * 1,
				3 + 4 * 1, 2 + 4 * 1, 1 + 4 * 1,

				// Front
				3 + 4 * 2, 1 + 4 * 2, 0 + 4 * 2,
				3 + 4 * 2, 2 + 4 * 2, 1 + 4 * 2,

				// Back
				3 + 4 * 3, 1 + 4 * 3, 0 + 4 * 3,
				3 + 4 * 3, 2 + 4 * 3, 1 + 4 * 3,

				// Right
				3 + 4 * 4, 1 + 4 * 4, 0 + 4 * 4,
				3 + 4 * 4, 2 + 4 * 4, 1 + 4 * 4,

				// Top
				3 + 4 * 5, 1 + 4 * 5, 0 + 4 * 5,
				3 + 4 * 5, 2 + 4 * 5, 1 + 4 * 5,

		};

		auto boxGeomData = core::resource::create<core::data::geometry>(cache);
		boxGeomData.load();

		core::stream vertStream{core::stream::type::vec3};
		core::stream normStream{core::stream::type::vec3};
		core::stream uvStream{core::stream::type::vec2};

		vertStream.as_vec3().value().get().resize(vertices.size());
		memcpy(vertStream.data(), vertices.data(), sizeof(psl::vec3) * vertices.size());
		normStream.as_vec3().value().get().resize(normals.size());
		memcpy(normStream.data(), normals.data(), sizeof(psl::vec3) * normals.size());
		uvStream.as_vec2().value().get().resize(uvs.size());
		memcpy(uvStream.data(), uvs.data(), sizeof(psl::vec2) * uvs.size());

		boxGeomData->vertices(core::data::geometry::constants::POSITION, vertStream);
		boxGeomData->vertices(core::data::geometry::constants::NORMAL, normStream);
		boxGeomData->vertices(core::data::geometry::constants::TEX, uvStream);

		boxGeomData->indices(triangles);

		generate_tangents(boxGeomData);

		return boxGeomData;
	}
}