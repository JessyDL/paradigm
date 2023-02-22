#pragma once

#include "core/data/geometry.hpp"
#include "psl/serialization/property.hpp"
#include "psl/serialization/serializer.hpp"
#include "psl/utility/cast.hpp"

namespace core::data {
class skeleton_t {
	friend class psl::serialization::accessor;
	static constexpr psl::string8::view serialization_name {"SKELETON"};

  public:
	using index_size_t	= core::data::geometry_t::index_size_t;
	using weight_size_t = float;

	class bone_t {
		friend class psl::serialization::accessor;
		static constexpr psl::string8::view serialization_name {"BONE"};

	  public:
		struct weight_t {
			constexpr weight_t() noexcept = default;
			constexpr weight_t(index_size_t vertex_v, weight_size_t value_v) noexcept
				: vertex(vertex_v), value(value_v) {};
			index_size_t vertex;
			weight_size_t value;
		};

		constexpr bone_t() = default;
		constexpr bone_t(psl::string8::view name, psl::mat4x4 transform = {}, psl::array<weight_t> weights = {})
			: m_Name(name), m_Transform(transform), m_Weights(weights) {};

		void name(psl::string_view value) { m_Name = value; }
		psl::string8::view name() const noexcept { return m_Name; }
		void transform(psl::mat4x4 value) { m_Transform = value; }
		psl::mat4x4 transform() const noexcept { return m_Transform; }
		void weights(psl::array<weight_t> const& values) { m_Weights = values; }
		psl::array<weight_t> const& weights() const noexcept { return m_Weights; }

	  private:
		template <typename S>
		void serialize(S& serializer) {
			serializer << m_Name << m_Transform;
			psl::serialization::property<"VERTICES", psl::array<index_size_t>> weight_vertices {};
			psl::serialization::property<"WEIGHT", psl::array<weight_size_t>> weight_values {};

			for(auto const& weight : m_Weights) {
				weight_vertices->emplace_back(weight.vertex);
				weight_values->emplace_back(weight.value);
			}

			serializer << weight_vertices << weight_values;
		};

		psl::serialization::property<"NAME", psl::string8_t> m_Name {};
		psl::serialization::property<"TRANSFORM", psl::mat4x4> m_Transform {};
		psl::array<weight_t> m_Weights {};
	};


	constexpr skeleton_t() = default;
	constexpr skeleton_t(psl::array<bone_t> bones) : m_Bones(bones) {};

	psl::array<bone_t> const& bones() const noexcept { return m_Bones; }
	void bones(psl::array<bone_t> values) { m_Bones = std::move(values); }

	bone_t const& bone(index_size_t index) const { return m_Bones->at(index); }
	bone_t& bone(index_size_t index) { return m_Bones->at(index); }
	void bone(index_size_t index, bone_t value) { m_Bones->at(index) = value; }

	index_size_t size() const noexcept { return psl::utility::narrow_cast<index_size_t>(m_Bones->size()); }

  private:
	template <typename S>
	void serialize(S& serializer) {
		serializer << m_Bones;
	}

	psl::serialization::property<"BONES", psl::array<bone_t>> m_Bones {};
};
}	 // namespace core::data
