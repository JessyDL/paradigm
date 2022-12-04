#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <numeric>
#include <optional>

#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/assertions.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/sparse_array.hpp"

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

		enum class stage_t : uint8_t
		{
			SETTLED = 0,	// values that have persisted for one promotion, and aren't about to be removed
			ADDED	= 1,	// values that have just been added
			REMOVED = 2,	// values slated for removal with the next promote calld values
		};

		enum class stage_range_t : uint8_t
		{
			SETTLED,	 // values that have persisted for one promotion, and aren't about to be removed
			ADDED,		 // values that have just been added
			REMOVED,	 // values slated for removal with the next promote call
			ALIVE,		 // all non-removed values
			ALL,		 // all values
			TERMINAL,	 // values that have just been added, and to-be removed values
		};

		/// \note prefer using the `instantiate<T>()` function
		/// \param size element size in the container
		staged_sparse_memory_region_t(size_t size);

		staged_sparse_memory_region_t(const staged_sparse_memory_region_t& other)		 = default;
		staged_sparse_memory_region_t(staged_sparse_memory_region_t&& other) noexcept	 = default;
		staged_sparse_memory_region_t& operator=(const staged_sparse_memory_region_t& other) = default;
		staged_sparse_memory_region_t& operator=(staged_sparse_memory_region_t&& other) noexcept = default;

		template <typename T>
		static inline auto instantiate() -> staged_sparse_memory_region_t
		{
			return staged_sparse_memory_region_t(sizeof(T));
		}

		void clear() noexcept
		{
			m_Reverse.clear();
			m_Sparse.clear();
			m_CachedChunkUserIndex = std::numeric_limits<key_type>::max();
			m_StageStart		   = {0, 0, 0, 0};
			m_StageSize			   = {0, 0, 0};
		}


		auto has(key_type index, stage_range_t stage = stage_range_t::SETTLED) const noexcept -> bool;

		template <typename T>
		constexpr inline auto operator[](key_type index) -> T&
		{
			psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
			return *(T*)(&this->operator[](index));
		}

		template <typename T>
		constexpr inline auto get(key_type index) -> T&
		{
			return this->template operator[]<T>(index);
		}

		template <typename T>
		constexpr inline auto set(key_type index, T&& value) -> bool
		{
			using type = std::remove_cvref_t<T>;
			psl_assert(sizeof(type) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
			auto sub_index = index;
			auto& chunk	   = chunk_for(sub_index);

			if(!has(index, stage_range_t::ALIVE))
			{
				return false;
			}

			*((type*)m_DenseData.data() + chunk[sub_index]) = std::forward<type>(value);
			return true;
		}

		template <typename T>
		inline auto dense(stage_range_t stage = stage_range_t::SETTLED) const noexcept -> psl::array_view<T>
		{
			psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
			return psl::array_view<T> {
			  std::next((T*)m_DenseData.data(), m_StageStart[stage_begin(stage)]),
			  std::next((T*)m_DenseData.data(), m_StageStart[stage_end(stage)] - m_StageStart[stage_begin(stage)])};
		}

		inline auto indices(stage_range_t stage = stage_range_t::SETTLED) const noexcept -> psl::array_view<key_type>
		{
			return psl::array_view<key_type> {
			  std::next(m_Reverse.data(), m_StageStart[stage_begin(stage)]),
			  std::next(m_Reverse.data(), m_StageStart[stage_end(stage)] - m_StageStart[stage_begin(stage)])};
		}

		auto data(stage_t stage) noexcept -> pointer
		{
			return static_cast<pointer>(m_DenseData.data()) + (m_StageStart[to_underlying(stage)] * m_Size);
		}

		auto cdata(stage_t stage) const noexcept -> const_pointer
		{
			return static_cast<const_pointer>(m_DenseData.data()) + (m_StageStart[to_underlying(stage)] * m_Size);
		}

		inline auto at(key_type index, stage_range_t stage = stage_range_t::SETTLED) const noexcept -> const_reference
		{
			return *addressof(index, stage);
		}

		inline auto at(key_type index, stage_range_t stage = stage_range_t::SETTLED) noexcept -> reference
		{
			return *addressof(index, stage);
		}

		template<typename T>
		inline auto at(key_type index, stage_range_t stage = stage_range_t::SETTLED) const noexcept -> T const&
		{
			return *reinterpret_cast<T const*>(addressof(index, stage));
		}

		template <typename T>
		inline auto at(key_type index, stage_range_t stage = stage_range_t::SETTLED) noexcept -> T&
		{
			return *reinterpret_cast<T*>(addressof(index, stage));
		}

		auto addressof(key_type index, stage_range_t stage = stage_range_t::SETTLED) const noexcept -> const_pointer;
		auto addressof(key_type index, stage_range_t stage = stage_range_t::SETTLED) noexcept -> pointer;

		auto erase(key_type first, key_type last) noexcept -> void;

		inline void erase(key_type index) noexcept { return erase(index, index + 1); }

		template <typename T>
		void emplace(key_type index, T&& value)
		{
			this->template operator[]<T>(index) = std::forward<T>(value);
		}

		template <typename T>
		void insert(key_type index, const T& value)
		{
			this->template operator[]<T>(index) = value;
		}
		void insert(key_type index)
		{
			auto sub_index = index;
			auto& chunk	   = chunk_for(sub_index);

			insert_impl(chunk, sub_index, index);
		}

		auto promote() noexcept -> void;

		template <typename Fn>
		auto remap(const psl::sparse_array<key_type>& mapping, Fn&& predicate) -> void
		{
			psl_assert(m_Reverse.size() >= mapping.size(), "expected {} >= {}", m_Reverse.size(), mapping.size());
			m_Sparse.clear();
			m_CachedChunkUserIndex = std::numeric_limits<key_type>::max();
			for(key_type i = 0; i < static_cast<key_type>(m_Reverse.size()); ++i)
			{
				if(predicate(m_Reverse[i]))
				{
					psl_assert(mapping.has(m_Reverse[i]), "mapping didnt have the ID {}", m_Reverse[i]);
					auto new_index = mapping.at(m_Reverse[i]);
					auto offset	   = new_index;
					auto& chunk	   = chunk_for(offset);
					chunk[offset]  = i;
					m_Reverse[i]   = new_index;
				}
				else
				{
					auto offset	  = m_Reverse[i];
					auto& chunk	  = chunk_for(offset);
					chunk[offset] = i;
				}
			}
		}

		auto merge(const staged_sparse_memory_region_t& other) noexcept -> void;

	  private:
		constexpr auto size(stage_range_t stage = stage_range_t::SETTLED) const noexcept -> size_type
		{
			return m_StageStart[stage_end(stage)] - m_StageStart[stage_begin(stage)];
		}
		constexpr auto empty() const noexcept -> bool { return std::empty(m_Reverse); };
		auto resize(key_type size) -> void;
		auto reserve(size_t capacity) -> void;
		constexpr auto capacity() const noexcept -> size_type { return std::size(m_Sparse) * chunks_size; }
		template <typename T>
		constexpr inline auto to_underlying(T value) const noexcept -> std::underlying_type_t<T>
		{
			return static_cast<std::underlying_type_t<T>>(value);
		}

		constexpr inline auto stage_begin(stage_range_t stage) const noexcept -> size_t;
		constexpr inline auto stage_end(stage_range_t stage) const noexcept -> size_t;

		auto operator[](key_type index) -> reference;

		inline auto grow() -> void;
		inline auto insert_impl(chunk_type& chunk, key_type offset, key_type user_index) -> void;
		constexpr inline auto has_impl(key_type chunk_index,
									   key_type offset,
									   stage_range_t stage = stage_range_t::SETTLED) const noexcept -> bool;
		constexpr inline auto has_impl(chunk_type& chunk,
									   key_type offset,
									   stage_range_t stage = stage_range_t::SETTLED) const noexcept -> bool;
		inline auto erase_impl(chunk_type& chunk, key_type offset, key_type user_index) -> void;
		constexpr inline auto get_chunk_from_index(key_type index) const noexcept -> const chunk_type&;
		constexpr inline auto get_chunk_from_index(key_type index) noexcept -> chunk_type&;
		constexpr inline auto chunk_for(key_type& index) noexcept -> chunk_type&;

		constexpr inline auto get_chunk_from_user_index(key_type index) const noexcept -> const chunk_type&;
		constexpr inline auto get_chunk_from_user_index(key_type index) noexcept -> chunk_type&;
		constexpr inline auto
		chunk_info_for(key_type index, key_type& element_index, key_type& chunk_index) const noexcept -> void;


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
