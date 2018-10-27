#pragma once
#include "ustring.h"
#include "string_utils.h"
#include <glm/glm.hpp>
#include <vector>

namespace utility
{
	template<>
	struct converter<glm::vec2>
	{
		static bool from_string(const std::vector<psl::string8::view>& string, glm::vec2* t, size_t start, size_t end)
		{
			if(end - start != 2)
				return false;
			t->x = std::stof(psl::string8_t(string[0 + start]));
			t->y = std::stof(psl::string8_t(string[1 + start]));
			return true;
		}

		static psl::string8_t to_string(const glm::vec2& t)
		{
			return std::to_string(t.x) + "," + std::to_string(t.y);
		}

		static glm::vec2 from_string(psl::string8::view str)
		{
			glm::vec2 result;
			if(auto split = utility::string::split(str, ","); split.size() == 2 && from_string(split, &result, 0, split.size()))
				return result;
			return glm::vec2::Infinity;
		}
	};

	template<>
	struct converter<glm::vec3>
	{
		static bool from_string(const std::vector<psl::string8::view>& string, glm::vec3* t, size_t start, size_t end)
		{
			if(end - start != 3)
				return false;
			t->x = std::stof(psl::string8_t(string[0 + start]));
			t->y = std::stof(psl::string8_t(string[1 + start]));
			t->z = std::stof(psl::string8_t(string[2 + start]));
			return true;
		}

		static psl::string8_t to_string(const glm::vec3& t)
		{
			return std::to_string(t.x) + "," + std::to_string(t.y) + "," + std::to_string(t.z);
		}

		static glm::vec3 from_string(psl::string8::view str)
		{
			glm::vec3 result;
			if(auto split = utility::string::split(str, ","); split.size() == 3 && from_string(split, &result, 0, split.size()))
				return result;
			return glm::vec3::Infinity;
		}
	};

	template<>
	struct converter<glm::vec4>
	{
		static bool from_string(const std::vector<psl::string8::view>& string, glm::vec4* t, size_t start, size_t end)
		{
			if(end - start != 4)
				return false;
			t->x = std::stof(psl::string8_t(string[0 + start]));
			t->y = std::stof(psl::string8_t(string[1 + start]));
			t->z = std::stof(psl::string8_t(string[2 + start]));
			t->w = std::stof(psl::string8_t(string[3 + start]));
			return true;
		}

		static psl::string8_t to_string(const glm::vec4& t)
		{
			return std::to_string(t.x) + "," + std::to_string(t.y) + "," + std::to_string(t.z) + "," + std::to_string(t.w);
		}

		static glm::vec4 from_string(psl::string8::view str)
		{
			glm::vec4 result;
			if(auto split = utility::string::split(str, ","); split.size() == 4 && from_string(split, &result, 0, split.size()))
				return result;
			return result;
		}
	};

	template<>
	struct converter<glm::mat2>
	{
		static bool from_string(const std::vector<psl::string8::view>& string, glm::mat2* t, size_t start, size_t end)
		{
			if(end - start != 4)
				return false;

			(*t)[0].x = std::stof(psl::string8_t(string[0 + start]));
			(*t)[0].y = std::stof(psl::string8_t(string[1 + start]));
			(*t)[1].x = std::stof(psl::string8_t(string[2 + start]));
			(*t)[1].y = std::stof(psl::string8_t(string[3 + start]));
			return true;
		};
		static psl::string8_t to_string(const glm::mat2& t)
		{
			return utility::to_string(t[0][0]) + "," + utility::to_string(t[0][1]) + "," +
				utility::to_string(t[1][0]) + "," + utility::to_string(t[1][1]);
		}

		static glm::mat2 from_string(psl::string8::view str)
		{
			glm::mat2 result;
			if(auto split = utility::string::split(str, ","); split.size() == 4 && from_string(split, &result, 0, split.size()))
				return result;
			return result;
		}
	};

