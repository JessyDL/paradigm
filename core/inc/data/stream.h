#pragma once

namespace core
{
	/// \brief stream contains a type erased "stream" of memory with basic facilities to protect it from incorrect casts.
	///
	/// stream should be used in scenarios where something could contain various, but functionaly similar arrays of float.
	/// see core::data::geometry for an example of this.
	/// \see core::data::geometry
	class stream
	{
		friend class serialization::accessor;
	public:
		/// \brief the supported internal types of the stream.
		enum class type
		{
			single = 0,
			vec2 = 1,
			vec3 = 2,
			vec4 = 3,
			mat2 = 4,
			mat3x3 = 5,
			mat4x4 =6
		};
		stream(type type = type::single) : m_Type(type)
		{

		}

		/// \brief returns the pointer to the head of the memory stream.
		/// \returns the pointer to the head of the memory stream.
		void* data()
		{
			return m_Data.value.data();
		}

		/// \brief returns the constant pointer to the head of the memory stream.
		/// \returns the constant pointer to the head of the memory stream.
		const void* cdata() const
		{
			return m_Data.value.data();
		}

		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<std::vector<float>>> as_single()
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
		std::optional<std::reference_wrapper<std::vector<psl::vec2>>> as_vec2()
		{
			if(m_Type == type::vec2)
			{
				return *(std::vector<psl::vec2>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<std::vector<psl::vec3>>> as_vec3()
		{
			if(m_Type == type::vec3)
			{
				return *(std::vector<psl::vec3>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<std::vector<psl::vec4>>> as_vec4()
		{
			if(m_Type == type::vec4)
			{
				return *(std::vector<psl::vec4>*)&m_Data.value;
			}
			return std::nullopt;
		}

		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<std::vector<psl::mat2x2>>> as_mat2()
		{
			if(m_Type == type::mat2)
			{
				return *(std::vector<psl::mat2x2>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<std::vector<psl::mat3x3>>> as_mat3()
		{
			if(m_Type == type::mat3x3)
			{
				return *(std::vector<psl::mat3x3>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<std::vector<psl::mat4x4>>> as_mat4()
		{
			if(m_Type == type::mat4x4)
			{
				return *(std::vector<psl::mat4x4>*)&m_Data.value;
			}
			return std::nullopt;
		}

		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const std::vector<float>>> as_single() const
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
		std::optional<std::reference_wrapper<const std::vector<psl::vec2>>> as_vec2()const
		{
			if(m_Type == type::vec2)
			{
				return *(std::vector<psl::vec2>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const std::vector<psl::vec3>>> as_vec3()const
		{
			if(m_Type == type::vec3)
			{
				return *(std::vector<psl::vec3>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const std::vector<psl::vec4>>> as_vec4()const
		{
			if(m_Type == type::vec4)
			{
				return *(std::vector<psl::vec4>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const std::vector<psl::mat2x2>>> as_mat2()const
		{
			if(m_Type == type::mat2)
			{
				return *(std::vector<psl::mat2x2>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const std::vector<psl::mat3x3>>> as_mat3()const
		{
			if(m_Type == type::mat3x3)
			{
				return *(std::vector<psl::mat3x3>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the type safe variant of the stream as a reference.
		///
		/// if the stream is of the correct type, it will return a valid vector, otherwise a std::nullopt
		/// \returns either a valid stream (on success) or a std::nullopt (on failure).
		std::optional<std::reference_wrapper<const std::vector<psl::mat4x4>>> as_mat4()const
		{
			if(m_Type == type::mat4x4)
			{
				return *(std::vector<psl::mat4x4>*)&m_Data.value;
			}
			return std::nullopt;
		}
		/// \brief returns the size of the stream (in unique element count).
		///
		/// depending on the contained type, one "element" is bigger/smaller. This method returns the size of one element.
		/// for example if the stream contains vec2 data, one element is equal to 2 floats, and so the size() would be 2 if there were 4 floats present.
		/// similarly if the stream contained vec3 data, one element would be equivalent to 3 floats, and so a size() of 4 would be equal to 12 floats.
		/// \returns the element count.
		size_t size() const
		{
			switch(m_Type.value)
			{
				case type::single:
				{
					return m_Data.value.size();

				}break;
				case type::vec2:
				{
					return m_Data.value.size() / 2;

				}break;
				case type::vec3:
				{
					return m_Data.value.size() / 3;
				}break; 
				case type::mat2:
				case type::vec4:
				{
					return m_Data.value.size() / 4;
				}break;
				case type::mat3x3:
				{
					return m_Data.value.size() / 9;
				}break;
				case type::mat4x4:
				{
					return m_Data.value.size() / 16;
				}break;
			}
			return 0u;
		}

		/// \brief returns the total size of the memory stream in bytes.
		/// \returns the total size of the memory stream in bytes.
		size_t bytesize() const
		{
			return m_Data.value.size() * sizeof(float);
		}
	private:
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_Type << m_Data;
		};


		static constexpr const char serialization_name[12]{"CORE_STREAM"};

		serialization::property<std::vector<float>, const_str("DATA", 4)> m_Data;
		serialization::property<type, const_str("TYPE", 4)> m_Type;
	};
}
