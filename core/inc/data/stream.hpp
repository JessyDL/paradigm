#pragma once
#include "psl/array.hpp"
#include "psl/math/matrix.hpp"
#include "psl/math/vec.hpp"
#include "psl/serialization/serializer.hpp"

namespace core
{
	/// \brief stream contains a type erased "stream" of memory with basic facilities to protect it from incorrect
	/// casts.
	///
	/// stream should be used in scenarios where something could contain various, but functionaly similar arrays of
	/// float. see core::data::geometry_t for an example of this. \see core::data::geometry_t
	class stream
	{
		friend class psl::serialization::accessor;

	  public:
		/// \brief the supported internal types of the stream.
		enum class type
		{
			single = 0,
			vec2   = 1,
			vec3   = 2,
			vec4   = 3,
			mat2   = 4,
			mat3x3 = 5,
			mat4x4 = 6
		};
		stream(type type = type::single) : m_Type(type) {}

		template <typename T>
		stream(psl::array<T> data) : m_Data(std::move(*(psl::array<float>*)(&data)))
		{
			using namespace psl;
			if constexpr(std::is_same_v<float, T>)
				m_Type = (type::single);
			else if constexpr(std::is_same_v<vec2, T>)
				m_Type = (type::vec2);
			else if constexpr(std::is_same_v<vec3, T>)
				m_Type = (type::vec3);
			else if constexpr(std::is_same_v<vec4, T>)
				m_Type = (type::vec4);
			else if constexpr(std::is_same_v<mat2x2, T>)
				m_Type = (type::mat2);
			else if constexpr(std::is_same_v<mat3x3, T>)
				m_Type = (type::mat3x3);
			else if constexpr(std::is_same_v<mat4x4, T>)
				m_Type = (type::mat4x4);
			else
				static_assert(utility::templates::always_false_v<T>);
		}

		/// \brief returns the pointer to the head of the memory stream.
		/// \returns the pointer to the head of the memory stream.
		void* data() { return m_Data.value.data(); }

