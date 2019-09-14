#pragma once
#include "fwd/resource/resource.h"
#include "psl/serialization.h"
#include "psl/array.h"
#include "psl/math/matrix.h"
#include "geometry.h"

namespace core::data
{
	class skeleton
	{
		template <typename T, char... Char>
		using prop = psl::serialization::property<T, Char...>;

		using index_size_t = core::data::geometry::index_size_t;

	  public:
		class bone
		{
		  public:
			bone()  = default;
			~bone() = default;

			bone(const bone& other)		= delete;
			bone(bone&& other) noexcept = delete;
			bone& operator=(const bone& other) = delete;
			bone& operator=(bone&& other) noexcept = delete;
			size_t size() const noexcept; // return m_WeightIDs.size()
		  private:
			prop<psl::mat4x4, const_str("INV_BIND_POSE", 13)> m_InverseBindPoseMatrix;
			prop<psl::array<index_size_t>, const_str("WEIGHT_ID", 9)> m_WeightIDs; // vertex id for the given weight
			prop<psl::array<float>, const_str("WEIGHT_VALUE", 12)>
				m_WeighValue; // how much the current bone will affect the vertex
			prop<psl::string8_t, const_str("NAME", 4)> m_Name;
		};

		skeleton() = default;
		skeleton(core::resource::cache& cache, const core::resource::metadata& metaData,
				 psl::meta::file* metaFile) noexcept;
		~skeleton() = default;

		skeleton(const skeleton& other)		= delete;
		skeleton(skeleton&& other) noexcept = delete;
		skeleton& operator=(const skeleton& other) = delete;
		skeleton& operator=(skeleton&& other) noexcept = delete;

		bool has_bones() const noexcept;
		size_t size() const noexcept;

	  private:
		prop<psl::array<bone>, const_str("BONES", 5)> m_Bones;
	};
} // namespace core::data