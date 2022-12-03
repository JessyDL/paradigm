#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <numeric>

#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/assertions.hpp"
#include "psl/memory/raw_region.hpp"

#include "psl/ecs/entity.hpp"


namespace psl::ecs::details
{
template <typename T>
concept IsValidForContainer = std::is_trivial<T>::value;
/// @brief
/// @tparam Key
/// @tparam chunks_size
/// \note due to the dense data being stored on its own page, alignment shouldn't be a concern.
/// \warning this container is unsuitable for types that have non-trivial constructors, copy/move operations, and destructors.
class staged_sparse_memory_region_t
{
	using Key = psl::ecs::entity;
	static constexpr Key chunks_size {4096};
	static constexpr bool is_power_of_two {chunks_size && ((chunks_size & (chunks_size - 1)) == 0)};
	static constexpr Key mod_val {(is_power_of_two) ? chunks_size - 1 : chunks_size};

  public:
	using key_type			= Key;
	using size_type			= typename psl::array<key_type>::size_type;
	using difference_type	= typename psl::array<key_type>::difference_type;
	using value_type		= std::byte;
	using pointer			= value_type*;
	using const_pointer		= value_type* const;
	using reference			= value_type&;
	using const_reference	= const value_type&;
	using iterator_category = std::random_access_iterator_tag;
	using chunk_type		= psl::static_array<key_type, chunks_size>;

	staged_sparse_memory_region_t(size_t size) : m_DenseData((m_Reverse.capacity() + 1) * size), m_Size(size) {}

	staged_sparse_memory_region_t(const staged_sparse_memory_region_t& other)				 = default;
	staged_sparse_memory_region_t(staged_sparse_memory_region_t&& other) noexcept			 = default;
	staged_sparse_memory_region_t& operator=(const staged_sparse_memory_region_t& other)	 = default;
	staged_sparse_memory_region_t& operator=(staged_sparse_memory_region_t&& other) noexcept = default;

	/// --------------------------------------------------------------------------
	/// Query operations
	/// --------------------------------------------------------------------------

	constexpr auto capacity() const noexcept -> size_type { return std::size(m_Sparse) * chunks_size; }

	auto has(key_type index, size_t startStage = 0, size_t endStage = 1) const noexcept -> bool;

	auto operator[](key_type index) -> reference;

	template <typename T>
	constexpr inline auto operator[](key_type index) -> T&
	{
		psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
		return *(T*)(&this->operator[](index));
	}

	template <typename T>
	constexpr inline auto get(key_type index)->T&
	{
		return this->template operator[]<T>(index);
	}

	auto reserve(size_t capacity) -> void;
	auto resize(key_type size) -> void;

	template<typename T>
	constexpr inline auto to_view(size_t stage = 0) -> psl::array_view<T>
	{
		psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
		return psl::array_view<T> {std::next((T*)m_DenseData.data(), m_StageStart[stage]),
								   std::next((T*)m_DenseData.data(), m_StageStart[stage + 1])};
	}

  private:
	inline auto grow()->void;
	inline auto insert_impl(chunk_type& chunk, key_type offset, key_type user_index) -> void;
	constexpr inline auto
	has_impl(key_type chunk_index, key_type offset, size_t startStage = 0, size_t endStage = 1) const noexcept
	  -> bool;
	constexpr inline auto
	has_impl(chunk_type& chunk, key_type offset, size_t startStage = 0, size_t endStage = 1) const noexcept
	  -> bool;
	inline auto erase_impl(chunk_type& chunk, key_type offset, key_type user_index)->void;
	constexpr inline auto get_chunk_from_index(key_type index) const noexcept -> const chunk_type&;
	constexpr inline auto get_chunk_from_index(key_type index) noexcept -> chunk_type&;
	constexpr inline auto chunk_for(key_type& index) noexcept -> chunk_type&;

	constexpr inline auto get_chunk_from_user_index(key_type index) const noexcept -> const chunk_type&;
	constexpr inline auto get_chunk_from_user_index(key_type index) noexcept -> chunk_type&;
	constexpr inline auto chunk_info_for(key_type index, key_type& element_index, key_type& chunk_index) const noexcept
	  -> void;


	psl::array<key_type> m_Reverse {};
	::memory::raw_region m_DenseData;
	psl::array<std::optional<chunk_type>> m_Sparse {};

	psl::static_array<key_type, 4> m_StageStart {0, 0, 0, 0};
	psl::static_array<key_type, 3> m_StageSize {0, 0, 0};

	mutable chunk_type* m_CachedChunk;
	mutable key_type m_CachedChunkUserIndex = std::numeric_limits<key_type>::max();

	size_t m_Size;	  // size of underlying type
};
}	 // namespace psl::ecs::details
