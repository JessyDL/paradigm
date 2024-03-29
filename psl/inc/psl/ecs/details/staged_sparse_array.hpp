#pragma once

/// \brief specialized implementation of psl::sparse_array
///
/// Unlike normal sparse_array, this variation is implemented as a cascading stage, where stage 0 is the "stale" data,
/// stage 1 is the recently added data, and stage 2 is the removed data. This is catered towards usage within the
/// psl::ecs solution, which is why it is part of its namespace.
///
/// In general it can be unwieldy and unsafe to use without knowing the internal workings, so avoid using this unless
/// you know what you need it for and are sure you understand its internals.
#include "../entity.hpp"
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/assertions.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/sparse_array.hpp"
#include <cstring>	  // std::memmove
#include <memory>	  // std::uninitialized_move
#include <optional>

#include "psl/ecs/details/stage_range.hpp"

namespace psl::ecs::details {
template <typename T, typename Key = psl::ecs::entity_t::size_type, Key chunks_size = 4096>
class staged_sparse_array {
	using value_t = T;
	using index_t = Key;
	using chunk_t = psl::static_array<index_t, chunks_size>;

	static constexpr bool is_power_of_two {chunks_size && ((chunks_size & (chunks_size - 1)) == 0)};
	static constexpr index_t mod_val {(is_power_of_two) ? chunks_size - 1 : chunks_size};

  public:
	using size_type			= typename psl::array<index_t>::size_type;
	using difference_type	= typename psl::array<index_t>::difference_type;
	using value_type		= T;
	using pointer			= value_type*;
	using const_pointer		= value_type* const;
	using reference			= value_type&;
	using const_reference	= const value_type&;
	using iterator_category = std::random_access_iterator_tag;

	staged_sparse_array() noexcept : m_Reverse(), m_DenseData((m_Reverse.capacity() + 1) * sizeof(T)) {};
	~staged_sparse_array() {
		auto data = dense(stage_range_t::ALL);
		for(auto it : data) it.~value_type();
	}

	staged_sparse_array(const staged_sparse_array& other)				 = default;
	staged_sparse_array(staged_sparse_array&& other) noexcept			 = default;
	staged_sparse_array& operator=(const staged_sparse_array& other)	 = default;
	staged_sparse_array& operator=(staged_sparse_array&& other) noexcept = default;

	reference operator[](index_t index) {
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		if(!has(index)) {
			insert_impl(chunk, sub_index, index);
		}
		return *((T*)m_DenseData.data() + chunk[sub_index]);
	}

	// here for consistency /w staged_sparse_array_region
	template <typename Y>
	reference operator[](index_t index) {
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		if(!has(index)) {
			insert_impl(chunk, sub_index, index);
		}
		return *((T*)m_DenseData.data() + chunk[sub_index]);
	}

	const_reference at(index_t index, stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		return *addressof(index, stage);
	}

	reference at(index_t index, stage_range_t stage = stage_range_t::ALIVE) noexcept {
		return *addressof(index, stage);
	}

	// here for consistency /w staged_sparse_array_region
	template <typename Y>
	const_reference at(index_t index, stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		return *addressof(index, stage);
	}

	// here for consistency /w staged_sparse_array_region
	template <typename Y>
	reference at(index_t index, stage_range_t stage = stage_range_t::ALIVE) noexcept {
		return *addressof(index, stage);
	}


	pointer addressof(index_t index, stage_range_t stage = stage_range_t::ALIVE) noexcept {
		index_t sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		psl_assert(has(index, stage), "missing index {} within [{}] in sparse array", index, to_underlying(stage));
		return ((T*)m_DenseData.data() + get_chunk_from_index(chunk_index)[sparse_index]);
	}

	const_pointer addressof(index_t index, stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		index_t sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		psl_assert(has(index, stage), "missing index {} within [{}] in sparse array", index, to_underlying(stage));
		return ((T*)m_DenseData.data() + get_chunk_from_index(chunk_index)[sparse_index]);
	}

