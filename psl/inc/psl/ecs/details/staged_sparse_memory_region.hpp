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

#include "psl/ecs/component_traits.hpp"
#include "psl/ecs/details/stage_range_t.hpp"
#include "psl/ecs/entity.hpp"


namespace psl::ecs::details {
/// \brief Constraint for what types can safely be stored by `staged_sparse_memory_region_t`
template <typename T>
concept IsValidForStagedSparseMemoryRange = IsComponentTrivialType<T>;

/// \brief A specialized container type to store components in a type agnostic manner
/// \note due to the dense data being stored on its own page, alignment shouldn't be a concern.
/// \warning This container is unsuitable for types that have non-trivial constructors, copy/move operations, and destructors. Check your type against `IsValidForStagedSparseMemoryRange` to verify.
class staged_sparse_memory_region_t {
	using Key = psl::ecs::entity_t::size_type;
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

	/// \note prefer using the `instantiate<T>()` function
	/// \param size element size in the container
	staged_sparse_memory_region_t(size_t size) : m_DenseData(0), m_Size(size), m_ScratchMemory(malloc(size)) {
		psl_assert(m_ScratchMemory != nullptr, "Couldn't allocate scratch memory");
		grow();
	}

	staged_sparse_memory_region_t(const staged_sparse_memory_region_t& other)				 = delete;
	staged_sparse_memory_region_t(staged_sparse_memory_region_t&& other) noexcept			 = delete;
	staged_sparse_memory_region_t& operator=(const staged_sparse_memory_region_t& other)	 = delete;
	staged_sparse_memory_region_t& operator=(staged_sparse_memory_region_t&& other) noexcept = delete;
	~staged_sparse_memory_region_t() {
		if(m_ScratchMemory) {
			free(m_ScratchMemory);
		}
		m_ScratchMemory = nullptr;
	}
	/// \brief Safe (and preferred) method of creating a `staged_sparse_memory_region_t`
	/// \tparam T component type this should be storing (only used for size)
	/// \returns An instance of `staged_sparse_memory_region_t` set up to be used with the given type
	template <IsValidForStagedSparseMemoryRange T>
	static FORCEINLINE auto instantiate() -> staged_sparse_memory_region_t {
		return staged_sparse_memory_region_t(sizeof(T));
	}

