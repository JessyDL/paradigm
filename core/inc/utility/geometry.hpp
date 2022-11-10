#pragma once

#include "data/geometry.hpp"
#include "psl/math/math.hpp"
#include <cmath>
#include <cstdlib>
#include <vector>

/// \brief various geometry and geometry related operation utilities.
namespace utility::geometry
{
	/// \brief generates tangent information for the geometry data
	/// \returns the generated tangent information.
	/// \param[in] positions vertex position info
	/// \param[in] uvs vertex UVs
	/// \param[in] indices triangle indices.
	/// \note it is expected this is valid geometry data, only minimal to no data validation is done.
	static std::vector<psl::vec3> generate_tangents(const std::vector<psl::vec3>& positions,
													const std::vector<psl::vec2>& uvs,
													const std::vector<uint32_t>& indices)
	{
		std::vector<psl::vec3> tangents;
		tangents.resize(positions.size());

		for(size_t i = 0; i < indices.size(); i += 3)
		{
			// Shortcuts for vertices
			const auto& v0 = positions[indices[i + 0]];
			const auto& v1 = positions[indices[i + 1]];
			const auto& v2 = positions[indices[i + 2]];

			const auto& u0 = uvs[indices[i + 0]];
			const auto& u1 = uvs[indices[i + 1]];
			const auto& u2 = uvs[indices[i + 2]];

			psl::vec3 edge1	   = v1 - v0;
			psl::vec3 edge2	   = v2 - v0;
			psl::vec2 deltaUV1 = u1 - u0;
			psl::vec2 deltaUV2 = u2 - u0;

			float r					 = 1.0f / (deltaUV1[0] * deltaUV2[1] - deltaUV1[1] * deltaUV2[0]);
			psl::vec3 tangent		 = ((edge1 * deltaUV2[1] - edge2 * deltaUV1[1]) * r);
			tangent					 = psl::math::normalize(tangent);
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
	static bool generate_tangents(core::resource::handle<core::data::geometry_t> geometry_data)
	{
		if(geometry_data.state() != core::resource::status::loaded) return false;

		if(!geometry_data->contains(core::data::geometry_t::constants::POSITION) ||
		   !geometry_data->contains(core::data::geometry_t::constants::TEX))
			return false;

		const auto& positions =
		  geometry_data->vertices(core::data::geometry_t::constants::POSITION).get<core::vertex_stream_t::type::vec3>();
		const auto& uvs =
		  geometry_data->vertices(core::data::geometry_t::constants::TEX).get<core::vertex_stream_t::type::vec2>();
		const auto& indices = geometry_data->indices();

		core::vertex_stream_t stream(core::vertex_stream_t::type::vec3);
		stream.get<core::vertex_stream_t::type::vec3>() = {generate_tangents(positions, uvs, indices)};
		if(stream.get<core::vertex_stream_t::type::vec3>().size() == positions.size())
		{
			geometry_data->vertices(core::data::geometry_t::constants::TANGENT, stream);
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
	static core::resource::handle<core::data::geometry_t>
	create_quad(core::resource::cache_t& cache, float top, float bottom, float left, float right)
	{
		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t normStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		auto& vertices = vertStream.get<core::vertex_stream_t::type::vec3>();
		auto& normals  = normStream.get<core::vertex_stream_t::type::vec3>();
		auto& uvs	   = uvStream.get<core::vertex_stream_t::type::vec2>();

		vertices.emplace_back(psl::vec3 {right, top, 0.0f});
		vertices.emplace_back(psl::vec3 {left, top, 0.0f});
		vertices.emplace_back(psl::vec3 {left, bottom, 0.0f});
		vertices.emplace_back(psl::vec3 {right, bottom, 0.0f});

		normals.emplace_back(psl::vec3 {1.0f, 1.0f, 1.0f});
		normals.emplace_back(psl::vec3 {1.0f, 1.0f, 1.0f});
		normals.emplace_back(psl::vec3 {1.0f, 1.0f, 1.0f});
		normals.emplace_back(psl::vec3 {1.0f, 1.0f, 1.0f});

		uvs.emplace_back(psl::vec2 {1.0f, 1.0f});
		uvs.emplace_back(psl::vec2 {0.0f, 1.0f});
		uvs.emplace_back(psl::vec2 {0.0f, 0.0f});
		uvs.emplace_back(psl::vec2 {1.0f, 0.0f});

		auto boxGeomData = cache.create<core::data::geometry_t>();

		boxGeomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
		boxGeomData->vertices(core::data::geometry_t::constants::NORMAL, normStream);
		boxGeomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		boxGeomData->indices(std::vector<uint32_t> {0, 1, 2, 2, 3, 0});

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
	/// \note the final size of the object is always 2x the extents. This means that a size {2, 1} results into a
	/// min-coordinate {-2, -1} and max-coordinate {2, 1}.
	static core::resource::handle<core::data::geometry_t> create_plane(core::resource::cache_t& cache,
																	   psl::vec2 size		   = psl::vec2::one,
																	   psl::ivec2 subdivisions = psl::ivec2(1, 1),
																	   psl::vec2 uvScale	   = psl::vec2::one)
	{
		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t normStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		auto& vertices = vertStream.get<core::vertex_stream_t::type::vec3>();
		auto& normals  = normStream.get<core::vertex_stream_t::type::vec3>();
		auto& uvs	   = uvStream.get<core::vertex_stream_t::type::vec2>();

		vertices.resize((subdivisions[0] + 1) * (subdivisions[1] + 1));
		normals.resize((subdivisions[0] + 1) * (subdivisions[1] + 1));
		uvs.resize((subdivisions[0] + 1) * (subdivisions[1] + 1));

		psl::vec2 offset = (size * -0.5f);
		for(auto i = 0, y = 0; y <= subdivisions[1]; y++)
		{
			for(auto x = 0; x <= subdivisions[0]; x++, i++)
			{
				vertices[i] = psl::vec3(((float)x / ((float)subdivisions[0])) * size[0] + offset[0],
										0,
										((float)y / ((float)subdivisions[1])) * size[1] + offset[1]);
				normals[i]	= psl::vec3::up;
				uvs[i]		= psl::vec2(((float)x / ((float)subdivisions[0])) * uvScale[0],
									((float)y / ((float)subdivisions[1])) * uvScale[1]);
			}
		}

		std::vector<uint32_t> indexBuffer((subdivisions[0]) * (subdivisions[1]) * 6);
		for(auto ti = 0, vi = 0, y = 0; y < subdivisions[1]; y++, vi++)
		{
			for(auto x = 0; x < subdivisions[0]; x++, ti += 6, vi++)
			{
				indexBuffer[ti]		= vi;
				indexBuffer[ti + 3] = indexBuffer[ti + 2] = vi + 1;
				indexBuffer[ti + 4] = indexBuffer[ti + 1] = vi + subdivisions[0] + 1;
				indexBuffer[ti + 5]						  = vi + subdivisions[0] + 2;
			}
		}


		auto boxGeomData = cache.create<core::data::geometry_t>();

		boxGeomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
		boxGeomData->vertices(core::data::geometry_t::constants::NORMAL, normStream);
		boxGeomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		boxGeomData->indices(indexBuffer);

		generate_tangents(boxGeomData);

		return boxGeomData;
	}

	static core::resource::handle<core::data::geometry_t>
	create_line(core::resource::cache_t& cache, psl::vec3 pos1, psl::vec3 pos2)
	{
		psl::static_array<psl::vec3, 2> vertices {pos1, pos2};
		psl::static_array<core::data::geometry_t::index_size_t, 2> indices {0, 1};

		auto geomData = cache.create<core::data::geometry_t>();

		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};

		vertStream.get<core::vertex_stream_t::type::vec3>().resize(vertices.size());
		memcpy(vertStream.data(), vertices.data(), sizeof(psl::vec3) * vertices.size());

		geomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);

		geomData->indices(indices);

		return geomData;
	}

	static core::resource::handle<core::data::geometry_t> create_line_quad(core::resource::cache_t& cache,
																		   psl::vec3 scale = psl::vec3::one)
	{
		float length = scale[0] * 0.5f;
		float width	 = scale[1] * 0.5f;
		float height = scale[2] * 0.5f;

		psl::vec3 p0 = psl::vec3(-length, -width, height);
		psl::vec3 p1 = psl::vec3(length, -width, height);
		psl::vec3 p2 = psl::vec3(length, -width, -height);
		psl::vec3 p3 = psl::vec3(-length, -width, -height);
		psl::static_array<psl::vec3, 4> vertices {p0, p1, p2, p3};

		psl::vec2 _00(0.f, 0.f);
		psl::vec2 _10(1.f, 0.f);
		psl::vec2 _01(0.f, 1.f);
		psl::vec2 _11(1.f, 1.f);

		psl::static_array<psl::vec2, 4> uvs {_11, _01, _00, _10};


		psl::static_array<core::data::geometry_t::index_size_t, 8> indices {0, 1, 1, 2, 2, 3, 3, 0};

		auto geomData = cache.create<core::data::geometry_t>();

		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		vertStream.get<core::vertex_stream_t::type::vec3>().resize(vertices.size());
		memcpy(vertStream.data(), vertices.data(), sizeof(psl::vec3) * vertices.size());
		uvStream.get<core::vertex_stream_t::type::vec2>().resize(uvs.size());
		memcpy(uvStream.data(), uvs.data(), sizeof(psl::vec2) * uvs.size());

		geomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
		geomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		geomData->indices(indices);

		return geomData;
	}
	static core::resource::handle<core::data::geometry_t> create_line_cube(core::resource::cache_t& cache,
																		   psl::vec3 scale = psl::vec3::one)
	{
		float length = scale[0] * 0.5f;
		float width	 = scale[1] * 0.5f;
		float height = scale[2] * 0.5f;

		psl::vec3 p0 = psl::vec3(-length, -width, height);
		psl::vec3 p1 = psl::vec3(length, -width, height);
		psl::vec3 p2 = psl::vec3(length, -width, -height);
		psl::vec3 p3 = psl::vec3(-length, -width, -height);

		psl::vec3 p4 = psl::vec3(-length, width, height);
		psl::vec3 p5 = psl::vec3(length, width, height);
		psl::vec3 p6 = psl::vec3(length, width, -height);
		psl::vec3 p7 = psl::vec3(-length, width, -height);

		std::vector<psl::vec3> vertices {
		  p0,
		  p1,
		  p2,
		  p3,
		  p4,
		  p5,
		  p6,
		  p7,
		};

		psl::vec2 _00(0.f, 0.f);
		psl::vec2 _10(1.f, 0.f);
		psl::vec2 _01(0.f, 1.f);
		psl::vec2 _11(1.f, 1.f);

		std::vector<psl::vec2> uvs {
		  _11,
		  _01,
		  _00,
		  _10,	  // Bottom
		  _11,
		  _01,
		  _00,
		  _10,	  // Left
		};

		std::vector<core::data::geometry_t::index_size_t> indices {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
																   6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};


		auto geomData = cache.create<core::data::geometry_t>();

		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		vertStream.get<core::vertex_stream_t::type::vec3>().resize(vertices.size());
		memcpy(vertStream.data(), vertices.data(), sizeof(psl::vec3) * vertices.size());
		uvStream.get<core::vertex_stream_t::type::vec2>().resize(uvs.size());
		memcpy(uvStream.data(), uvs.data(), sizeof(psl::vec2) * uvs.size());

		geomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
		geomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		geomData->indices(indices);

		return geomData;
	}
	/// \brief generates a box primitive with a pivot of {0,0,0}.
	///
	/// \param[in, out] cache the cache to store the generated geometry information on.
	/// \param[in] scale the scale of the object.
	/// \returns the handle to the generated geometry data.
	/// \note unlike extent, scale implies that the final size is equal to the scale (i.e. an object of 1 unit at a
	/// scale of 1 is equal to 1unit).
	static core::resource::handle<core::data::geometry_t> create_box(core::resource::cache_t& cache,
																	 psl::vec3 scale = psl::vec3::one)
	{
		float length = scale[0] * 0.5f;
		float width	 = scale[1] * 0.5f;
		float height = scale[2] * 0.5f;

		psl::vec3 p0 = psl::vec3(-length, -width, height);
		psl::vec3 p1 = psl::vec3(length, -width, height);
		psl::vec3 p2 = psl::vec3(length, -width, -height);
		psl::vec3 p3 = psl::vec3(-length, -width, -height);

		psl::vec3 p4 = psl::vec3(-length, width, height);
		psl::vec3 p5 = psl::vec3(length, width, height);
		psl::vec3 p6 = psl::vec3(length, width, -height);
		psl::vec3 p7 = psl::vec3(-length, width, -height);

		std::vector<psl::vec3> vertices {
		  p0, p1, p2, p3,	 // Bottom
		  p7, p4, p0, p3,	 // Left
		  p4, p5, p1, p0,	 // Front
		  p6, p7, p3, p2,	 // Back
		  p5, p6, p2, p1,	 // Right
		  p7, p6, p5, p4	 // Top
		};

		std::vector<psl::vec3> normals {
		  psl::vec3::down,	  psl::vec3::down,	  psl::vec3::down,	  psl::vec3::down,		 // Bottom
		  psl::vec3::right,	  psl::vec3::right,	  psl::vec3::right,	  psl::vec3::right,		 // Right
		  psl::vec3::forward, psl::vec3::forward, psl::vec3::forward, psl::vec3::forward,	 // Front
		  psl::vec3::back,	  psl::vec3::back,	  psl::vec3::back,	  psl::vec3::back,		 // Back
		  psl::vec3::left,	  psl::vec3::left,	  psl::vec3::left,	  psl::vec3::left,		 // Left
		  psl::vec3::up,	  psl::vec3::up,	  psl::vec3::up,	  psl::vec3::up			 // Top
		};

		psl::vec2 _00(0.f, 0.f);
		psl::vec2 _10(1.f, 0.f);
		psl::vec2 _01(0.f, 1.f);
		psl::vec2 _11(1.f, 1.f);

		std::vector<psl::vec2> uvs {
		  _11, _01, _00, _10,	 // Bottom
		  _11, _01, _00, _10,	 // Left
		  _11, _01, _00, _10,	 // Front
		  _11, _01, _00, _10,	 // Back
		  _11, _01, _00, _10,	 // Right
		  _11, _01, _00, _10,	 // Top
		};

		std::vector<uint32_t> triangles {
		  3,		 1,			0,			  // Bottom
		  3,		 2,			1,			  // Bottom
		  3 + 4 * 1, 1 + 4 * 1, 0 + 4 * 1,	  // Left
		  3 + 4 * 1, 2 + 4 * 1, 1 + 4 * 1,	  // Left
		  3 + 4 * 2, 1 + 4 * 2, 0 + 4 * 2,	  // Front
		  3 + 4 * 2, 2 + 4 * 2, 1 + 4 * 2,	  // Front
		  3 + 4 * 3, 1 + 4 * 3, 0 + 4 * 3,	  // Back
		  3 + 4 * 3, 2 + 4 * 3, 1 + 4 * 3,	  // Back
		  3 + 4 * 4, 1 + 4 * 4, 0 + 4 * 4,	  // Right
		  3 + 4 * 4, 2 + 4 * 4, 1 + 4 * 4,	  // Right
		  3 + 4 * 5, 1 + 4 * 5, 0 + 4 * 5,	  // Top
		  3 + 4 * 5, 2 + 4 * 5, 1 + 4 * 5,	  // Top

		};

		auto boxGeomData = cache.create<core::data::geometry_t>();

		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t normStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		vertStream.get<core::vertex_stream_t::type::vec3>().resize(vertices.size());
		memcpy(vertStream.data(), vertices.data(), sizeof(psl::vec3) * vertices.size());
		normStream.get<core::vertex_stream_t::type::vec3>().resize(normals.size());
		memcpy(normStream.data(), normals.data(), sizeof(psl::vec3) * normals.size());
		uvStream.get<core::vertex_stream_t::type::vec2>().resize(uvs.size());
		memcpy(uvStream.data(), uvs.data(), sizeof(psl::vec2) * uvs.size());

		boxGeomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
		boxGeomData->vertices(core::data::geometry_t::constants::NORMAL, normStream);
		boxGeomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		boxGeomData->indices(triangles);

		generate_tangents(boxGeomData);

		return boxGeomData;
	}
	static core::resource::handle<core::data::geometry_t>
	create_spherified_cube(core::resource::cache_t& cache, psl::vec3 scale = psl::vec3::one, uint32_t divisions = 4)
	{
		scale *= 0.5f;
		const float step = 1.0f / float(divisions);
		const psl::vec3 step3(step, step, step);

		static const psl::vec3 origins[6] = {psl::vec3(-1.0, -1.0, -1.0),
											 psl::vec3(1.0, -1.0, -1.0),
											 psl::vec3(1.0, -1.0, 1.0),
											 psl::vec3(-1.0, -1.0, 1.0),
											 psl::vec3(-1.0, 1.0, -1.0),
											 psl::vec3(-1.0, -1.0, 1.0)};
		static const psl::vec3 rights[6]  = {psl::vec3(2.0, 0.0, 0.0),
											 psl::vec3(0.0, 0.0, 2.0),
											 psl::vec3(-2.0, 0.0, 0.0),
											 psl::vec3(0.0, 0.0, -2.0),
											 psl::vec3(2.0, 0.0, 0.0),
											 psl::vec3(2.0, 0.0, 0.0)};
		static const psl::vec3 ups[6]	  = {psl::vec3(0.0, 2.0, 0.0),
											 psl::vec3(0.0, 2.0, 0.0),
											 psl::vec3(0.0, 2.0, 0.0),
											 psl::vec3(0.0, 2.0, 0.0),
											 psl::vec3(0.0, 0.0, 2.0),
											 psl::vec3(0.0, 0.0, -2.0)};

		std::vector<psl::vec3> vertices;

		for(uint32_t face = 0; face < 6; ++face)
		{
			const psl::vec3 origin = origins[face];
			const psl::vec3 right  = rights[face];
			const psl::vec3 up	   = ups[face];
			for(uint32_t j = 0; j < divisions + 1; ++j)
			{
				const psl::vec3 j3((float)j, (float)j, (float)j);
				for(uint32_t i = 0; i < divisions + 1; ++i)
				{
					const psl::vec3 i3((float)i, (float)i, (float)i);
					const psl::vec3 p  = origin + step3 * (i3 * right + j3 * up);
					const psl::vec3 p2 = p * p;
					const psl::vec3 n(p[0] * psl::math::sqrt(1.0f - 0.5f * (p2[1] + p2[2]) + p2[1] * p2[2] / 3.0f),
									  p[1] * psl::math::sqrt(1.0f - 0.5f * (p2[2] + p2[0]) + p2[2] * p2[0] / 3.0f),
									  p[2] * psl::math::sqrt(1.0f - 0.5f * (p2[0] + p2[1]) + p2[0] * p2[1] / 3.0f));
					vertices.emplace_back(n);
				}
			}
		}

		std::vector<uint32_t> triangles;
		const uint32_t k = divisions + 1;
		for(uint32_t face = 0; face < 6; ++face)
		{
			for(uint32_t j = 0; j < divisions; ++j)
			{
				for(uint32_t i = 0; i < divisions; ++i)
				{
					const uint32_t a = (face * k + j) * k + i;
					const uint32_t b = (face * k + j) * k + i + 1;
					const uint32_t c = (face * k + j + 1) * k + i;
					const uint32_t d = (face * k + j + 1) * k + i + 1;
					triangles.push_back(a);
					triangles.push_back(b);
					triangles.push_back(d);
					triangles.push_back(c);
					triangles.push_back(a);
					triangles.push_back(d);
				}
			}
		}

		auto boxGeomData = cache.create<core::data::geometry_t>();

		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t normStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		vertStream.get<core::vertex_stream_t::type::vec3>().resize(vertices.size());
		memcpy(vertStream.data(), vertices.data(), sizeof(psl::vec3) * vertices.size());
		auto& normals = normStream.get<core::vertex_stream_t::type::vec3>();
		auto& uvs	  = uvStream.get<core::vertex_stream_t::type::vec2>();
		normals.resize(vertices.size());
		uvs.resize(vertices.size());

		for(auto i = 0; i < vertices.size(); ++i)
		{
			normals[i] = psl::math::normalize(vertices[i]);
			uvs[i]	   = {normals[i][0], normals[i][1]};
		}

		boxGeomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
		boxGeomData->vertices(core::data::geometry_t::constants::NORMAL, normStream);
		boxGeomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		boxGeomData->indices(triangles);

		generate_tangents(boxGeomData);

		return boxGeomData;
	}

	static core::resource::handle<core::data::geometry_t> create_cone(core::resource::cache_t& cache,
																	  float height		 = 1.0f,
																	  float topRadius	 = 1.0f,
																	  float bottomRadius = 1.0f,
																	  uint32_t sides	 = 18)
	{
		auto tempBottom			= bottomRadius;
		bottomRadius			= topRadius * 0.5f;
		topRadius				= tempBottom * 0.5f;
		uint32_t heightSegments = 1;
		height *= 0.5f;
		int nbVerticesCap = sides + 1;

		std::vector<psl::vec3> vertices(nbVerticesCap + nbVerticesCap + sides * heightSegments * 2 + 2);
		uint32_t vert = 0;
		float _2pi	  = 3.14159265359f * 2.f;

		// Bottom cap
		vertices[vert++] = psl::vec3(0.f, -height, 0.f);
		while(vert <= sides)
		{
			float rad	   = (float)vert / sides * _2pi;
			vertices[vert] = psl::vec3(cos(rad) * bottomRadius, -height, sin(rad) * bottomRadius);
			vert++;
		}

		// Top cap
		vertices[vert++] = psl::vec3(0.f, height, 0.f);
		while(vert <= sides * 2 + 1)
		{
			float rad	   = (float)(vert - sides - 1) / sides * _2pi;
			vertices[vert] = psl::vec3(cos(rad) * topRadius, height, sin(rad) * topRadius);
			vert++;
		}

		// Sides
		int v = 0;
		while(vert <= vertices.size() - 4)
		{
			float rad		   = (float)v / sides * _2pi;
			vertices[vert]	   = psl::vec3(cos(rad) * topRadius, height, sin(rad) * topRadius);
			vertices[vert + 1] = psl::vec3(cos(rad) * bottomRadius, -height, sin(rad) * bottomRadius);
			vert += 2;
			v++;
		}
		vertices[vert]	   = vertices[sides * 2 + 2];
		vertices[vert + 1] = vertices[sides * 2 + 3];

		// Normals
		std::vector<psl::vec3> normals(vertices.size());
		vert = 0;

		// Bottom cap
		while(vert <= sides)
		{
			normals[vert++] = psl::vec3::down;
		}

		// Top cap
		while(vert <= sides * 2 + 1)
		{
			normals[vert++] = psl::vec3::up;
		}

		// Sides
		v = 0;
		while(vert <= vertices.size() - 4)
		{
			float rad  = (float)v / sides * _2pi;
			float fcos = cos(rad);
			float fsin = sin(rad);

			normals[vert]	  = psl::vec3(fcos, 0.f, fsin);
			normals[vert + 1] = normals[vert];

			vert += 2;
			v++;
		}
		normals[vert]	  = normals[sides * 2 + 2];
		normals[vert + 1] = normals[sides * 2 + 3];

		std::vector<psl::vec2> uvs(vertices.size());

		// Bottom cap
		uint32_t u = 0;
		uvs[u++]   = psl::vec2(0.5f, 0.5f);
		while(u <= sides)
		{
			float rad = (float)u / sides * _2pi;
			uvs[u]	  = psl::vec2(cos(rad) * .5f + .5f, sin(rad) * .5f + .5f);
			u++;
		}

		// Top cap
		uvs[u++] = psl::vec2(0.5f, 0.5f);
		while(u <= sides * 2 + 1)
		{
			float rad = (float)u / sides * _2pi;
			uvs[u]	  = psl::vec2(cos(rad) * .5f + .5f, sin(rad) * .5f + .5f);
			u++;
		}

		// Sides
		int u_sides = 0;
		while(u <= uvs.size() - 4)
		{
			float t	   = (float)u_sides / sides;
			uvs[u]	   = psl::vec2(t, 1.f);
			uvs[u + 1] = psl::vec2(t, 0.f);
			u += 2;
			u_sides++;
		}
		uvs[u]	   = psl::vec2(1.f, 1.f);
		uvs[u + 1] = psl::vec2(1.f, 0.f);

		// triangles
		uint32_t nbTriangles = sides + sides + sides * 2;
		std::vector<uint32_t> triangles(nbTriangles * 3 + 3);

		// Bottom cap
		uint32_t tri = 0;
		uint32_t i	 = 0;
		while(tri < sides - 1)
		{
			triangles[i]	 = 0;
			triangles[i + 1] = tri + 1;
			triangles[i + 2] = tri + 2;
			tri++;
			i += 3;
		}
		triangles[i]	 = 0;
		triangles[i + 1] = tri + 1;
		triangles[i + 2] = 1;
		tri++;
		i += 3;

		// Top cap
		// tri++;
		while(tri < sides * 2)
		{
			triangles[i]	 = tri + 2;
			triangles[i + 1] = tri + 1;
			triangles[i + 2] = nbVerticesCap;
			tri++;
			i += 3;
		}

		triangles[i]	 = nbVerticesCap + 1;
		triangles[i + 1] = tri + 1;
		triangles[i + 2] = nbVerticesCap;
		tri++;
		i += 3;
		tri++;

		// Sides
		while(tri <= nbTriangles)
		{
			triangles[i]	 = tri + 2;
			triangles[i + 1] = tri + 1;
			triangles[i + 2] = tri + 0;
			tri++;
			i += 3;

			triangles[i]	 = tri + 1;
			triangles[i + 1] = tri + 2;
			triangles[i + 2] = tri + 0;
			tri++;
			i += 3;
		}
		/*
		for (i = 0; i < triangles.size(); i += 3)
		{
			auto temp = triangles[i];
			triangles[i] = triangles[i + 2];
			triangles[i + 2] = temp;
		}
		*/

		auto geomData = cache.create<core::data::geometry_t>();

		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t normStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		vertStream.get<core::vertex_stream_t::type::vec3>().resize(vertices.size());
		memcpy(vertStream.data(), vertices.data(), sizeof(psl::vec3) * vertices.size());
		normStream.get<core::vertex_stream_t::type::vec3>().resize(normals.size());
		memcpy(normStream.data(), normals.data(), sizeof(psl::vec3) * normals.size());
		uvStream.get<core::vertex_stream_t::type::vec2>().resize(uvs.size());
		memcpy(uvStream.data(), uvs.data(), sizeof(psl::vec2) * uvs.size());

		geomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
		geomData->vertices(core::data::geometry_t::constants::NORMAL, normStream);
		geomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		geomData->indices(triangles);

		generate_tangents(geomData);

		return geomData;
	}

	static core::resource::handle<core::data::geometry_t> create_sphere(core::resource::cache_t& cache,
																		psl::vec3 scale	   = psl::vec3::one,
																		uint16_t longitude = 24,
																		uint16_t latitude  = 16)
	{
		std::vector<psl::vec3> vertices = std::vector<psl::vec3>((longitude + 1) * latitude + 2);
		float _pi						= 3.14159265359f;
		float _2pi						= _pi * 2.0f;

		vertices[0] = psl::vec3::up;
		for(auto lat = 0; lat < latitude; lat++)
		{
			float a1   = _pi * (float)(lat + 1) / (latitude + 1);
			float sin1 = sin(a1);
			float cos1 = cos(a1);

			for(auto lon = 0; lon <= longitude; lon++)
			{
				float a2   = _2pi * (float)(lon == longitude ? 0 : lon) / longitude;
				float sin2 = sin(a2);
				float cos2 = cos(a2);

				vertices[lon + lat * (longitude + 1) + 1] = psl::vec3(sin1 * cos2, cos1, sin1 * sin2);
			}
		}
		vertices[vertices.size() - 1] = psl::vec3::down;

		std::vector<psl::vec2> uvs = std::vector<psl::vec2>(vertices.size());
		uvs[0]					   = psl::vec2::up;
		uvs[uvs.size() - 1]		   = psl::vec2::zero;
		for(auto lat = 0; lat < latitude; lat++)
			for(auto lon = 0; lon <= longitude; lon++)
				uvs[lon + lat * (longitude + 1) + 1] =
				  psl::vec2((float)lon / longitude, 1.f - (float)(lat + 1) / (latitude + 1));

		auto nbFaces					= vertices.size();
		auto nbTriangles				= nbFaces * 2;
		auto nbIndexes					= nbTriangles * 3;
		std::vector<uint32_t> triangles = std::vector<uint32_t>(nbIndexes);

		// Top Cap
		uint32_t i = 0;
		for(auto lon = 0; lon < longitude; lon++)
		{
			triangles[i++] = lon + 2;
			triangles[i++] = lon + 1;
			triangles[i++] = 0;
		}

		// Middle
		for(auto lat = 0; lat < latitude - 1; lat++)
		{
			for(auto lon = 0; lon < longitude; lon++)
			{
				auto current = lon + lat * (longitude + 1) + 1;
				auto next	 = current + longitude + 1;

				triangles[i++] = current;
				triangles[i++] = current + 1;
				triangles[i++] = next + 1;

				triangles[i++] = current;
				triangles[i++] = next + 1;
				triangles[i++] = next;
			}
		}

		// Bottom Cap
		for(auto lon = 0; lon < longitude; lon++)
		{
			triangles[i++] = (uint32_t)vertices.size() - 1;
			triangles[i++] = (uint32_t)vertices.size() - (lon + 2) - 1;
			triangles[i++] = (uint32_t)vertices.size() - (lon + 1) - 1;
		}
		std::vector<psl::vec3> res_positions(vertices.size());
		std::vector<psl::vec3> res_normals(vertices.size());

		for(i = 0; i < vertices.size(); ++i)
		{
			res_positions[i] = vertices[i] * scale;
			res_normals[i]	 = psl::math::normalize(res_positions[i]);
		}


		float min = 1, max = -1;
		uint32_t northIndex {0}, southIndex {0};
		for(i = 0; i < vertices.size(); ++i)
		{
			if(res_positions[i][1] < min)
			{
				northIndex = i;
				min		   = res_positions[i][1];
			}
			if(res_positions[i][1] > max)
			{
				southIndex = i;
				max		   = res_positions[i][1];
			}
		}

		auto north		  = res_positions[northIndex];
		auto north_uv	  = uvs[northIndex];
		auto north_normal = res_normals[northIndex];
		auto south		  = res_positions[southIndex];
		auto south_uv	  = uvs[southIndex];
		auto south_normal = res_normals[southIndex];

		uint32_t verticeIndex = (uint32_t)res_positions.size() - 1;
		for(i = 0; i < triangles.size(); i += 3)
		{
			if(triangles[i] == northIndex)
			{
				// Vertex A = vertexData[triangles[i]];
				const auto& B = uvs[triangles[i + 1]];
				const auto& C = uvs[triangles[i + 2]];
				auto newNorth = north_uv;
				newNorth[0]	  = (B[0] + C[0]) / 2;
				verticeIndex++;
				res_positions.emplace_back(north);
				uvs.emplace_back(newNorth);
				res_normals.emplace_back(north_normal);
				triangles[i] = verticeIndex;
			}
			else if(triangles[i] == southIndex)
			{
				// Vertex A = vertexData[triangles[i]];
				const auto& B = uvs[triangles[i + 1]];
				const auto& C = uvs[triangles[i + 2]];
				auto newSouth = south_uv;
				newSouth[0]	  = (B[0] + C[0]) / 2;
				verticeIndex++;
				res_positions.emplace_back(south);
				uvs.emplace_back(newSouth);
				res_normals.emplace_back(south_normal);
				triangles[i] = verticeIndex;
			}
		}

		auto geomData = cache.create<core::data::geometry_t>();

		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t normStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		vertStream.get<core::vertex_stream_t::type::vec3>().resize(res_positions.size());
		memcpy(vertStream.data(), res_positions.data(), sizeof(psl::vec3) * res_positions.size());
		normStream.get<core::vertex_stream_t::type::vec3>().resize(res_normals.size());
		memcpy(normStream.data(), res_normals.data(), sizeof(psl::vec3) * res_normals.size());
		uvStream.get<core::vertex_stream_t::type::vec2>().resize(uvs.size());
		memcpy(uvStream.data(), uvs.data(), sizeof(psl::vec2) * uvs.size());

		geomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
		geomData->vertices(core::data::geometry_t::constants::NORMAL, normStream);
		geomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		geomData->indices(triangles);

		generate_tangents(geomData);

		return geomData;
	}

	static core::resource::handle<core::data::geometry_t>
	create_icosphere(core::resource::cache_t& cache, psl::vec3 scale = psl::vec3::one, size_t subdivisions = 2)
	{
		core::vertex_stream_t vertStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t normStream {core::vertex_stream_t::type::vec3};
		core::vertex_stream_t uvStream {core::vertex_stream_t::type::vec2};

		scale *= 0.5f;
		std::vector<psl::vec3>& vertices = vertStream.get<core::vertex_stream_t::type::vec3>();
		vertices.reserve(1024);
		std::unordered_map<uint64_t, uint32_t> middlePointIndexCache;

		auto get_middle_point = [](uint32_t p1,
								   uint32_t p2,
								   std::vector<psl::vec3>& vertices,
								   std::unordered_map<uint64_t, uint32_t>& cache) {
			// first check if we have it already
			bool firstIsSmaller	  = p1 < p2;
			uint64_t smallerIndex = firstIsSmaller ? p1 : p2;
			uint64_t greaterIndex = firstIsSmaller ? p2 : p1;
			uint64_t key		  = (smallerIndex << 32) + greaterIndex;

			auto it = cache.find(key);
			if(it != cache.end())
			{
				return it->second;
			}

			// not in cache, calculate it
			psl::vec3 point1 = vertices[p1];
			psl::vec3 point2 = vertices[p2];
			psl::vec3 middle =
			  psl::vec3((point1[0] + point2[0]) / 2.f, (point1[1] + point2[1]) / 2.f, (point1[2] + point2[2]) / 2.f);

			// add vertex makes sure point is on unit sphere
			uint32_t i = (uint32_t)vertices.size();
			vertices.push_back(psl::math::normalize(middle));

			// store it, return index
			cache[key] = i;

			return i;
		};
		using namespace std;

		// create 12 vertices o.f a icosahedron
		float t = (1.f + sqrtf(5.f)) / 2.f;

		vertices.push_back(psl::math::normalize(psl::vec3(-1.f, t, 0.f)));
		vertices.push_back(psl::math::normalize(psl::vec3(1.f, t, 0.f)));
		vertices.push_back(psl::math::normalize(psl::vec3(-1.f, -t, 0.f)));
		vertices.push_back(psl::math::normalize(psl::vec3(1.f, -t, 0.f)));

		vertices.push_back(psl::math::normalize(psl::vec3(0.f, -1.f, t)));
		vertices.push_back(psl::math::normalize(psl::vec3(0.f, 1.f, t)));
		vertices.push_back(psl::math::normalize(psl::vec3(0.f, -1.f, -t)));
		vertices.push_back(psl::math::normalize(psl::vec3(0.f, 1.f, -t)));

		vertices.push_back(psl::math::normalize(psl::vec3(t, 0.f, -1.f)));
		vertices.push_back(psl::math::normalize(psl::vec3(t, 0.f, 1.f)));
		vertices.push_back(psl::math::normalize(psl::vec3(-t, 0.f, -1.f)));
		vertices.push_back(psl::math::normalize(psl::vec3(-t, 0.f, 1.f)));

		struct triangle
		{
			triangle(uint32_t v1, uint32_t v2, uint32_t v3) : v1(v1), v2(v2), v3(v3) {};
			uint32_t v1;
			uint32_t v2;
			uint32_t v3;
		};

		std::vector<triangle> faces;

		// 5 faces around point 0
		faces.push_back(triangle(0, 11, 5));
		faces.push_back(triangle(0, 5, 1));
		faces.push_back(triangle(0, 1, 7));
		faces.push_back(triangle(0, 7, 10));
		faces.push_back(triangle(0, 10, 11));

		// 5 adjacent faces
		faces.push_back(triangle(1, 5, 9));
		faces.push_back(triangle(5, 11, 4));
		faces.push_back(triangle(11, 10, 2));
		faces.push_back(triangle(10, 7, 6));
		faces.push_back(triangle(7, 1, 8));

		// 5 faces around point 3
		faces.push_back(triangle(3, 9, 4));
		faces.push_back(triangle(3, 4, 2));
		faces.push_back(triangle(3, 2, 6));
		faces.push_back(triangle(3, 6, 8));
		faces.push_back(triangle(3, 8, 9));

		// 5 adjacent faces
		faces.push_back(triangle(4, 9, 5));
		faces.push_back(triangle(2, 4, 11));
		faces.push_back(triangle(6, 2, 10));
		faces.push_back(triangle(8, 6, 7));
		faces.push_back(triangle(9, 8, 1));

		// refine triangles
		for(uint32_t i = 0; i < subdivisions; i++)
		{
			std::vector<triangle> faces2;
			for(auto& tri : faces)
			{
				// replace triangle by 4 triangles
				uint32_t a = get_middle_point(tri.v1, tri.v2, vertices, middlePointIndexCache);
				uint32_t b = get_middle_point(tri.v2, tri.v3, vertices, middlePointIndexCache);
				uint32_t c = get_middle_point(tri.v3, tri.v1, vertices, middlePointIndexCache);

				faces2.push_back(triangle(tri.v1, a, c));
				faces2.push_back(triangle(tri.v2, b, a));
				faces2.push_back(triangle(tri.v3, c, b));
				faces2.push_back(triangle(a, b, c));
			}
			faces = faces2;
		}

		std::vector<psl::vec2>& res_uvs = uvStream.get<core::vertex_stream_t::type::vec2>();
		res_uvs.resize(vertices.size());

		for(int i = 0; i < vertices.size(); ++i)
		{
			res_uvs[i] = psl::vec2(atan2(vertices[i][2], vertices[i][0]) / 3.14159265359f / 2,
								   acos(vertices[i][1]) / 3.14159265359f);
			vertices[i] *= scale;
		}

		std::vector<uint32_t> tempIndices;

		for(uint32_t i = 0; i < faces.size(); i++)
		{
			tempIndices.push_back(faces[i].v1);
			tempIndices.push_back(faces[i].v2);
			tempIndices.push_back(faces[i].v3);
		}

		// GenerateTangents(vertexData, tempIndices);

		std::vector<uint32_t> problemIndices;
		for(int i = 0; i < faces.size(); ++i)
		{
			uint32_t a			= faces[i].v1;
			uint32_t b			= faces[i].v2;
			uint32_t c			= faces[i].v3;
			psl::vec3 texA		= psl::vec3(res_uvs[a], 0);
			psl::vec3 texB		= psl::vec3(res_uvs[b], 0);
			psl::vec3 texC		= psl::vec3(res_uvs[c], 0);
			psl::vec3 texNormal = psl::math::cross(texB - texA, texC - texA);
			if(texNormal[2] < 0) problemIndices.push_back(i);
		}


		uint32_t verticeIndex = (uint32_t)res_uvs.size() - 1;
		std::unordered_map<uint32_t, uint32_t> visited;
		for(auto& i : problemIndices)
		{
			uint32_t a = faces[i].v1;
			uint32_t b = faces[i].v2;
			uint32_t c = faces[i].v3;
			auto A	   = res_uvs[a];
			auto B	   = res_uvs[b];
			auto C	   = res_uvs[c];
			if(A[0] < 0.25f)
			{
				uint32_t tempA = a;
				auto it		   = visited.find(a);
				if(it == visited.end())
				{
					A[0] += 1;
					vertices.emplace_back(vertices[a]);
					res_uvs.emplace_back(A);
					verticeIndex++;
					visited[a] = verticeIndex;
					tempA	   = verticeIndex;
				}
				else
				{
					tempA = it->second;
				}
				a = tempA;
			}
			if(B[0] < 0.25f)
			{
				uint32_t tempB = b;
				auto it		   = visited.find(b);
				if(it == visited.end())
				{
					B[0] += 1;
					vertices.emplace_back(vertices[b]);
					res_uvs.emplace_back(B);
					verticeIndex++;
					visited[b] = verticeIndex;
					tempB	   = verticeIndex;
				}
				else
				{
					tempB = it->second;
				}
				b = tempB;
			}
			if(C[0] < 0.25f)
			{
				uint32_t tempC = c;
				auto it		   = visited.find(c);
				if(it == visited.end())
				{
					C[0] += 1;
					vertices.emplace_back(vertices[c]);
					res_uvs.emplace_back(C);
					verticeIndex++;
					visited[c] = verticeIndex;
					tempC	   = verticeIndex;
				}
				else
				{
					tempC = it->second;
				}
				c = tempC;
			}
			faces[i].v1 = a;
			faces[i].v2 = b;
			faces[i].v3 = c;
		}

		float min = 1, max = -1;
		uint32_t northIndex {0}, southIndex {0};
		for(uint32_t i = 0; i < vertices.size(); ++i)
		{
			if(vertices[i][1] < min)
			{
				northIndex = i;
				min		   = vertices[i][1];
			}
			if(vertices[i][1] > max)
			{
				southIndex = i;
				max		   = vertices[i][1];
			}
		}

		auto north	  = vertices[northIndex];
		auto north_uv = res_uvs[northIndex];
		auto south	  = vertices[southIndex];
		auto south_uv = res_uvs[southIndex];

		verticeIndex = (uint32_t)vertices.size() - 1;
		for(int i = 0; i < faces.size(); ++i)
		{
			if(faces[i].v1 == northIndex)
			{
				// Vertex A = vertexData[faces[i].v1];
				const auto& B = res_uvs[faces[i].v2];
				const auto& C = res_uvs[faces[i].v3];
				auto newNorth = north_uv;
				newNorth[0]	  = (B[0] + C[0]) / 2;
				verticeIndex++;

				vertices.emplace_back(north);
				res_uvs.emplace_back(newNorth);

				faces[i].v1 = verticeIndex;
			}
			else if(faces[i].v1 == southIndex)
			{
				// Vertex A = vertexData[faces[i].v1];
				const auto& B = res_uvs[faces[i].v2];
				const auto& C = res_uvs[faces[i].v3];
				auto newSouth = south_uv;
				newSouth[0]	  = (B[0] + C[0]) / 2;
				verticeIndex++;
				vertices.emplace_back(south);
				res_uvs.emplace_back(newSouth);
				faces[i].v1 = verticeIndex;
			}
		}

		std::vector<uint32_t> indices;
		for(uint32_t i = 0; i < faces.size(); i++)
		{
			indices.push_back(faces[i].v1);
			indices.push_back(faces[i].v2);
			indices.push_back(faces[i].v3);
		}
		std::vector<psl::vec3>& res_normals = normStream.get<core::vertex_stream_t::type::vec3>();
		res_normals.resize(vertices.size());
		for(int i = 0; i < vertices.size(); ++i)
		{
			res_normals[i] = psl::math::normalize(vertices[i]);
		}


		auto geomData = cache.create<core::data::geometry_t>();
		geomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
		geomData->vertices(core::data::geometry_t::constants::NORMAL, normStream);
		geomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

		std::vector<uint32_t> triangles(faces.size() * 3);
		size_t triangleI = 0;
		for(const auto& face : faces)
		{
			triangles[triangleI++] = face.v1;
			triangles[triangleI++] = face.v2;
			triangles[triangleI++] = face.v3;
		}

		geomData->indices(triangles);

		generate_tangents(geomData);

		return geomData;
	}

	static core::resource::handle<core::data::geometry_t> copy(core::resource::cache_t& cache,
															   core::resource::handle<core::data::geometry_t> target)
	{
		if(target.state() != core::resource::status::loaded) return {};

		auto geomData = cache.create<core::data::geometry_t>();
		for(const auto& [name, stream] : target->vertex_streams())
		{
			geomData->vertices(name, stream);
		}
		geomData->indices(target->indices());
		return geomData;
	}

	static core::resource::handle<core::data::geometry_t>
	merge(core::resource::cache_t& cache, const psl::array<core::resource::handle<core::data::geometry_t>>& geometry)
	{
		if(std::any_of(std::begin(geometry), std::end(geometry), [](const auto& geom) {
			   return geom.state() != core::resource::status::loaded;
		   }))
			return {};
		if(geometry.size() == 1) return copy(cache, geometry[0]);
		if(geometry.size() == 0) return {};

		using index_t			= core::data::geometry_t::index_size_t;
		auto indices			= geometry[0]->indices();
		auto streams			= geometry[0]->vertex_streams();
		auto source_vertexcount = geometry[0]->vertex_count();

		for(auto i = 1; i < geometry.size(); ++i)
		{
			for(const auto& [name, stream] : geometry[i]->vertex_streams())
			{
				if(streams.find(name) == std::end(streams)) continue;

				auto& dest	  = streams[name];
				auto bytesize = dest.bytesize();
				dest.resize_elementcount(dest.size() + stream.size());
				auto dest_ptr = (void*)((std::intptr_t)dest.data() + bytesize);
				memcpy(dest_ptr, stream.data(), stream.bytesize());
			}
			auto expected_size = source_vertexcount + geometry[i]->vertex_count();
			//.erase(std::remove_if(std::begin(streams), std::end(streams), [expected_size](const auto& name, const
			// auto& stream) { return stream.size() != expected_size; }), std::end(streams));

			auto previous_indices_count = static_cast<index_t>(indices.size());
			indices.insert(std::end(indices), std::begin(geometry[i]->indices()), std::end(geometry[i]->indices()));

			std::for_each(std::next(std::begin(indices), previous_indices_count),
						  std::end(indices),
						  [&source_vertexcount](auto& index) { index += source_vertexcount; });
			source_vertexcount = expected_size;
		}

		auto geomData = cache.create<core::data::geometry_t>();
		for(const auto& [name, stream] : streams)
		{
			geomData->vertices(name, stream);
		}
		geomData->indices(indices);
		return geomData;
	}

	static core::resource::handle<core::data::geometry_t>
	replicate(core::resource::handle<core::data::geometry_t> source, const psl::array<psl::vec3>& positions)
	{
		if(source.state() != core::resource::status::loaded) return {};
		using index_t = core::data::geometry_t::index_size_t;

		index_t vertices = source->vertex_count();

		{
			auto indices  = source->indices();
			index_t count = static_cast<index_t>(indices.size());
			indices.resize(indices.size() * positions.size());

			for(index_t i = 1; i < positions.size(); ++i)
			{
				for(index_t c = 0; c < count; ++c)
				{
					indices[(i * count) + c] = indices[c] + (vertices * i);
				}
			}
			source->indices(indices);
		}

		auto streams = source->vertex_streams();
		for(auto& [name, stream] : streams)
		{
			const core::vertex_stream_t& original = source->vertices(name);
			auto bytesize						  = original.bytesize();
			stream.resize_elementcount(stream.size() * positions.size());

			for(auto i = 0; i < positions.size(); ++i)
			{
				memcpy((void*)((size_t)stream.data() + (i * bytesize)), original.data(), bytesize);
			}
			if(name == core::data::geometry_t::constants::POSITION)
			{
				auto& proxy = stream.get<core::vertex_stream_t::type::vec3>();
				for(size_t i = 0; i < positions.size(); ++i)
				{
					for(size_t c = 0; c < vertices; ++c)
					{
						proxy[(i * (vertices)) + c] += positions[i];
					}
				}
			}
			source->vertices(name, stream);
		}

		return source;
	}

	static core::resource::handle<core::data::geometry_t>
	replicate(core::resource::cache_t& cache,
			  core::resource::handle<core::data::geometry_t> source,
			  const psl::array<psl::vec3>& positions)
	{
		return replicate(copy(cache, source), positions);
	}

	inline core::resource::handle<core::data::geometry_t>
	rotate(core::resource::handle<core::data::geometry_t> source,
		   psl::quat rotation,
		   psl::string_view channel = core::data::geometry_t::constants::POSITION)
	{
		psl_assert(source->vertices(channel).has_value(), "missing vertices channel '{}' in source", channel);
		source->transform(channel,
						  [rotation](psl::vec3& value) mutable { value = psl::math::rotate(rotation, value); });
		return source;
	}

	template <typename T>
	inline core::resource::handle<core::data::geometry_t>
	scale(core::resource::handle<core::data::geometry_t> source,
		  T scale,
		  psl::string_view channel = core::data::geometry_t::constants::POSITION)
	{
		psl_assert(source->vertices(channel).has_value(), "missing vertices channel '{}' in source", channel);
		source->transform(channel, [scale](T& value) mutable { value *= scale; });
		return source;
	}

	template <typename T>
	inline core::resource::handle<core::data::geometry_t>
	translate(core::resource::handle<core::data::geometry_t> source,
			  T translation,
			  psl::string_view channel = core::data::geometry_t::constants::POSITION)
	{
		psl_assert(source->vertices(channel).has_value(), "missing vertices channel '{}' in source", channel);
		source->transform(channel, [translation](T& value) mutable { value += translation; });
		return source;
	}

	inline core::resource::handle<core::data::geometry_t>
	copy_channel(core::resource::handle<core::data::geometry_t> geom,
				 psl::string_view source,
				 psl::string_view destination)
	{
		psl_assert(geom->vertices(source).has_value(), "missing vertices channel '{}' in geom", source);
		geom->vertices(destination, geom->vertices(source));
		return geom;
	}

	template <typename T>
	inline core::resource::handle<core::data::geometry_t>
	set_channel(core::resource::handle<core::data::geometry_t> source, psl::string_view channel, T value)
	{
		if(source->contains(channel))
		{
			if(source->vertices(channel).is<T>())
			{
				source->transform(channel, [value](T& old) mutable { old = value; });
				return source;
			}
		}

		psl::array<T> data(source->vertex_count(), value);
		core::vertex_stream_t stream {std::move(data)};
		source->vertices(channel, stream);
		return source;
	}
}	 // namespace utility::geometry