	pointer addressof_if(index_t index, stage_range_t stage = stage_range_t::ALIVE) noexcept {
		index_t sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		if(has(index, stage)) {
			return ((T*)m_DenseData.data() + get_chunk_from_index(chunk_index)[sparse_index]);
		}
		return nullptr;
	}

	const_pointer addressof_if(index_t index, stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		index_t sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		if(has(index, stage)) {
			return ((T*)m_DenseData.data() + get_chunk_from_index(chunk_index)[sparse_index]);
		}
		return nullptr;
	}

	inline auto dense_index_for(index_t index, stage_range_t stage = stage_range_t::ALIVE) const noexcept -> index_t {
		index_t sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		psl_assert(has(index, stage), "missing index {} within [{}] in sparse array", index, to_underlying(stage));
		return static_cast<index_t>(get_chunk_from_index(chunk_index)[sparse_index]);
	}

	void reserve(size_t capacity) {
		if(capacity <= m_Reverse.capacity())
			return;

		m_Reverse.reserve(capacity);
		grow();
	}

	void resize(index_t size) {
		index_t chunk_index;
		if constexpr(is_power_of_two) {
			chunk_index = (size - (size & mod_val)) / chunks_size;
		} else {
			chunk_index = (size - (size % mod_val)) / chunks_size;
		}
		if(m_Sparse.size() <= chunk_index)
			m_Sparse.resize(chunk_index + 1);
	}

	void erase(index_t first, index_t last) noexcept {
		// todo: stub implementation
		for(auto i = first; i < last; ++i) {
			erase(i);
		}
	}
	void erase(index_t index) noexcept {
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		if(has_impl(chunk, sub_index))
			erase_impl(chunk, sub_index, index);
	}

