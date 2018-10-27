#pragma once
#include "mat4x3.hpp"
#include "vec3.hpp"

namespace glm
{
	struct bbox
	{

		union 
		{
#pragma warning (disable:4201)
			struct
			{
				vec3 min;
				vec3 max;
				vec3 center;
				vec3 weightedCenter;
			};
#pragma warning (default:4201)
			mat4x3 data;
		};

		bbox(const vec3 &min, const vec3 &max, const vec3 &weightedCenter)	:	min(min), max(max), center((min+max)*0.5f), weightedCenter(weightedCenter) {	}
		bbox(const mat4x3 &mat) : data(mat)	{		}
		bbox()	: min(glm::vec3::Zero), max(glm::vec3::Zero), center(glm::vec3::Zero), weightedCenter(glm::vec3::Zero) {}

		const static glm::bbox Zero;
		const static glm::bbox One;
		const static glm::bbox Identity;
		const static glm::bbox Infinity;
	};

	struct ray
	{
		union
		{
#pragma warning (disable:4201)
			struct
			{
				vec3 origin;
				vec3 direction;
				vec3 inverseDirection;
				float slopes[6];
				float precomputation[6];
			};
			struct
			{
				float x;	//origin
				float y;
				float z;

				float i;	//direction
				float j;
				float k;

				float ii;	// inverse ray direction
				float ij;
				float ik;

				float s_yx;	// ray slopes
				float s_xy;
				float s_zy;
				float s_yz;
				float s_xz;
				float s_zx;

				float c_xy;	// precomputation
				float c_yx;
				float c_zy;
				float c_yz;
				float c_xz;
				float c_zx;
			};
#pragma warning (default:4201)
		};


		ray(const vec3 &origin, const vec3 &direction) : origin(origin), direction(direction) 
		{
			Recalculate();
		}
		ray()	: origin(vec3::Zero), direction(vec3::Up) 
		{
			Recalculate();
		}

		void Recalculate()
		{
			ii = 1.0f / i; // inverse ray direction
			ij = 1.0f / j;
			ik = 1.0f / k;

			s_yx = i * ij; // ray slopes
			s_xy = j * ii;
			s_zy = j * ik;
			s_yz = k * ij;
			s_xz = i * ik;
			s_zx = k * ii;

			c_xy = y - s_xy * x; // precomputation
			c_yx = x - s_yx * y;
			c_zy = y - s_zy * z;
			c_yz = z - s_yz * y;
			c_xz = z - s_xz * x;
			c_zx = x - s_zx * z;
		}
	};
}
