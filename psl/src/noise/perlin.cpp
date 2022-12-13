#include "psl/noise/perlin.hpp"
#include "psl/math/math.hpp"

#include <numeric>
#include <random>

using namespace psl::noise;
using namespace psl::math;
perlin::perlin(uint32_t seed) noexcept
{
	m_Permutation.resize(256);

	std::iota(std::begin(m_Permutation), std::end(m_Permutation), 0);

	std::default_random_engine engine(seed);
	std::shuffle(std::begin(m_Permutation), std::end(m_Permutation), engine);
	m_Permutation.insert(std::end(m_Permutation), std::begin(m_Permutation), std::end(m_Permutation));
}

void perlin::reseed(uint32_t seed) noexcept
{
	std::iota(std::begin(m_Permutation), std::end(m_Permutation), 0);
	std::default_random_engine engine(seed);
	std::shuffle(std::begin(m_Permutation), std::end(m_Permutation), engine);
	std::copy(
	  std::begin(m_Permutation), std::next(std::begin(m_Permutation), 256), std::next(std::begin(m_Permutation), 256));
}

float perlin::noise(psl::vec3 coordinate, int32_t octaves, float persistence) const noexcept
{
	octaves = std::max<int32_t>(1, octaves);
	float res {0.0f};
	float amp {1.0f};

	for(int32_t i = 0; i < octaves; ++i)
	{
		auto X = static_cast<int32_t>(std::floor(coordinate[0])) & 255;
		auto Y = static_cast<int32_t>(std::floor(coordinate[1])) & 255;
		auto Z = static_cast<int32_t>(std::floor(coordinate[2])) & 255;

		auto fcoordinate = coordinate - psl::math::floor(coordinate);

		auto u = fade(fcoordinate[0]);
		auto v = fade(fcoordinate[1]);
		auto w = fade(fcoordinate[2]);

		auto A	= m_Permutation[X] + Y;
		auto AA = m_Permutation[A] + Z;
		auto AB = m_Permutation[A + size_t {1}] + Z;
		auto B	= m_Permutation[X + size_t {1}] + Y;
		auto BA = m_Permutation[B] + Z;
		auto BB = m_Permutation[B + size_t {1}] + Z;

		auto value = lerp(
		  w,
		  lerp(v,
			   lerp(u,
					grad(m_Permutation[AA], fcoordinate[0], fcoordinate[1], fcoordinate[2]),
					grad(m_Permutation[BA], fcoordinate[0] - 1, fcoordinate[1], fcoordinate[2])),
			   lerp(u,
					grad(m_Permutation[AB], fcoordinate[0], fcoordinate[1] - 1, fcoordinate[2]),
					grad(m_Permutation[BB], fcoordinate[0] - 1, fcoordinate[1] - 1, fcoordinate[2]))),
		  lerp(v,
			   lerp(u,
					grad(m_Permutation[AA + size_t {1}], fcoordinate[0], fcoordinate[1], fcoordinate[2] - 1),
					grad(m_Permutation[BA + size_t {1}], fcoordinate[0] - 1, fcoordinate[1], fcoordinate[2] - 1)),
			   lerp(u,
					grad(m_Permutation[AB + size_t {1}], fcoordinate[0], fcoordinate[1] - 1, fcoordinate[2] - 1),
					grad(m_Permutation[BB + size_t {1}], fcoordinate[0] - 1, fcoordinate[1] - 1, fcoordinate[2] - 1))));

		res += value * amp;
		coordinate *= 2.0f;
		amp *= persistence;
	}
	return res * 0.5f + 0.5f;
}


float perlin::noise(psl::vec2 coordinate, int32_t octaves, float persistence) const noexcept
{
	octaves = std::max<int32_t>(1, octaves);
	float res {0.0f};
	float amp {1.0f};

	for(int32_t i = 0; i < octaves; ++i)
	{
		auto X = static_cast<int32_t>(std::floor(coordinate[0])) & 255;
		auto Y = static_cast<int32_t>(std::floor(coordinate[1])) & 255;

		auto fcoordinate = coordinate - psl::math::floor(coordinate);

		auto u = fade(fcoordinate[0]);
		auto v = fade(fcoordinate[1]);
		auto w = fade(1.0f);

		auto A	= m_Permutation[X] + Y;
		auto AA = m_Permutation[A];
		auto AB = m_Permutation[A + size_t {1}];
		auto B	= m_Permutation[X + size_t {1}] + Y;
		auto BA = m_Permutation[B];
		auto BB = m_Permutation[B + size_t {1}];

		auto value = lerp(w,
						  lerp(v,
							   lerp(u,
									grad(m_Permutation[AA], fcoordinate[0], fcoordinate[1]),
									grad(m_Permutation[BA], fcoordinate[0] - 1, fcoordinate[1])),
							   lerp(u,
									grad(m_Permutation[AB], fcoordinate[0], fcoordinate[1] - 1),
									grad(m_Permutation[BB], fcoordinate[0] - 1, fcoordinate[1] - 1))),
						  lerp(v,
							   lerp(u,
									grad(m_Permutation[AA + size_t {1}], fcoordinate[0], fcoordinate[1]),
									grad(m_Permutation[BA + size_t {1}], fcoordinate[0] - 1, fcoordinate[1])),
							   lerp(u,
									grad(m_Permutation[AB + size_t {1}], fcoordinate[0], fcoordinate[1] - 1),
									grad(m_Permutation[BB + size_t {1}], fcoordinate[0] - 1, fcoordinate[1] - 1))));

		res += value * amp;
		coordinate *= 2.0f;
		amp *= persistence;
	}
	return res * 0.5f + 0.5f;
}
