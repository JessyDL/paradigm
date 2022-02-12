#pragma once

#include "data/stream.hpp"
#include "fwd/resource/resource.hpp"
#include "psl/array_view.hpp"
#include "psl/math/matrix.hpp"
#include "psl/ustring.hpp"
#include <unordered_map>
#include <vector>

/*
#define GEOMETRY_VERTEX_POSITION "GEOMETRY_VERTEX_POSITION"
#define GEOMETRY_VERTEX_COLOR "GEOMETRY_VERTEX_COLOR"
#define GEOMETRY_VERTEX_NORMAL "GEOMETRY_VERTEX_NORMAL"
#define GEOMETRY_VERTEX_TANGENT "GEOMETRY_VERTEX_TANGENT"
#define GEOMETRY_VERTEX_BITANGENT "GEOMETRY_VERTEX_BITANGENT"
#define GEOMETRY_VERTEX_TEX "GEOMETRY_VERTEX_TEX"
#define GEOMETRY_VERTEX_BONES "GEOMETRY_VERTEX_BONES"
#define GEOMETRY_BONES "GEOMETRY_BONES"
*/

/// \brief hard defined max bone weights that geometry can use
/// \todo implement a more robust system for this.
#define MAX_BONE_WEIGHTS 4

namespace psl
{
	template <typename T>
	class array_view;
}


namespace core::data
{
	/// \brief describes a stream of data that will be uploaded to the GPU as geometry data.
	///
	/// Unlike most engines, geometry data in paradigm is considered a collection of undescribed data, that has been
	/// keyed. There are some globally accepted keys defined for convenience at the core::data::geometry_t::constants
	/// namespace. Only 2 streams are mandatory (core::data::geometry_t::constants::POSITION and the indices stream), if
	/// those are not present, the data is considered invalid. All streams (except the index buffer stream) should have
	/// equally as many entries as the position buffer stream has. The index buffer stream should not reffer to
	/// positions larger than the size of the position buffer stream. \note these data streams can be anything, as long
	/// as atleast a position buffer and a index buffer is present. It's up to you to make sure your geometry object has
	/// all the required streams to correctly bind it to a core::ivk::material_t. \todo write an example of a custom
	/// stream. \todo support numbered streams (i.e. UV0, UV1, etc..).
	class geometry_t
	{
		friend class psl::serialization::accessor;

	  public:
		using index_size_t = uint32_t;
		using bone_t	   = psl::mat4x4;

		/// \brief contains globally pre-defined keys for common streams that most engines have.
		struct constants
		{
			/// \brief vertex position data key
			static constexpr psl::string_view POSITION = "GEOMETRY_VERTEX_POSITION";
			/// \brief vertex color data key
			static constexpr psl::string_view COLOR = "GEOMETRY_VERTEX_COLOR";
			/// \brief vertex normal information key
			static constexpr psl::string_view NORMAL = "GEOMETRY_VERTEX_NORMAL";
			/// \brief vertex tangent information key
			static constexpr psl::string_view TANGENT = "GEOMETRY_VERTEX_TANGENT";
			/// \brief vertex bitangent information key
			static constexpr psl::string_view BITANGENT = "GEOMETRY_VERTEX_BITANGENT";
			/// \brief vertex uv-coordinate key
			static constexpr psl::string_view TEX = "GEOMETRY_VERTEX_TEX";
			/// \brief vertex bone weight key
			static constexpr psl::string_view VERTEX_BONES = "GEOMETRY_VERTEX_BONES";
			/// \brief geometry bones key
			static constexpr psl::string_view BONES = "GEOMETRY_BONES";
		};
		geometry_t() = default;
		geometry_t(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile) noexcept;
		~geometry_t()				  = default;
		geometry_t(const geometry_t&) = delete;
		geometry_t(geometry_t&&)	  = delete;
		geometry_t& operator=(const geometry_t&) = delete;
		geometry_t& operator=(geometry_t&&) = delete;

		/// \brief returns the stream reference (if any found) for the given key.
		/// \returns the stream reference (if any found) for the given key.
		std::optional<std::reference_wrapper<const core::stream>>
		vertices(const psl::string_view name = constants::POSITION) const;

		/// \brief sets the stream data for the given key. If data was already present, then it gets replaced.
		/// \warning be sure that the stream has the correct amount of vertices as is expected by the index buffer. As
		/// you might be replacing/resizing the internal model data, we cannot error check this condition for you here.
		void vertices(const psl::string_view name, const core::stream& stream);
		template <typename T>
		void vertices(const psl::string_view name, psl::array<T> stream)
		{
			m_VertexStreams.value[psl::string {name}] = core::stream(std::move(stream));
		}
		/// \brief gets all indices that are currently assigned to this model data.
		/// \returns all indices that are currently assigned to this model data.
		const std::vector<index_size_t>& indices() const;
		/// \brief sets the indices.
		/// \warning be sure to check that the indices do not reffer to out of index locations in the position buffer.
		/// Seeing you might be replacing/resizing the model data, we cannot error check this condition for you here.
		void indices(psl::array_view<index_size_t> indices);

		/// \brief returns all the streams of data and their keys.
		/// \returns all the streams of data and their keys.
		const std::unordered_map<psl::string, core::stream>& vertex_streams() const;

		/// \brief helper method to check for validity of the model data in its current state.
		/// \returns true if the data passes all checks.
		bool is_valid() const noexcept;

		index_size_t vertex_count() const noexcept;
		index_size_t index_count() const noexcept;
		index_size_t triangles() const noexcept;
		size_t bytesize() const noexcept;
		size_t elements() const noexcept;

		/// \brief erases the given stream if found
		bool erase(psl::string_view name) noexcept;

		template <typename F>
		bool transform(psl::string_view name, F&& transformation)
		{
			if(auto it = m_VertexStreams.value.find(psl::string(name)); it != std::end(m_VertexStreams.value))
			{
				return it->second.transform(std::forward<F>(transformation));
			}
			return false;
		}

	  private:
		/// \brief serialization method to be used by the serializer when writing this container to the disk.
		/// \param[in] serializer the serialization object, consult the serialization namespace for more information.
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_VertexStreams << m_Indices;
		};

		/// \brief serialization name to be used by the serializer when writing and reading this container to and from
		/// disk.
		static constexpr psl::string8::view serialization_name {"GEOMETRY"};

		psl::serialization::property<"INDICES", std::vector<index_size_t>> m_Indices;
		psl::serialization::property<"STREAMS", std::unordered_map<psl::string, core::stream>> m_VertexStreams;
	};

}	 // namespace core::data
