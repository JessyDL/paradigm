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
	/// \brief Constraint for what types can safely be stored by `staged_sparse_memory_region_t`
	template <typename T>
	concept IsValidForStagedSparseMemoryRange = std::is_trivial<T>::value;

	/// \brief A specialized container type to store components in a type agnostic manner
	/// \note due to the dense data being stored on its own page, alignment shouldn't be a concern.
	/// \warning This container is unsuitable for types that have non-trivial constructors, copy/move operations, and destructors. Check your type against `IsValidForStagedSparseMemoryRange` to verify.
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

		/// \brief Defines the discrete stages the `staged_sparse_memory_region_t` can store
		enum class stage_t : uint8_t
		{
			SETTLED = 0,	// values that have persisted for one promotion, and aren't about to be removed
			ADDED	= 1,	// values that have just been added
			REMOVED = 2,	// values slated for removal with the next promote calld values
		};

		/// \brief Defines types of ranges you can safely interact with in a contiguous way
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

		staged_sparse_memory_region_t(const staged_sparse_memory_region_t& other)				 = default;
		staged_sparse_memory_region_t(staged_sparse_memory_region_t&& other) noexcept			 = default;
		staged_sparse_memory_region_t& operator=(const staged_sparse_memory_region_t& other)	 = default;
		staged_sparse_memory_region_t& operator=(staged_sparse_memory_region_t&& other) noexcept = default;

		/// \brief Safe (and preferred) method of creating a `staged_sparse_memory_region_t`
		/// \tparam T component type this should be storing (only used for size)
		/// \returns An instance of `staged_sparse_memory_region_t` set up to be used with the given type
		template <IsValidForStagedSparseMemoryRange T>
		static inline auto instantiate() -> staged_sparse_memory_region_t
		{
			return staged_sparse_memory_region_t(sizeof(T));
		}

		/// \brief Revert the object back to a safe default stage
		/// \note memory does not get deallocated.
		void clear() noexcept
		{
			m_Reverse.clear();
			m_Sparse.clear();
			m_CachedChunkUserIndex = std::numeric_limits<key_type>::max();
			m_StageStart		   = {0, 0, 0, 0};
			m_StageSize			   = {0, 0, 0};
		}

		/// \brief Checks if the given index exist in the requested range
		/// \param index Index to check
		/// \param stage Stage that will be searched in
		/// \return Boolean containing true if found
		auto has(key_type index, stage_range_t stage = stage_range_t::SETTLED) const noexcept -> bool;

		/// \brief Fetch or create the item at the given index
		/// \tparam T Used to check the expected component size
		/// \param index Where the item would be
		/// \returns The object as `T` at the given location
		template <IsValidForStagedSparseMemoryRange T>
		constexpr inline auto operator[](key_type index) -> T&
		{
			psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
			return *(T*)(&this->operator[](index));
		}

		/// \brief Fetch the item at the given index
		/// \tparam T Used to check the expected component size
		/// \param index Where the item would be
		/// \param stage The `stage_range_t` to search the index.
		/// \returns The object as `T` at the given location
		/// \note Throws if the item was not found in the given stage
		template <IsValidForStagedSparseMemoryRange T>
		constexpr inline auto get(key_type index, stage_range_t stage = stage_range_t::ALL) const -> T&
		{
			psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
			auto sub_index = index;
			auto& chunk	   = chunk_for(sub_index);

			if(!has(index, stage))
			{
				throw std::exception();
			}

			return *((T*)m_DenseData.data() + chunk[sub_index]);
		}

		/// \brief Sets the value at the given index
		/// \tparam T Type of the component (used for size and assignment)
		/// \param index Index of the item
		/// \param value The value to set at the given index
		/// \return Boolean value indicating if the index was successfuly set or not
		template <IsValidForStagedSparseMemoryRange T>
		constexpr inline auto set(key_type index, T&& value) -> bool
		{
			using type = std::remove_cvref_t<T>;
			psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
			auto sub_index = index;
			auto& chunk	   = chunk_for(sub_index);

			if(!has(index, stage_range_t::ALIVE))
			{
				return false;
			}

			*((type*)m_DenseData.data() + chunk[sub_index]) = std::forward<type>(value);
			return true;
		}

		/// \brief Get a view of the underlying data for the given `stage_range_t`
		/// \tparam T Component type to interpret the data as
		/// \param stage `stage_range_t` to limit what data is returned
		/// \return A view of the underlying data as the requested type
		template <IsValidForStagedSparseMemoryRange T>
		inline auto dense(stage_range_t stage = stage_range_t::SETTLED) const noexcept -> psl::array_view<T>
		{
			psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
			return psl::array_view<T> {std::next((T*)m_DenseData.data(), m_StageStart[stage_begin(stage)]),
									   std::next((T*)m_DenseData.data(), m_StageStart[stage_end(stage)])};
		}

		/// \brief Get a view of the indices for the given `stage_range_t`
		/// \param stage `stage_range_t` to limit what indices are returned
		/// \return A view of the indices
		inline auto indices(stage_range_t stage = stage_range_t::SETTLED) const noexcept -> psl::array_view<key_type>
		{
			return psl::array_view<key_type> {std::next(m_Reverse.data(), m_StageStart[stage_begin(stage)]),
											  std::next(m_Reverse.data(), m_StageStart[stage_end(stage)])};
		}

		/// \brief Get the data pointer for the given stage (where the data begins)
		/// \param stage `stage_t` to retrieve
		/// \return A pointer to the head of the dense data
		auto data(stage_t stage) noexcept -> pointer
		{
			return static_cast<pointer>(m_DenseData.data()) + (m_StageStart[to_underlying(stage)] * m_Size);
		}

		/// \brief Get the data pointer for the given stage (where the data begins)
		/// \param stage `stage_t` to retrieve
		/// \return A pointer to the head of the dense data
		auto cdata(stage_t stage) const noexcept -> const_pointer
		{
			return static_cast<const_pointer>(m_DenseData.data()) + (m_StageStart[to_underlying(stage)] * m_Size);
		}

		/// \brief Get a reference of the requested type at the index
		/// \tparam T Type we want to interpret the data as
		/// \param index Where to look
		/// \param stage Used to limit the stages we wish to look in
		/// \return Given memory address as a const ref
		/// \note When assertions are enabled, this function can assert
		template <IsValidForStagedSparseMemoryRange T>
		inline auto at(key_type index, stage_range_t stage = stage_range_t::SETTLED) const noexcept -> T const&
		{
			return *reinterpret_cast<T const*>(addressof(index, stage));
		}


		/// \brief Get a reference of the requested type at the index
		/// \tparam T Type we want to interpret the data as
		/// \param index Where to look
		/// \param stage Used to limit the stages we wish to look in
		/// \return Given memory address as a const ref
		/// \note When assertions are enabled, this function can assert
		template <IsValidForStagedSparseMemoryRange T>
		inline auto at(key_type index, stage_range_t stage = stage_range_t::SETTLED) noexcept -> T&
		{
			return *reinterpret_cast<T*>(addressof(index, stage));
		}

		/// \brief Get a pointer of the data at the index
		/// \param index Where to look
		/// \param stage Used to limit the stages we wish to look in
		/// \return memory address
		/// \note When assertions are enabled, this function can assert
		auto addressof(key_type index, stage_range_t stage = stage_range_t::SETTLED) const noexcept -> const_pointer;

		/// \brief Get a pointer of the data at the index
		/// \param index Where to look
		/// \param stage Used to limit the stages we wish to look in
		/// \return memory address
		/// \note When assertions are enabled, this function can assert
		auto addressof(key_type index, stage_range_t stage = stage_range_t::SETTLED) noexcept -> pointer;

		/// \brief Erases all values between the first/last indices
		/// \param first Begin of the range
		/// \param last End of the range
		/// \return Amount of elements erased
		auto erase(key_type first, key_type last) noexcept -> size_t;

		/// \brief Erases value at the given index
		/// \param index Index to erase
		/// \return Amount of elements erased (0 or 1)
		inline auto erase(key_type index) noexcept -> size_t { return erase(index, index + 1); }

		/// \brief Emplaces an item at the given index
		/// \tparam T The type of the element to emplace
		/// \param index Where to emplace it
		/// \param value The value to emplace
		/// \note When assertions are enabled this method can assert when the tparam is not of the expected size
		template <IsValidForStagedSparseMemoryRange T>
		auto emplace(key_type index, T&& value) -> void
		{
			this->template operator[]<T>(index) = std::forward<T>(value);
		}

		/// \brief Inserts an item at the given index
		/// \tparam T The type of the element to insert
		/// \param index Where to insert it
		/// \param value The value to insert
		/// \note When assertions are enabled this method can assert when the tparam is not of the expected size
		/// \note The value is assigned regardless if the index already contained a value.
		template <IsValidForStagedSparseMemoryRange T>
		auto insert(key_type index, const T& value) -> void
		{
			this->template operator[]<T>(index) = value;
		}

		/// \brief Inserts all items [begin, end) starting from the index
		/// \param index First index to where to start the insertions
		/// \param begin Iterator to the beginning of the values range
		/// \param end Iterator to the end of the values range
		/// \note When assertions are enabled this method can assert when the typename is not of the expected size
		/// \note The value is assigned regardless if the index already contained a value.
		template <typename ItF, typename ItL>
		auto insert(key_type index, ItF&& begin, ItL&& end) -> void
		{
			for(auto it = begin; it != end; it = std::next(it), ++index)
			{
				insert(index, *it);
			}
		}

		/// \brief Valueless insert. Either creates the memory for the given index, or does nothing if it already exists
		/// \param index Where to insert
		auto insert(key_type index) -> void;

		/// \brief Promotes all values to the next `stage_t`. The cycle is as follows: ADDED -> SETTLED -> REMOVED -> deleted.
		auto promote() noexcept -> void;

		/// \brief Remaps the current instance based on the mapping provided
		/// \tparam Fn
		/// \param mapping The mapping to use
		/// \param predicate Predicate that returns a boolean value if the index was found
		/// \details accesses all indices in the current container rejecting those who don't satisfy the predicate.
		/// Those who weren't rejected by the predicate are then looked up in the `mapping` value, and their return value
		/// is used to re-assign the index in the current container.
		/// f.e. if the mapping had a value of { 100, 200 } (index, value), and the predicate didn't reject the item on our end,
		/// then what was at 100 in this container would be remapped to the index 200.
		template <typename Fn>
		auto remap(const psl::sparse_array<key_type, key_type>& mapping, Fn&& predicate) -> void
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

		struct merge_result
		{
			bool success {false};
			size_t added {0};
			size_t total {0};
		};

		/// \brief Merges 2 staged_sparse_memory_region_t together (into the current one) replacing pre-existing entries.
		/// \param other the staged_sparse_memory_region_t to merge into this one
		/// \returns object that contains { .success /* bool */, .total /* total items processed, inserted + replaced */, .added /* amount that was inserted into the current container */ }
		auto merge(const staged_sparse_memory_region_t& other) noexcept -> merge_result;

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

		constexpr inline auto stage_begin(stage_range_t stage) const noexcept -> size_t
		{
			switch(stage)
			{
			case stage_range_t::SETTLED:
			case stage_range_t::ALIVE:
			case stage_range_t::ALL:
				return to_underlying(stage_t::SETTLED);
			case stage_range_t::ADDED:
			case stage_range_t::TERMINAL:
				return to_underlying(stage_t::ADDED);
			case stage_range_t::REMOVED:
				return to_underlying(stage_t::REMOVED);
			}
			psl::unreachable("stage was of unknown value");
		}

		constexpr inline auto stage_end(stage_range_t stage) const noexcept -> size_t
		{
			switch(stage)
			{
			case stage_range_t::SETTLED:
				return to_underlying(stage_t::ADDED);
			case stage_range_t::ALIVE:
			case stage_range_t::ADDED:
				return to_underlying(stage_t::REMOVED);
			case stage_range_t::TERMINAL:
			case stage_range_t::REMOVED:
			case stage_range_t::ALL:
				return to_underlying(stage_t::REMOVED) + 1;
			}
			psl::unreachable("stage was of unknown value");
		}

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
		constexpr inline auto chunk_for(key_type& index) noexcept -> chunk_type&
		{
			if(index >= capacity()) resize(index + 1);

			if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size)
			{
				if constexpr(is_power_of_two)
				{
					index = index & (mod_val);
				}
				else
				{
					index = index % mod_val;
				}
				return *m_CachedChunk;
			}
			key_type chunk_index;
			if constexpr(is_power_of_two)
			{
				const auto element_index = index & (mod_val);
				chunk_index				 = (index - element_index) / chunks_size;
				m_CachedChunkUserIndex	 = index - element_index;
				index					 = element_index;
			}
			else
			{
				const auto element_index = index % mod_val;
				chunk_index				 = (index - element_index) / chunks_size;
				m_CachedChunkUserIndex	 = index - element_index;
				index					 = element_index;
			}
			std::optional<chunk_type>& chunk = m_Sparse[chunk_index];
			if(!chunk)
			{
				chunk = chunk_type {};
				// chunk.resize(chunks_size);
				std::fill(std::begin(chunk.value()), std::end(chunk.value()), std::numeric_limits<key_type>::max());
			}
			m_CachedChunk = &chunk.value();
			return chunk.value();
		}

		constexpr inline auto chunk_for(key_type& index) const noexcept -> chunk_type&
		{
			psl_assert(index < capacity(), "expected index to be lower than the capacity");
			if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size)
			{
				if constexpr(is_power_of_two)
				{
					index = index & (mod_val);
				}
				else
				{
					index = index % mod_val;
				}
				return *m_CachedChunk;
			}
			key_type chunk_index;
			if constexpr(is_power_of_two)
			{
				const auto element_index = index & (mod_val);
				chunk_index				 = (index - element_index) / chunks_size;
				m_CachedChunkUserIndex	 = index - element_index;
				index					 = element_index;
			}
			else
			{
				const auto element_index = index % mod_val;
				chunk_index				 = (index - element_index) / chunks_size;
				m_CachedChunkUserIndex	 = index - element_index;
				index					 = element_index;
			}
			const std::optional<chunk_type>& chunk = m_Sparse.at(chunk_index);
			psl_assert(chunk.has_value(), "chunk was not created yet");
			m_CachedChunk = const_cast<chunk_type*>(&(chunk.value()));
			return *m_CachedChunk;
		}

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
