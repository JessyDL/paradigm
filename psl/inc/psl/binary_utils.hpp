#pragma once
#include "psl/string_utils.hpp"
#include "psl/template_utils.hpp"
#include <memory>
#include <vector>

/// \brief this namespace contains various helper utilities to aid you.
namespace utility
{
	/// \brief contains various utilities in helping you to convert to, and from a byte stream.
	namespace binary
	{
		template <typename T>
		struct byte_size
		{
			static constexpr size_t size {0};
		};

		template <typename T>
		struct is_container : public std::false_type
		{};
		template <typename T, typename A>
		struct is_container<std::vector<T, A>> : public std::true_type
		{};
		template <typename T, typename... A>
		struct is_container<std::tuple<T, A...>> : public std::true_type
		{};

		template <>
		struct byte_size<float>
		{
			static constexpr size_t size {4};
		};
		template <>
		struct byte_size<double>
		{
			static constexpr size_t size {8};
		};
		template <>
		struct byte_size<int>
		{
			static constexpr size_t size {4};
		};
		template <>
		struct byte_size<psl::string8_t>
		{
			static constexpr size_t size {1};
		};

		template <typename T>
		struct get_contained_type
		{
			typedef T type;
		};

		template <typename T, typename A>
		struct get_contained_type<std::vector<T, A>>
		{
			typedef T type;
		};

		template <typename T, typename = void>
		struct has_contained_type : std::false_type
		{};


		template <typename T, typename A>
		struct has_contained_type<std::vector<T, A>> : std::true_type
		{};

		/// \brief writes in sets of 8 bytes to the target location
		/// \param[in, out] target the location to write to.
		/// \param[in] buffer the source of the data.
		/// \param[in] offset offset in the source data (offset is in bytes).
		/// \param[in] size multiplier to the size of to write (1 size is equivalent to 8 bytes).
		template <typename T>
		inline void write_8b(T* target, std::vector<uint8_t>& buffer, size_t offset, size_t size = 1)
		{
			memcpy(&buffer.data()[offset], target, sizeof(uint64_t) * size);
		}

		/// \brief writes in sets of 4 bytes to the target location
		/// \param[in, out] target the location to write to.
		/// \param[in] buffer the source of the data.
		/// \param[in] offset offset in the source data (offset is in bytes).
		/// \param[in] size multiplier to the size of to write (1 size is equivalent to 4 bytes).
		template <typename T>
		inline void write_4b(T* target, std::vector<uint8_t>& buffer, size_t offset, size_t size = 1)
		{
			memcpy(&buffer.data()[offset], target, sizeof(uint32_t) * size);
		}

		/// \brief writes in sets of 2 bytes to the target location
		/// \param[in, out] target the location to write to.
		/// \param[in] buffer the source of the data.
		/// \param[in] offset offset in the source data (offset is in bytes).
		/// \param[in] size multiplier to the size of to write (1 size is equivalent to 2 bytes).
		template <typename T>
		inline void write_2b(T* target, std::vector<uint8_t>& buffer, size_t offset, size_t size = 1)
		{
			memcpy(&buffer.data()[offset], target, sizeof(uint16_t) * size);
		}

		/// \brief writes in sets of 1 byte to the target location
		/// \param[in, out] target the location to write to.
		/// \param[in] buffer the source of the data.
		/// \param[in] offset offset in the source data (offset is in bytes).
		/// \param[in] size multiplier to the size of to write (1 size is equivalent to 1 byte).
		template <typename T>
		inline void write_1b(T* target, std::vector<uint8_t>& buffer, size_t offset, size_t size = 1)
		{
			memcpy(&buffer.data()[offset], target, sizeof(uint8_t) * size);
		}

		inline void write(void* target, std::vector<uint8_t>& buffer, size_t offset, size_t bytes, size_t size = 1)
		{
			memcpy(&buffer.data()[offset], target, sizeof(uint8_t) * size * bytes);
		}

		template <typename T>
		inline void read_8b(T* target, const std::vector<uint8_t>& buffer, size_t offset, size_t size = 1)
		{
			memcpy(target, &buffer.data()[offset], sizeof(uint64_t) * size);
		}

		template <typename T>
		inline void read_4b(T* target, const std::vector<uint8_t>& buffer, size_t offset, size_t size = 1)
		{
			memcpy(target, &buffer.data()[offset], sizeof(uint32_t) * size);
		}


		template <typename T>
		inline void read_2b(T* target, const std::vector<uint8_t>& buffer, size_t offset, size_t size = 1)
		{
			memcpy(target, &buffer.data()[offset], sizeof(uint16_t) * size);
		}

		template <typename T>
		inline void read_1b(T* target, const std::vector<uint8_t>& buffer, size_t offset, size_t size = 1)
		{
			memcpy(target, &buffer.data()[offset], sizeof(uint8_t) * size);
		}


		inline void read(void* target, const std::vector<uint8_t>& buffer, size_t offset, size_t bytes, size_t size = 1)
		{
			memcpy(target, &buffer.data()[offset], sizeof(uint8_t) * size * bytes);
		}

		template <typename T>
		inline void read_8b(T* target, const psl::string8::view& buffer, size_t offset, size_t size = 1)
		{
			memcpy(target, &buffer[offset], sizeof(uint64_t) * size);
		}

		template <typename T>
		inline void read_4b(T* target, const psl::string8::view& buffer, size_t offset, size_t size = 1)
		{
			memcpy(target, &buffer[offset], sizeof(uint32_t) * size);
		}


		template <typename T>
		inline void read_2b(T* target, const psl::string8::view& buffer, size_t offset, size_t size = 1)
		{
			memcpy(target, &buffer[offset], sizeof(uint16_t) * size);
		}

		template <typename T>
		inline void read_1b(T* target, const psl::string8::view& buffer, size_t offset, size_t size = 1)
		{
			memcpy(target, &buffer[offset], sizeof(uint8_t) * size);
		}

		inline void read(void* target, const psl::string8::view& buffer, size_t offset, size_t bytes, size_t size = 1)
		{
			memcpy(target, &buffer[offset], sizeof(uint8_t) * size * bytes);
		}
	}	 // namespace binary
}	 // namespace utility