		/// \brief returns the constant pointer to the head of the memory stream.
		/// \returns the constant pointer to the head of the memory stream.
		const void* cdata() const { return m_Data.value.data(); }

		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<psl::array<float>>> as_single()
		{
			if(m_Type == type::single)
			{
				return (m_Data.value);
			}
			return std::nullopt;
		}

		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<psl::array<psl::vec2>>> as_vec2()
		{
			if(m_Type == type::vec2)
			{
				return *(psl::array<psl::vec2>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<psl::array<psl::vec3>>> as_vec3()
		{
			if(m_Type == type::vec3)
			{
				return *(psl::array<psl::vec3>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<psl::array<psl::vec4>>> as_vec4()
		{
			if(m_Type == type::vec4)
			{
				return *(psl::array<psl::vec4>*)&m_Data.value;
			}
			return std::nullopt;
		}

		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<psl::array<psl::mat2x2>>> as_mat2()
		{
			if(m_Type == type::mat2)
			{
				return *(psl::array<psl::mat2x2>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<psl::array<psl::mat3x3>>> as_mat3()
		{
			if(m_Type == type::mat3x3)
			{
				return *(psl::array<psl::mat3x3>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<psl::array<psl::mat4x4>>> as_mat4()
		{
			if(m_Type == type::mat4x4)
			{
				return *(psl::array<psl::mat4x4>*)&m_Data.value;
			}
			return std::nullopt;
		}

		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const psl::array<float>>> as_single() const { return m_Data.value; }

		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const psl::array<psl::vec2>>> as_vec2() const
		{
			if(m_Type == type::vec2)
			{
				return *(psl::array<psl::vec2>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const psl::array<psl::vec3>>> as_vec3() const
		{
			if(m_Type == type::vec3)
			{
				return *(psl::array<psl::vec3>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const psl::array<psl::vec4>>> as_vec4() const
		{
			if(m_Type == type::vec4)
			{
				return *(psl::array<psl::vec4>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const psl::array<psl::mat2x2>>> as_mat2() const
		{
			if(m_Type == type::mat2)
			{
				return *(psl::array<psl::mat2x2>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const psl::array<psl::mat3x3>>> as_mat3() const
		{
			if(m_Type == type::mat3x3)
			{
				return *(psl::array<psl::mat3x3>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const psl::array<psl::mat4x4>>> as_mat4() const
		{
			if(m_Type == type::mat4x4)
			{
				return *(psl::array<psl::mat4x4>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the size of the stream (in unique element count).
		///
		/// depending on the contained type, one "element" is bigger/smaller. This method returns the size of one
		/// element. for example if the stream contains vec2 data, one element is equal to 2 floats, and so the size()
		/// would be 2 if there were 4 floats present. similarly if the stream contained vec3 data, one element would be
		/// equivalent to 3 floats, and so a size() of 4 would be equal to 12 floats. \returns the element count.
		size_t size() const { return m_Data.value.size() / elements(); }

		void resize(size_t count) noexcept { m_Data.value.resize(count * elements()); }

		void reserve(size_t count) noexcept { m_Data.value.reserve(count * elements()); }

		/// \brief returns the total size of the memory stream in bytes.
		/// \returns the total size of the memory stream in bytes.
		size_t bytesize() const { return m_Data.value.size() * sizeof(float); }

		size_t elements() const noexcept
		{
			switch(m_Type.value)
			{
			case type::single:
			{
				return 1;
			}
			break;
			case type::vec2:
			{
				return 2;
			}
			break;
			case type::vec3:
			{
				return 3;
			}
			break;
			case type::mat2:
			case type::vec4:
			{
				return 4;
			}
			break;
			case type::mat3x3:
			{
				return 9;
			}
			break;
			case type::mat4x4:
			{
				return 16;
			}
			break;
			}
			psl_assert(false, "unknown sized stream");
			return 0;
		}

		template <typename T>
		bool is() const noexcept
		{
			switch(m_Type.value)
			{
			case type::single:
				return std::is_same_v<T, float>;
				break;
			case type::vec2:
				return std::is_same_v<T, psl::vec2>;
				break;
			case type::vec3:
				return std::is_same_v<T, psl::vec3>;
				break;
			case type::mat2:
				return std::is_same_v<T, psl::mat2x2>;
				break;
			case type::vec4:
				return std::is_same_v<T, psl::vec4>;
				break;
			case type::mat3x3:
				return std::is_same_v<T, psl::mat3x3>;
				break;
			case type::mat4x4:
				return std::is_same_v<T, psl::mat4x4>;
				break;
			}
			return false;
		}

		template <typename F>
		bool transform(F&& transformation)
		{
			switch(m_Type.value)
			{
			case type::single:
				if constexpr(std::is_invocable_v<F, float&>)
				{
					auto& data = *reinterpret_cast<typename psl::array<float>*>(&m_Data.value);
					std::for_each(std::begin(data), std::end(data), transformation);
					return true;
				}
				break;
			case type::vec2:
				if constexpr(std::is_invocable_v<F, psl::vec2&>)
				{
					auto& data = *reinterpret_cast<typename psl::array<psl::vec2>*>(&m_Data.value);
					std::for_each(std::begin(data), std::end(data), transformation);
					return true;
				}
				break;
			case type::vec3:
				if constexpr(std::is_invocable_v<F, psl::vec3&>)
				{
					auto& data = *reinterpret_cast<typename psl::array<psl::vec3>*>(&m_Data.value);
					std::for_each(std::begin(data), std::end(data), transformation);
					return true;
				}
				break;
			case type::mat2:
				if constexpr(std::is_invocable_v<F, psl::mat2x2&>)
				{
					auto& data = *reinterpret_cast<typename psl::array<psl::mat2x2>*>(&m_Data.value);
					std::for_each(std::begin(data), std::end(data), transformation);
					return true;
				}
				break;
			case type::vec4:
				if constexpr(std::is_invocable_v<F, psl::vec4&>)
				{
					auto& data = *reinterpret_cast<typename psl::array<psl::vec4>*>(&m_Data.value);
					std::for_each(std::begin(data), std::end(data), transformation);
					return true;
				}
				break;
			case type::mat3x3:
				if constexpr(std::is_invocable_v<F, psl::mat3x3&>)
				{
					auto& data = *reinterpret_cast<typename psl::array<psl::mat3x3>*>(&m_Data.value);
					std::for_each(std::begin(data), std::end(data), transformation);
					return true;
				}
				break;
			case type::mat4x4:
				if constexpr(std::is_invocable_v<F, psl::mat4x4&>)
				{
					auto& data = *reinterpret_cast<typename psl::array<psl::mat4x4>*>(&m_Data.value);
					std::for_each(std::begin(data), std::end(data), transformation);
					return true;
				}
				break;
			}
			return false;
		}

	  private:
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_Type << m_Data;
		};


		static constexpr psl::string8::view serialization_name {"CORE_STREAM"};

		psl::serialization::property<"DATA", psl::array<float>> m_Data;
		psl::serialization::property<"TYPE", type> m_Type;
	};
}	 // namespace core
