#pragma once
#include "psl/array.hpp"
#include "psl/math/vec.hpp"

namespace psl::noise
{
	class perlin
	{
	  public:
		perlin(uint32_t seed = 5489U) noexcept;

		void reseed(uint32_t seed) noexcept;

		float noise(psl::vec2 coordinate, int32_t octaves = 1, float persistence = 0.5f) const noexcept;
		float noise(psl::vec3 coordinate, int32_t octaves = 1, float persistence = 0.5f) const noexcept;

	  private:
		psl::array<int32_t> m_Permutation;
	};
}	 // namespace psl::noise