	constexpr bool has(index_t index, stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		if(index < capacity()) {
			index_t chunk_index;
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

	void emplace(index_t index, value_type&& value) { this->operator[](index) = std::forward<value_type>(value); }

	void insert(index_t index, const_reference value) { this->operator[](index) = value; }
	void insert(index_t index) {
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		insert_impl(chunk, sub_index, index);
	}

	psl::array_view<index_t> indices(stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		return psl::array_view<index_t> {std::next(m_Reverse.data(), m_StageStart[stage_begin(stage)]),
										 std::next(m_Reverse.data(), m_StageStart[stage_end(stage)])};
	}

	psl::array_view<value_t> dense(stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		return psl::array_view<value_t> {std::next((value_t*)m_DenseData.data(), m_StageStart[stage_begin(stage)]),
										 std::next((value_t*)m_DenseData.data(), m_StageStart[stage_end(stage)])};
	}

	// here for consistency /w staged_sparse_array_region
	template <typename Y>
	psl::array_view<value_t> dense(stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		return psl::array_view<value_t> {std::next((value_t*)m_DenseData.data(), m_StageStart[stage_begin(stage)]),
										 std::next((value_t*)m_DenseData.data(), m_StageStart[stage_end(stage)])};
	}

	void promote() noexcept {
		for(auto it = begin(stage_t::REMOVED); it != end(stage_t::REMOVED); ++it) it->~value_type();

		for(auto i = m_StageStart[2]; i < m_Reverse.size(); ++i) {
			auto offset	  = m_Reverse[i];
			auto& chunk	  = chunk_for(offset);
			chunk[offset] = std::numeric_limits<index_t>::max();
		}
		m_Reverse.erase(std::next(std::begin(m_Reverse), m_StageStart[2]), std::end(m_Reverse));

		m_StageSize[0] += m_StageSize[1];
		m_StageStart[1] = m_StageSize[0];
		m_StageStart[3] = m_StageStart[2];
		m_StageSize[1]	= 0;
		m_StageSize[2]	= 0;
	}

	size_type size(stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		return m_StageStart[stage_end(stage)] - m_StageStart[stage_begin(stage)];
	}

	size_type capacity() const noexcept { return std::size(m_Sparse) * chunks_size; }

	auto begin(stage_t stage = stage_t::SETTLED) noexcept {
		return std::next((T*)m_DenseData.data(), m_StageStart[to_underlying(stage)]);
	}
	auto end(stage_t stage = stage_t::ADDED) noexcept {
		return std::next((T*)m_DenseData.data(), m_StageStart[to_underlying(stage)]);
	}

	const auto cbegin(stage_t stage = stage_t::SETTLED) const noexcept {
		return std::next(m_DenseData.data(), m_StageStart[to_underlying(stage)]);
	}

	const auto cend(stage_t stage = stage_t::ADDED) const noexcept {
		return std::next(m_DenseData.data(), m_StageStart[to_underlying(stage) + 1]);
	}

	constexpr bool empty() const noexcept { return std::empty(m_Reverse); };
	void* data() noexcept { return m_DenseData.data(); };
	void* const data() const noexcept { return m_DenseData.data(); };

	void clear() noexcept {
		m_Reverse.clear();
		m_Sparse.clear();
		m_CachedChunkUserIndex = std::numeric_limits<index_t>::max();
		m_StageStart		   = {0, 0, 0, 0};
		m_StageSize			   = {0, 0, 0};
	}

	template <typename Fn>
	void remap(const psl::sparse_array<index_t>& mapping, Fn&& predicate) {
		psl_assert(m_Reverse.size() >= mapping.size(), "expected {} >= {}", m_Reverse.size(), mapping.size());
		m_Sparse.clear();
		m_CachedChunkUserIndex = std::numeric_limits<index_t>::max();
		for(index_t i = 0; i < static_cast<index_t>(m_Reverse.size()); ++i) {
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

	void merge(const staged_sparse_array& other) noexcept {
		for(index_t i = 0; i < static_cast<index_t>(other.m_Reverse.size()); ++i) {
			if(!has(other.m_Reverse[i])) {
				this->operator[](other.m_Reverse[i]) = *((T*)(other.m_DenseData.data()) + i);
			} else {
				at(other.m_Reverse[i]) = *((T*)(other.m_DenseData.data()) + i);
			}
		}
		for(index_t i = other.m_StageStart[2]; i < other.m_StageStart[3]; ++i) {
			erase(other.m_Reverse[i]);
		}
	}

  private:
	inline constexpr bool
	has_impl(index_t chunk_index, index_t offset, stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		if(m_Sparse[chunk_index]) {
			const auto& chunk = get_chunk_from_index(chunk_index);
			return chunk[offset] != std::numeric_limits<index_t>::max() &&
				   chunk[offset] >= m_StageStart[stage_begin(stage)] && chunk[offset] < m_StageStart[stage_end(stage)];
		}
		return false;
	}
	inline constexpr bool
	has_impl(chunk_t& chunk, index_t offset, stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		return chunk[offset] != std::numeric_limits<index_t>::max() &&
			   chunk[offset] >= m_StageStart[stage_begin(stage)] && chunk[offset] < m_StageStart[stage_end(stage)];
	}
	auto insert_impl(chunk_t& chunk, index_t offset, index_t user_index) {
		for(auto i = m_StageStart[2]; i < m_Reverse.size(); ++i) {
			auto old_offset = m_Reverse[i];
			auto& old_chunk = chunk_for(old_offset);
			old_chunk[old_offset] += 1;
		}

		if constexpr(std::is_trivially_copyable_v<T>) {
			std::memmove((T*)m_DenseData.data() + m_StageStart[2] + 1,
						 (T*)m_DenseData.data() + m_StageStart[2],
						 (m_Reverse.size() - m_StageStart[2]) * sizeof(T));
		} else {
			auto src	 = (T*)m_DenseData.data() + m_StageStart[2];	// beginning removed elements;
			auto src_end = src + m_StageSize[2] - 1;

			auto dst = src + 1;

			if(m_StageSize[2] > 0) {
				if constexpr(std::is_move_constructible_v<T>)
					std::uninitialized_move(src_end, src_end + 1, src_end + 1);
				else
					std::uninitialized_copy(src_end, src_end + 1, src_end + 1);
			}

			if(m_StageSize[2] > 1) {
				std::rotate(src, src + (m_StageSize[2] - 1), src_end);
			}

			if(m_StageSize[2] > 0) {
				if constexpr(!std::is_trivially_destructible_v<T>)
					src->~value_type();
			}
		}

		if constexpr(!std::is_trivially_constructible_v<T>) {
#ifdef new
	#define STACK_NEW new
	#undef new
#endif
			new(reinterpret_cast<T*>(m_DenseData.data()) + m_StageStart[2]) value_type();
#ifdef STACK_NEW
	#define new STACK_NEW
	#undef STACK_NEW
#endif
		}

		chunk[offset] = static_cast<index_t>(m_StageStart[2]);
		auto orig_cap = m_Reverse.capacity();
		m_Reverse.emplace(std::next(std::begin(m_Reverse), m_StageStart[2]), user_index);
		if(orig_cap != m_Reverse.capacity())
			grow();
		psl_assert((m_Reverse.capacity() + 1) * sizeof(value_t) <= m_DenseData.size(),
				   "{} <= {}",
				   (m_Reverse.capacity() + 1) * sizeof(value_t),
				   m_DenseData.size());
		m_StageStart[2] += 1;
		m_StageStart[3] += 1;
		m_StageSize[1] += 1;
	}

	auto erase_impl(chunk_t& chunk, index_t offset, index_t user_index) {
		auto reverse_index = chunk[offset];

		// chunk[offset] = std::numeric_limits<index_t>::max();
		// figure out which stage it belonged to
		auto what_stage = (reverse_index < m_StageStart[1]) ? 0 : (reverse_index < m_StageStart[2]) ? 1 : 2;
		if(what_stage == 2)
			return;

		// we swap it out
		for(auto i = what_stage; i < 2; ++i) {
			if(reverse_index != m_StageStart[i + 1] - 1) {
				std::iter_swap(std::next(std::begin(m_Reverse), reverse_index),
							   std::next(std::begin(m_Reverse), m_StageStart[i + 1] - 1));
				std::iter_swap((T*)m_DenseData.data() + reverse_index,
							   (T*)m_DenseData.data() + (m_StageStart[i + 1] - 1));
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
		// if constexpr(std::is_same<value_t, int>())
		//	assert(orig_value == this->at(user_index,2,2));

		psl_assert(chunk[offset] == reverse_index, "expected {} == {}", chunk[offset], reverse_index);
	}

	void grow() {
		auto capacity = m_Reverse.capacity() + 1;
		if(m_DenseData.size() < capacity * sizeof(T)) {
			auto new_capacity = std::max<size_t>(capacity, m_DenseData.size() * 2 / sizeof(T));

			::memory::raw_region reg(new_capacity * sizeof(T));

			if constexpr(std::is_trivially_copyable_v<T>) {
				std::memcpy(reg.data(), m_DenseData.data(), m_DenseData.size());
			} else {
				auto src = (T*)m_DenseData.data(), end = src + size(stage_range_t::ALL), dst = (T*)reg.data();

				if constexpr(std::is_move_constructible_v<T>)
					std::uninitialized_move(src, end, dst);
				else
					std::uninitialized_copy(src, end, dst);

				if constexpr(!std::is_trivially_destructible_v<T>) {
					for(auto it = src; it != end; ++it) {
						it->~T();
					}
				}
			}

			m_DenseData = std::move(reg);

			m_Reverse.reserve((m_DenseData.size() / sizeof(T)) - 1);

			psl_assert((m_Reverse.capacity() + 1) * sizeof(value_t) <= m_DenseData.size() &&
						 (m_Reverse.capacity() + 2) * sizeof(value_t) >= m_DenseData.size(),
					   "capacity was not in line with density data");
		}
	}

	inline chunk_t& chunk_for(index_t& index) noexcept {
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
		index_t chunk_index;
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
		std::optional<chunk_t>& chunk = m_Sparse[chunk_index];
		if(!chunk) {
			chunk = chunk_t {};
			// chunk.resize(chunks_size);
			std::fill(std::begin(chunk.value()), std::end(chunk.value()), std::numeric_limits<index_t>::max());
		}
		m_CachedChunk = &chunk.value();
		return chunk.value();
	}

	inline const chunk_t& get_chunk_from_user_index(index_t index) const noexcept {
		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size)
			return *m_CachedChunk;
		m_CachedChunkUserIndex = index;
		index_t chunk_index;
		if constexpr(is_power_of_two) {
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
		} else {
			chunk_index = (index - (index % mod_val)) / chunks_size;
		}
		m_CachedChunk = &m_Sparse[chunk_index].value();
		return *m_CachedChunk;
	}
	inline chunk_t& get_chunk_from_user_index(index_t index) noexcept {
		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size)
			return *m_CachedChunk;
		m_CachedChunkUserIndex = index;
		index_t chunk_index;
		if constexpr(is_power_of_two) {
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
		} else {
			chunk_index = (index - (index % mod_val)) / chunks_size;
		}
		m_CachedChunk = &m_Sparse[chunk_index].value();
		return *m_CachedChunk;
	}

	inline const chunk_t& get_chunk_from_index(index_t index) const noexcept {
		return m_Sparse[index].value();
	}


	inline chunk_t& get_chunk_from_index(index_t index) noexcept {
		return m_Sparse[index].value();
	}

	inline void chunk_info_for(index_t index, index_t& element_index, index_t& chunk_index) const noexcept {
		if constexpr(is_power_of_two) {
			element_index = index & (mod_val);
			chunk_index	  = (index - element_index) / chunks_size;
		} else {
			chunk_index	  = (index - (index % mod_val)) / chunks_size;
			element_index = index % mod_val;
		}
	}

	psl::array<index_t> m_Reverse;
	::memory::raw_region m_DenseData;
	psl::array<std::optional<chunk_t>> m_Sparse;

	// sizes for each stage, this means stage 1 starts at std::begin() + m_Stage0
	psl::static_array<index_t, 4> m_StageStart {0, 0, 0, 0};
	psl::static_array<index_t, 3> m_StageSize {0, 0, 0};
	mutable chunk_t* m_CachedChunk;
	mutable index_t m_CachedChunkUserIndex = std::numeric_limits<index_t>::max();
};

template <typename Key, Key chunks_size>
class staged_sparse_array<void, Key, chunks_size> {
	using index_t = Key;
	using chunk_t = psl::static_array<index_t, chunks_size>;

	static constexpr bool is_power_of_two {chunks_size && ((chunks_size & (chunks_size - 1)) == 0)};
	static constexpr index_t mod_val {(is_power_of_two) ? chunks_size - 1 : chunks_size};

  public:
	using size_type			= typename psl::array<index_t>::size_type;
	using difference_type	= typename psl::array<index_t>::difference_type;
	using iterator_category = std::random_access_iterator_tag;

	staged_sparse_array() noexcept : m_Reverse() {};
	~staged_sparse_array() = default;

	staged_sparse_array(const staged_sparse_array& other)				 = default;
	staged_sparse_array(staged_sparse_array&& other) noexcept			 = default;
	staged_sparse_array& operator=(const staged_sparse_array& other)	 = default;
	staged_sparse_array& operator=(staged_sparse_array&& other) noexcept = default;

	bool operator[](index_t index) const noexcept { return has(index); }

	void reserve(size_t capacity) {
		if(capacity <= m_Reverse.capacity())
			return;

		m_Reverse.reserve(capacity);
	}

	void resize(index_t size) {
		index_t chunk_index;
		if constexpr(is_power_of_two) {
			chunk_index = (size - (size & mod_val)) / chunks_size;
		} else {
			chunk_index = (size - (size % mod_val)) / chunks_size;
		}
		if(m_Sparse.size() <= chunk_index)
			m_Sparse.resize(chunk_index + 1);
	}

	void erase(index_t first, index_t last) noexcept {
		// todo: stub implementation
		for(auto i = first; i < last; ++i) {
			erase(i);
		}
	}
	void erase(index_t index) noexcept {
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);
		if(has(index)) {
			erase_impl(chunk, sub_index, index);
		}
	}

	constexpr bool has(index_t index, stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		if(index < capacity()) {
			index_t chunk_index;
			if constexpr(is_power_of_two) {
				const auto element_index = index & (mod_val);
				chunk_index				 = (index - element_index) / chunks_size;
				index					 = element_index;
			} else {
				chunk_index = (index - (index % mod_val)) / chunks_size;
				index		= index % mod_val;
			}
			if(m_Sparse[chunk_index]) {
				const auto& chunk = get_chunk_from_index(chunk_index);
				return chunk[index] != std::numeric_limits<index_t>::max() &&
					   chunk[index] >= m_StageStart[stage_begin(stage)] &&
					   chunk[index] < m_StageStart[stage_end(stage)];
			}
		}
		return false;
	}

	template <typename ItF, typename ItL>
	void insert(index_t index, ItF&& first, ItL&& last) {
		// todo: stub implementation
		while(first != last) {
			insert(index, *first);
			first = std::next(first);
			index += 1;
		}
	}

	void insert(index_t index) {
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		insert_impl(chunk, sub_index, index);
	}


	psl::array_view<index_t> indices(stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		return psl::array_view<index_t> {std::next(m_Reverse.data(), m_StageStart[stage_begin(stage)]),
										 std::next(m_Reverse.data(), m_StageStart[stage_end(stage)])};
	}

	void promote() noexcept {
		for(auto i = m_StageStart[2]; i < m_Reverse.size(); ++i) {
			auto offset	  = m_Reverse[i];
			auto& chunk	  = chunk_for(offset);
			chunk[offset] = std::numeric_limits<index_t>::max();
		}
		m_Reverse.erase(std::next(std::begin(m_Reverse), m_StageStart[2]), std::end(m_Reverse));

		m_StageSize[0] += m_StageSize[1];
		m_StageStart[1] = m_StageSize[0];
		m_StageStart[3] = m_StageStart[2];
		m_StageSize[1]	= 0;
		m_StageSize[2]	= 0;
	}

	size_type size(stage_range_t stage = stage_range_t::ALIVE) const noexcept {
		return m_StageStart[stage_end(stage)] - m_StageStart[stage_begin(stage)];
	}
	size_type capacity() const noexcept { return std::size(m_Sparse) * chunks_size; }

	constexpr bool empty() const noexcept { return std::empty(m_Reverse); };

	void clear() noexcept {
		m_Reverse.clear();
		m_Sparse.clear();
		m_CachedChunkUserIndex = std::numeric_limits<index_t>::max();
		m_StageStart		   = {0, 0, 0, 0};
		m_StageSize			   = {0, 0, 0};
	}

	template <typename Fn>
	void remap(const psl::sparse_array<index_t>& mapping, Fn&& predicate) {
		psl_assert(m_Reverse.size() >= mapping.size(), "expected {} >= {}", m_Reverse.size(), mapping.size());
		m_Sparse.clear();
		m_CachedChunkUserIndex = std::numeric_limits<index_t>::max();
		for(index_t i = 0; i < static_cast<index_t>(m_Reverse.size()); ++i) {
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

	void merge(const staged_sparse_array& other) noexcept {
		for(index_t i = 0; i < static_cast<index_t>(other.m_Reverse.size()); ++i) {
			if(!has(other.m_Reverse[i])) {
				insert(other.m_Reverse[i]);
			}
		}
		for(index_t i = other.m_StageStart[2]; i < other.m_StageStart[3]; ++i) {
			erase(other.m_Reverse[i]);
		}
	}

  private:
	auto insert_impl(chunk_t& chunk, index_t offset, index_t user_index) {
		for(auto i = m_StageStart[2]; i < m_Reverse.size(); ++i) {
			auto old_offset = m_Reverse[i];
			auto& old_chunk = chunk_for(old_offset);
			old_chunk[old_offset] += 1;
		}

		chunk[offset] = static_cast<index_t>(m_StageStart[2]);
		m_Reverse.emplace(std::next(std::begin(m_Reverse), m_StageStart[2]), user_index);

		m_StageStart[2] += 1;
		m_StageStart[3] += 1;
		m_StageSize[1] += 1;
	}

	auto erase_impl(chunk_t& chunk, index_t offset, index_t user_index) {
		auto orig_value	   = this->operator[](user_index);
		auto reverse_index = chunk[offset];

		// chunk[offset] = std::numeric_limits<index_t>::max();
		// figure out which stage it belonged to
		auto what_stage = (reverse_index < m_StageStart[1]) ? 0 : (reverse_index < m_StageStart[2]) ? 1 : 2;
		if(what_stage == 2)
			return;

		// we swap it out
		for(auto i = what_stage; i < 2; ++i) {
			if(reverse_index != m_StageStart[i + 1] - 1) {
				std::iter_swap(std::next(std::begin(m_Reverse), reverse_index),
							   std::next(std::begin(m_Reverse), m_StageStart[i + 1] - 1));

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
		// if constexpr(std::is_same<value_t, int>())
		//	assert(orig_value == this->at(user_index,2,2));

		psl_assert(chunk[offset] == reverse_index, "{} == {}", chunk[offset], reverse_index);
	}

	inline chunk_t& chunk_for(index_t& index) noexcept {
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
		index_t chunk_index;
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
		std::optional<chunk_t>& chunk = m_Sparse[chunk_index];
		if(!chunk) {
			chunk = chunk_t {};
			// chunk.resize(chunks_size);
			std::fill(std::begin(chunk.value()), std::end(chunk.value()), std::numeric_limits<index_t>::max());
		}
		m_CachedChunk = &chunk.value();
		return chunk.value();
	}

	inline const chunk_t& get_chunk_from_user_index(index_t index) const noexcept {
		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size)
			return *m_CachedChunk;
		m_CachedChunkUserIndex = index;
		index_t chunk_index;
		if constexpr(is_power_of_two) {
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
		} else {
			chunk_index = (index - (index % mod_val)) / chunks_size;
		}
		m_CachedChunk = &m_Sparse[chunk_index].value();
		return *m_CachedChunk;
	}
	inline chunk_t& get_chunk_from_user_index(index_t index) noexcept {
		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size)
			return *m_CachedChunk;
		m_CachedChunkUserIndex = index;
		index_t chunk_index;
		if constexpr(is_power_of_two) {
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
		} else {
			chunk_index = (index - (index % mod_val)) / chunks_size;
		}
		m_CachedChunk = &m_Sparse[chunk_index].value();
		return *m_CachedChunk;
	}

	inline const chunk_t& get_chunk_from_index(index_t index) const noexcept { return m_Sparse[index].value(); }


	inline chunk_t& get_chunk_from_index(index_t index) noexcept { return m_Sparse[index].value(); }

	inline void chunk_info_for(index_t index, index_t& element_index, index_t& chunk_index) const noexcept {
		if constexpr(is_power_of_two) {
			element_index = index & (mod_val);
			chunk_index	  = (index - element_index) / chunks_size;
		} else {
			chunk_index	  = (index - (index % mod_val)) / chunks_size;
			element_index = index % mod_val;
		}
	}

	psl::array<index_t> m_Reverse;
	psl::array<std::optional<chunk_t>> m_Sparse;

	// sizes for each stage, this means stage 1 starts at std::begin() + m_Stage0
	psl::static_array<index_t, 4> m_StageStart {0, 0, 0, 0};
	psl::static_array<index_t, 3> m_StageSize {0, 0, 0};
	mutable chunk_t* m_CachedChunk;
	mutable index_t m_CachedChunkUserIndex = std::numeric_limits<index_t>::max();
};
}	 // namespace psl::ecs::details