	/// \brief Revert the object back to a safe default stage
	/// \note memory does not get deallocated.
	FORCEINLINE void clear() noexcept {
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
	FORCEINLINE auto has(key_type index, stage_range_t stage = stage_range_t::ALIVE) const noexcept -> bool {
		if(index < capacity()) {
			key_type chunk_index;
			if constexpr(is_power_of_two) {
				const auto element_index = index & (mod_val);
				chunk_index				 = (index - element_index) / chunks_size;
				index					 = element_index;
			} else {
				chunk_index = (index - (index % mod_val)) / chunks_size;
				index		= index % mod_val;
			}
			return has_impl(chunk_index, index, stage);
		}
		return false;
	}

	/// \brief Fetch or create the item at the given index
	/// \tparam T Used to check the expected component size
	/// \param index Where the item would be
	/// \returns The object as `T` at the given location
	template <IsValidForStagedSparseMemoryRange T>
	constexpr FORCEINLINE auto operator[](key_type index) -> T& {
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
	constexpr FORCEINLINE auto get(key_type index, stage_range_t stage = stage_range_t::ALL) const -> T& {
		psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		if(!has(index, stage)) {
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
	constexpr FORCEINLINE auto set(key_type index, const T& value) -> bool {
		psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		if(!has(index, stage_range_t::ALIVE)) {
			return false;
		}

		*((T*)m_DenseData.data() + chunk[sub_index]) = value;
		return true;
	}

	/// \brief Get a view of the underlying data for the given `stage_range_t`
	/// \tparam T Component type to interpret the data as
	/// \param stage `stage_range_t` to limit what data is returned
	/// \return A view of the underlying data as the requested type
	template <IsValidForStagedSparseMemoryRange T>
	FORCEINLINE auto dense(stage_range_t stage = stage_range_t::ALIVE) const noexcept -> psl::array_view<T> {
		psl_assert(sizeof(T) == m_Size, "expected {} but instead got {}", m_Size, sizeof(T));
		return psl::array_view<T> {std::next((T*)m_DenseData.data(), m_StageStart[stage_begin(stage)]),
								   std::next((T*)m_DenseData.data(), m_StageStart[stage_end(stage)])};
	}

	/// \brief Get a view of the indices for the given `stage_range_t`
	/// \param stage `stage_range_t` to limit what indices are returned
	/// \return A view of the indices
	FORCEINLINE auto indices(stage_range_t stage = stage_range_t::ALIVE) const noexcept -> psl::array_view<key_type> {
		return psl::array_view<key_type> {std::next(m_Reverse.data(), m_StageStart[stage_begin(stage)]),
										  std::next(m_Reverse.data(), m_StageStart[stage_end(stage)])};
	}

	/// \brief Get the data pointer for the given stage (where the data begins)
	/// \param stage `stage_t` to retrieve
	/// \return A pointer to the head of the dense data
	FORCEINLINE auto data(stage_t stage = stage_t::SETTLED) noexcept -> pointer {
		return static_cast<pointer>(m_DenseData.data()) + (m_StageStart[to_underlying(stage)] * m_Size);
	}

	/// \brief Get the data pointer for the given stage (where the data begins)
	/// \param stage `stage_t` to retrieve
	/// \return A pointer to the head of the dense data
	FORCEINLINE auto cdata(stage_t stage = stage_t::SETTLED) const noexcept -> const_pointer {
		return static_cast<const_pointer>(m_DenseData.data()) + (m_StageStart[to_underlying(stage)] * m_Size);
	}

	FORCEINLINE auto data(stage_t stage = stage_t::SETTLED) const noexcept -> const_pointer { return cdata(stage); }

	/// \brief Get a reference of the requested type at the index
	/// \tparam T Type we want to interpret the data as
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return Given memory address as a const ref
	/// \note When assertions are enabled, this function can assert
	template <IsValidForStagedSparseMemoryRange T>
	FORCEINLINE auto at(key_type index, stage_range_t stage = stage_range_t::ALIVE) const noexcept -> T const& {
		return *reinterpret_cast<T const*>(addressof(index, stage));
	}


	/// \brief Get a reference of the requested type at the index
	/// \tparam T Type we want to interpret the data as
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return Given memory address as a const ref
	/// \note When assertions are enabled, this function can assert
	template <IsValidForStagedSparseMemoryRange T>
	FORCEINLINE auto at(key_type index, stage_range_t stage = stage_range_t::ALIVE) noexcept -> T& {
		return *reinterpret_cast<T*>(addressof(index, stage));
	}

	/// \brief Get a pointer of the data at the index
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return memory address
	/// \note When assertions are enabled, this function can assert
	FORCEINLINE auto addressof(key_type index, stage_range_t stage = stage_range_t::ALIVE) const noexcept
	  -> const_pointer {
		key_type sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		psl_assert(has(index, stage),
				   "missing index {} within [{}, {}] in sparse array",
				   index,
				   static_cast<std::underlying_type_t<stage_range_t>>(stage));
		return (static_cast<const_pointer>(m_DenseData.data()) +
				(get_chunk_from_index(chunk_index)[sparse_index] * m_Size));
	}

	/// \brief Get a pointer of the data at the index
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return memory address
	/// \note When assertions are enabled, this function can assert
	FORCEINLINE auto addressof(key_type index, stage_range_t stage = stage_range_t::ALIVE) noexcept -> pointer {
		key_type sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		psl_assert(has(index, stage),
				   "missing index {} within [{}, {}] in sparse array",
				   index,
				   static_cast<std::underlying_type_t<stage_range_t>>(stage));
		return (static_cast<pointer>(m_DenseData.data()) + (get_chunk_from_index(chunk_index)[sparse_index] * m_Size));
	}

	/// \brief Get a pointer of the data at the index, or nullptr when not found
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return memory address or nullptr
	FORCEINLINE auto addressof_if(key_type index, stage_range_t stage = stage_range_t::ALIVE) const noexcept
	  -> const_pointer {
		key_type sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		if(has(index, stage)) {
			return (static_cast<const_pointer>(m_DenseData.data()) +
					(get_chunk_from_index(chunk_index)[sparse_index] * m_Size));
		}
		return nullptr;
	}

	/// \brief Get a pointer of the data at the index, or nullptr when not found
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return memory address or nullptr
	FORCEINLINE auto addressof_if(key_type index, stage_range_t stage = stage_range_t::ALIVE) noexcept -> pointer {
		key_type sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		if(has(index, stage)) {
			return (static_cast<pointer>(m_DenseData.data()) +
					(get_chunk_from_index(chunk_index)[sparse_index] * m_Size));
		}
		return nullptr;
	}

	/// \brief Retrieves the internal index for the dense data for the given index
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return Index of the data relative to the data() (note that this does not take the type's size in account)
	FORCEINLINE auto dense_index_for(key_type index, stage_range_t stage = stage_range_t::ALIVE) const noexcept
	  -> key_type {
		key_type sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		psl_assert(has(index, stage),
				   "missing index {} within [{}, {}] in sparse array",
				   index,
				   static_cast<std::underlying_type_t<stage_range_t>>(stage));
		return static_cast<key_type>(get_chunk_from_index(chunk_index)[sparse_index]);
	}

	/// \brief Erases all values between the first/last indices
	/// \param first Begin of the range
	/// \param last End of the range
	/// \return Amount of elements erased
	FORCEINLINE auto erase(key_type first, key_type last) noexcept -> size_t {
		size_t count {0};
		for(auto i = first; i < last; ++i) {
			auto sub_index = i;
			auto& chunk	   = chunk_for(sub_index);

			if(has_impl(chunk, sub_index, stage_range_t::ALIVE)) {
				erase_impl(chunk, sub_index, i);
				++count;
			}
		}
		return count;
	}

	/// \brief Erases value at the given index
	/// \param index Index to erase
	/// \return Amount of elements erased (0 or 1)
	FORCEINLINE auto erase(key_type index) noexcept -> size_t { return erase(index, index + 1); }

	FORCEINLINE auto erase(key_type* begin, key_type* end) noexcept -> size_t {
		size_t count {0};
		for(auto it = begin; it != end; ++it) {
			auto sub_index = *it;
			auto& chunk	   = chunk_for(sub_index);

			if(has_impl(chunk, sub_index, stage_range_t::ALIVE)) {
				erase_impl(chunk, sub_index, *it);
				++count;
			}
		}
		return count;
	}


	/// \brief Emplaces an item at the given index
	/// \tparam T The type of the element to emplace
	/// \param index Where to emplace it
	/// \param value The value to emplace
	/// \note When assertions are enabled this method can assert when the tparam is not of the expected size
	template <IsValidForStagedSparseMemoryRange T>
	FORCEINLINE auto emplace(key_type index, T&& value) -> void {
		this->template operator[]<T>(index) = std::forward<T>(value);
	}

	/// \brief Inserts an item at the given index
	/// \tparam T The type of the element to insert
	/// \param index Where to insert it
	/// \param value The value to insert
	/// \note When assertions are enabled this method can assert when the tparam is not of the expected size
	/// \note The value is assigned regardless if the index already contained a value.
	template <IsValidForStagedSparseMemoryRange T>
	FORCEINLINE auto insert(key_type index, const T& value) -> void {
		this->template operator[]<T>(index) = value;
	}

	/// \brief Inserts all items [begin, end) starting from the index
	/// \param index First index to where to start the insertions
	/// \param begin Iterator to the beginning of the values range
	/// \param end Iterator to the end of the values range
	/// \note When assertions are enabled this method can assert when the typename is not of the expected size
	/// \note The value is assigned regardless if the index already contained a value.
	template <typename ItF, typename ItL>
	FORCEINLINE auto insert(key_type index, ItF&& begin, ItL&& end) -> void {
		using T = typename std::iterator_traits<ItF>::value_type;

		key_type element_index {};
		key_type chunk_index {};

		auto get_chunk = [this](auto index, auto& element_index, auto& chunk_index) mutable -> chunk_type* {
			if(index >= capacity())
				resize(index + 1);
			chunk_info_for(index, element_index, chunk_index);
			auto& chunk_opt = m_Sparse[chunk_index];
			if(!chunk_opt) {
				chunk_opt = chunk_type {};
				std::fill(
				  std::begin(chunk_opt.value()), std::end(chunk_opt.value()), std::numeric_limits<key_type>::max());
			}
			return &(chunk_opt.value());
		};

		chunk_type* chunk = get_chunk(index, element_index, chunk_index);
		auto it			  = begin;
		while(it != end) {
			if(index % chunks_size == 0) {
				chunk = get_chunk(index, element_index, chunk_index);
			}
			insert_impl(*chunk, element_index, index);

			psl_assert(m_DenseData.size() < (*chunk)[element_index] * sizeof(T), "");
			*((T*)m_DenseData.data() + (*chunk)[element_index]) = *it;
			++index;
			it = std::next(it);
			++element_index;
		}
	}

	/// \brief Valueless insert. Either creates the memory for the given index, or does nothing if it already exists
	/// \param index Where to insert
	FORCEINLINE auto insert(key_type index) -> void {
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		insert_impl(chunk, sub_index, index);
	}

	FORCEINLINE auto get_or_insert(key_type index) -> pointer {
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		insert_impl(chunk, sub_index, index);
		return ((std::byte*)m_DenseData.data() + (chunk[sub_index] * m_Size));
	}

	/// \brief Promotes all values to the next `stage_t`. The cycle is as follows: ADDED -> SETTLED -> REMOVED -> deleted.
	FORCEINLINE auto promote() noexcept -> void {
		for(auto i = m_StageStart[to_underlying(stage_t::REMOVED)]; i < m_Reverse.size(); ++i) {
			auto offset	  = m_Reverse[i];
			auto& chunk	  = chunk_for(offset);
			chunk[offset] = std::numeric_limits<key_type>::max();
		}
		m_Reverse.erase(std::next(std::begin(m_Reverse), m_StageStart[to_underlying(stage_t::REMOVED)]),
						std::end(m_Reverse));

		m_StageSize[to_underlying(stage_t::SETTLED)] += m_StageSize[to_underlying(stage_t::ADDED)];
		m_StageStart[to_underlying(stage_t::ADDED)]	 = m_StageSize[to_underlying(stage_t::SETTLED)];
		m_StageStart[3]								 = m_StageStart[to_underlying(stage_t::REMOVED)];
		m_StageSize[to_underlying(stage_t::ADDED)]	 = 0;
		m_StageSize[to_underlying(stage_t::REMOVED)] = 0;
	}

	/// \brief Remaps the current instance based on the mapping provided
	/// \tparam Fn
	/// \param mapping The mapping to use
	/// \param predicate Predicate that returns a boolean value if the index was found
	/// \details accesses all indices in the current container rejecting those who don't satisfy the predicate.
	/// Those who weren't rejected by the predicate are then looked up in the `mapping` value, and their return
	/// value is used to re-assign the index in the current container. f.e. if the mapping had a value of { 100, 200
	/// } (index, value), and the predicate didn't reject the item on our end, then what was at 100 in this
	/// container would be remapped to the index 200.
	template <typename Fn>
	FORCEINLINE auto remap(const psl::sparse_array<key_type>& mapping, Fn&& predicate) -> void {
		psl_assert(m_Reverse.size() >= mapping.size(), "expected {} >= {}", m_Reverse.size(), mapping.size());
		m_Sparse.clear();
		m_CachedChunkUserIndex = std::numeric_limits<key_type>::max();
		for(key_type i = 0; i < static_cast<key_type>(m_Reverse.size()); ++i) {
			if(predicate(m_Reverse[i])) {
				psl_assert(mapping.has(m_Reverse[i]), "mapping didnt have the ID {}", m_Reverse[i]);
				auto new_index = mapping.at(m_Reverse[i]);
				auto offset	   = new_index;
				auto& chunk	   = chunk_for(offset);
				chunk[offset]  = i;
				m_Reverse[i]   = new_index;
			} else {
				auto offset	  = m_Reverse[i];
				auto& chunk	  = chunk_for(offset);
				chunk[offset] = i;
			}
		}
	}

	struct merge_result {
		bool success {false};
		size_t added {0};
		size_t total {0};
	};

	/// \brief Merges 2 staged_sparse_memory_region_t together (into the current one) replacing pre-existing entries.
	/// \param other the staged_sparse_memory_region_t to merge into this one
	/// \returns object that contains { .success /* bool */, .total /* total items processed, inserted + replaced */, .added /* amount that was inserted into the current container */ }
	FORCEINLINE auto merge(const staged_sparse_memory_region_t& other) noexcept -> merge_result {
		if(m_Size != other.m_Size || this == &other) {
			psl_assert(m_Size == other.m_Size,
					   "staged_sparse_memory_region_t should have the same component size, but instead got {} and {}",
					   m_Size,
					   other.m_Size);
			psl_assert(this != &other, "self merging not allowed");
			return merge_result {false};
		}

		size_t inserted {0};
		for(key_type i = 0; i < static_cast<key_type>(other.m_Reverse.size()); ++i) {
			auto sub_index = other.m_Reverse[i];
			auto& chunk	   = chunk_for(sub_index);
			if(!has_impl(chunk, sub_index, stage_range_t::ALL)) {
				insert_impl(chunk, sub_index, other.m_Reverse[i]);
				++inserted;
			}

			memcpy((std::byte*)m_DenseData.data() + (chunk[sub_index] * m_Size),
				   (std::byte*)other.m_DenseData.data() + (i * m_Size),
				   m_Size);
		}
		for(key_type i = other.m_StageStart[2]; i < other.m_StageStart[3]; ++i) {
			erase(other.m_Reverse[i]);
		}
		return merge_result {
		  .success = true, .added = inserted, .total = static_cast<key_type>(other.m_Reverse.size())};
	}

  private:
	FORCEINLINE constexpr auto size(stage_range_t stage) const noexcept -> size_type {
		return m_StageStart[stage_end(stage)] - m_StageStart[stage_begin(stage)];
	}

	FORCEINLINE constexpr auto empty() const noexcept -> bool { return std::empty(m_Reverse); };

	FORCEINLINE auto resize(key_type size) -> void {
		key_type chunk_index;
		if constexpr(is_power_of_two) {
			chunk_index = (size - (size & mod_val)) / chunks_size;
		} else {
			chunk_index = (size - (size % mod_val)) / chunks_size;
		}
		if(m_Sparse.size() <= chunk_index)
			m_Sparse.resize(chunk_index + 1);
	}

	FORCEINLINE auto reserve(size_t capacity) -> void {
		if(capacity <= m_Reverse.capacity())
			return;

		m_Reverse.reserve(capacity);
		grow();
	}

	FORCEINLINE constexpr auto capacity() const noexcept -> size_type { return std::size(m_Sparse) * chunks_size; }

	FORCEINLINE auto operator[](key_type index) -> reference {
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		if(!has(index)) {
			insert_impl(chunk, sub_index, index);
		}
		return *((std::byte*)m_DenseData.data() + (chunk[sub_index] * m_Size));
	}

	FORCEINLINE auto grow() -> void {
		auto capacity = static_cast<key_type>(m_Reverse.capacity() + 1);

		if(m_DenseData.size() < capacity * m_Size) {
			auto new_capacity = std::max<key_type>(capacity, static_cast<key_type>(m_DenseData.size() * 2 / m_Size));

			// copy the data to a new container of the given size, and then assign that container to the member
			// variable.
			{
				::memory::raw_region reg(new_capacity * m_Size);
				std::memcpy(reg.data(), m_DenseData.data(), m_DenseData.size());
				m_DenseData = std::move(reg);
			}
			m_Reverse.reserve((m_DenseData.size() / m_Size) - 1);
		}
		psl_assert((m_Reverse.capacity() + 1) * m_Size <= m_DenseData.size() &&
					 (m_Reverse.capacity() + 2) * m_Size >= m_DenseData.size(),
				   "capacity was not in line with density data");
	}

	FORCEINLINE auto insert_impl(chunk_type& chunk, key_type offset, key_type user_index) -> void {
		for(auto i = m_StageStart[2]; i < m_Reverse.size(); ++i) {
			auto old_offset = m_Reverse[i];
			auto& old_chunk = chunk_for(old_offset);
			old_chunk[old_offset] += 1;
		}

		std::memcpy((std::byte*)m_DenseData.data() + (m_StageStart[2] + 1) * m_Size,
					(std::byte*)m_DenseData.data() + (m_StageStart[2] * m_Size),
					(m_Reverse.size() - m_StageStart[2]) * m_Size);


		chunk[offset] = static_cast<key_type>(m_StageStart[2]);
		auto orig_cap = m_Reverse.capacity();
		m_Reverse.emplace(std::next(std::begin(m_Reverse), m_StageStart[2]), user_index);
		if(orig_cap != m_Reverse.capacity())
			grow();
		psl_assert((m_Reverse.capacity() + 1) * m_Size <= m_DenseData.size(),
				   "{} <= {}",
				   (m_Reverse.capacity() + 1) * m_Size,
				   m_DenseData.size());
		m_StageStart[2] += 1;
		m_StageStart[3] += 1;
		m_StageSize[1] += 1;
	}

	FORCEINLINE auto has_impl(key_type chunk_index, key_type offset, stage_range_t stage) const noexcept -> bool {
		if(m_Sparse.at(chunk_index)) {
			const auto& chunk = get_chunk_from_index(chunk_index);
			return chunk[offset] != std::numeric_limits<key_type>::max() &&
				   chunk[offset] >= m_StageStart[stage_begin(stage)] && chunk[offset] < m_StageStart[stage_end(stage)];
		}
		return false;
	}

	constexpr FORCEINLINE auto has_impl(chunk_type& chunk, key_type offset, stage_range_t stage) const noexcept
	  -> bool {
		return chunk[offset] != std::numeric_limits<key_type>::max() &&
			   chunk[offset] >= m_StageStart[stage_begin(stage)] && chunk[offset] < m_StageStart[stage_end(stage)];
	}

	FORCEINLINE auto erase_impl(chunk_type& chunk, key_type offset, key_type user_index) -> void {
		auto reverse_index = chunk[offset];

		// figure out which stage it belonged to
		auto what_stage = (reverse_index < m_StageStart[1]) ? 0 : (reverse_index < m_StageStart[2]) ? 1 : 2;
		if(what_stage == 2)
			return;

		// we swap it out
		for(auto i = what_stage; i < 2; ++i) {
			if(reverse_index != m_StageStart[i + 1] - 1) {
				std::iter_swap(std::next(std::begin(m_Reverse), reverse_index),
							   std::next(std::begin(m_Reverse), m_StageStart[i + 1] - 1));

				auto A = (std::byte*)m_DenseData.data() + (reverse_index * m_Size);
				auto B = (std::byte*)m_DenseData.data() + ((m_StageStart[i + 1] - 1) * m_Size);

				std::memcpy(m_ScratchMemory, A, m_Size);
				std::memcpy(A, B, m_Size);
				std::memcpy(B, m_ScratchMemory, m_Size);

				chunk[offset]		 = m_StageStart[i + 1] - 1;
				auto new_index		 = m_Reverse[reverse_index];
				auto& new_chunk		 = chunk_for(new_index);
				new_chunk[new_index] = reverse_index;
				reverse_index		 = m_StageStart[i + 1] - 1;
			}
			m_StageStart[i + 1] -= 1;
		}


		// decrement that stage's size
		m_StageSize[what_stage] -= 1;

		m_StageSize[2] += 1;
		psl_assert(chunk[offset] == reverse_index, "expected {} == {}", chunk[offset], reverse_index);
	}

	FORCEINLINE auto get_chunk_from_index(key_type index) const noexcept -> const chunk_type& {
		return m_Sparse.at(index).value();
	}

	FORCEINLINE auto get_chunk_from_index(key_type index) noexcept -> chunk_type& { return m_Sparse.at(index).value(); }

	constexpr FORCEINLINE auto chunk_for(key_type& index) noexcept -> chunk_type& {
		if(index >= capacity())
			resize(index + 1);

		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size) {
			if constexpr(is_power_of_two) {
				index = index & (mod_val);
			} else {
				index = index % mod_val;
			}
			return *m_CachedChunk;
		}
		key_type chunk_index;
		if constexpr(is_power_of_two) {
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
			m_CachedChunkUserIndex	 = index - element_index;
			index					 = element_index;
		} else {
			const auto element_index = index % mod_val;
			chunk_index				 = (index - element_index) / chunks_size;
			m_CachedChunkUserIndex	 = index - element_index;
			index					 = element_index;
		}
		std::optional<chunk_type>& chunk = m_Sparse[chunk_index];
		if(!chunk) {
			chunk = chunk_type {};
			// chunk.resize(chunks_size);
			std::fill(std::begin(chunk.value()), std::end(chunk.value()), std::numeric_limits<key_type>::max());
		}
		m_CachedChunk = &chunk.value();
		return chunk.value();
	}

	constexpr FORCEINLINE auto chunk_for(key_type& index) const noexcept -> chunk_type& {
		psl_assert(index < capacity(), "expected index to be lower than the capacity");
		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size) {
			if constexpr(is_power_of_two) {
				index = index & (mod_val);
			} else {
				index = index % mod_val;
			}
			return *m_CachedChunk;
		}
		key_type chunk_index;
		if constexpr(is_power_of_two) {
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
			m_CachedChunkUserIndex	 = index - element_index;
			index					 = element_index;
		} else {
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

	constexpr FORCEINLINE auto get_chunk_from_user_index(key_type index) const noexcept -> const chunk_type& {
		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size)
			return *m_CachedChunk;
		m_CachedChunkUserIndex = index;
		key_type chunk_index;
		if constexpr(is_power_of_two) {
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
		} else {
			chunk_index = (index - (index % mod_val)) / chunks_size;
		}
		m_CachedChunk = const_cast<chunk_type*>(&m_Sparse[chunk_index].value());
		return *m_CachedChunk;
	}

	constexpr FORCEINLINE auto get_chunk_from_user_index(key_type index) noexcept -> chunk_type& {
		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size)
			return *m_CachedChunk;
		m_CachedChunkUserIndex = index;
		key_type chunk_index;
		if constexpr(is_power_of_two) {
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
		} else {
			chunk_index = (index - (index % mod_val)) / chunks_size;
		}
		m_CachedChunk = &m_Sparse[chunk_index].value();
		return *m_CachedChunk;
	}

	constexpr FORCEINLINE auto
	chunk_info_for(key_type index, key_type& element_index, key_type& chunk_index) const noexcept -> void {
		if constexpr(is_power_of_two) {
			element_index = index & (mod_val);
			chunk_index	  = (index - element_index) / chunks_size;
		} else {
			chunk_index	  = (index - (index % mod_val)) / chunks_size;
			element_index = index % mod_val;
		}
	}


	psl::array<key_type> m_Reverse {};
	::memory::raw_region m_DenseData;
	psl::array<std::optional<chunk_type>> m_Sparse {};

	psl::static_array<key_type, 4> m_StageStart {0, 0, 0, 0};
	psl::static_array<key_type, 3> m_StageSize {0, 0, 0};

	mutable chunk_type* m_CachedChunk;
	mutable key_type m_CachedChunkUserIndex = std::numeric_limits<key_type>::max();

	size_t m_Size;						// size of underlying type
	void* m_ScratchMemory {nullptr};	// used by some methods to temporarily store a copy
};
}	 // namespace psl::ecs::details

#undef FORCEINLINE