	template<>
	struct converter<glm::mat3>
	{
		static bool from_string(const std::vector<psl::string8::view>& string, glm::mat3* t, size_t start, size_t end)
		{
			if(end - start != 9)
				return false;

			(*t)[0].x = std::stof(psl::string8_t(string[0 + start]));
			(*t)[0].y = std::stof(psl::string8_t(string[1 + start]));
			(*t)[0].z = std::stof(psl::string8_t(string[2 + start]));

			(*t)[1].x = std::stof(psl::string8_t(string[3 + start]));
			(*t)[1].y = std::stof(psl::string8_t(string[4 + start]));
			(*t)[1].z = std::stof(psl::string8_t(string[5 + start]));

			(*t)[2].x = std::stof(psl::string8_t(string[6 + start]));
			(*t)[2].y = std::stof(psl::string8_t(string[7 + start]));
			(*t)[2].z = std::stof(psl::string8_t(string[8 + start]));

			return true;
		};
		static psl::string8_t to_string(const glm::mat3& t)
		{
			return utility::to_string(t[0][0]) + "," + utility::to_string(t[0][1]) + "," + utility::to_string(t[0][2]) + "," +
				utility::to_string(t[1][0]) + "," + utility::to_string(t[1][1]) + "," + utility::to_string(t[1][2]) + "," +
				utility::to_string(t[2][0]) + "," + utility::to_string(t[2][1]) + "," + utility::to_string(t[2][2]);
		}

		static glm::mat3 from_string(psl::string8::view str)
		{
			glm::mat3 result;
			if(auto split = utility::string::split(str, ","); split.size() == 9 && from_string(split, &result, 0, split.size()))
				return result;
			return result;
		}
	};

	template<>
	struct converter<glm::mat4>
	{
		static bool from_string(const std::vector<psl::string8::view>& string, glm::mat4* t, size_t start, size_t end)
		{
			if(end - start != 16)
				return false;
			(*t)[0].x = std::stof(psl::string8_t(string[0 + start]));
			(*t)[0].y = std::stof(psl::string8_t(string[1 + start]));
			(*t)[0].z = std::stof(psl::string8_t(string[2 + start]));
			(*t)[0].w = std::stof(psl::string8_t(string[3 + start]));

			(*t)[1].x = std::stof(psl::string8_t(string[4 + start]));
			(*t)[1].y = std::stof(psl::string8_t(string[5 + start]));
			(*t)[1].z = std::stof(psl::string8_t(string[6 + start]));
			(*t)[1].w = std::stof(psl::string8_t(string[7 + start]));

			(*t)[2].x = std::stof(psl::string8_t(string[8 + start]));
			(*t)[2].y = std::stof(psl::string8_t(string[9 + start]));
			(*t)[2].z = std::stof(psl::string8_t(string[10 + start]));
			(*t)[2].w = std::stof(psl::string8_t(string[11 + start]));

			(*t)[3].x = std::stof(psl::string8_t(string[12 + start]));
			(*t)[3].y = std::stof(psl::string8_t(string[13 + start]));
			(*t)[3].z = std::stof(psl::string8_t(string[14 + start]));
			(*t)[3].w = std::stof(psl::string8_t(string[15 + start]));
			return true;
		};

		static psl::string8_t to_string(const glm::mat4& t)
		{
			return utility::to_string(t[0][0]) + "," + utility::to_string(t[0][1]) + "," + utility::to_string(t[0][2]) + "," + utility::to_string(t[0][3]) + "," +
				utility::to_string(t[1][0]) + "," + utility::to_string(t[1][1]) + "," + utility::to_string(t[1][2]) + "," + utility::to_string(t[1][3]) + "," +
				utility::to_string(t[2][0]) + "," + utility::to_string(t[2][1]) + "," + utility::to_string(t[2][2]) + "," + utility::to_string(t[2][3]) + "," +
				utility::to_string(t[3][0]) + "," + utility::to_string(t[3][1]) + "," + utility::to_string(t[3][2]) + "," + utility::to_string(t[3][3]);
		}

		static glm::mat4 from_string(psl::string8::view str)
		{
			glm::mat4 result;
			if(auto split = utility::string::split(str, ","); split.size() == 16 && from_string(split, &result, 0, split.size()))
				return result;
			return result;
		}
	};

	template<>
	struct converter<glm::quat>
	{
		static bool from_string(const std::vector<psl::string8::view>& string, glm::quat* t, size_t start, size_t end)
		{
			if(end - start != 4)
				return false;
			t->x = std::stof(psl::string8_t(string[0 + start]));
			t->y = std::stof(psl::string8_t(string[1 + start]));
			t->z = std::stof(psl::string8_t(string[2 + start]));
			t->w = std::stof(psl::string8_t(string[3 + start]));
			return true;
		};

		static psl::string8_t to_string(const glm::quat& t)
		{
			return std::to_string(t.x) + "," + std::to_string(t.y) + "," + std::to_string(t.z) + "," + std::to_string(t.w);
		}

		static glm::quat from_string(psl::string8::view str)
		{
			glm::quat result;
			if(auto split = utility::string::split(str, ","); split.size() == 4 && from_string(split, &result, 0, split.size()))
				return result;
			return result;
		}
	};
}
