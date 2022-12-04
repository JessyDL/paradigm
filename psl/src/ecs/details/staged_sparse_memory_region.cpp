#include "psl/ecs/details/staged_sparse_memory_region.hpp"
#include "psl/utility/cast.hpp"

namespace psl::ecs::details
{
	staged_sparse_memory_region_t::staged_sparse_memory_region_t(size_t size) : m_DenseData(0), m_Size(size) { grow(); }

	auto staged_sparse_memory_region_t::has(key_type index, stage_range_t stage) const noexcept -> bool
	{
		if(index < capacity())
		{
			key_type chunk_index;
			if constexpr(is_power_of_two)
			{
				const auto element_index = index & (mod_val);
				chunk_index				 = (index - element_index) / chunks_size;
				index					 = element_index;
			}
			else
			{
				chunk_index = (index - (index % mod_val)) / chunks_size;
				index		= index % mod_val;
			}
			return has_impl(chunk_index, index, stage);
		}
		return false;
	}

	auto staged_sparse_memory_region_t::operator[](key_type index) -> reference
	{
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		if(!has(index))
		{
			insert_impl(chunk, sub_index, index);
		}
		return *((std::byte*)m_DenseData.data() + (chunk[sub_index] * m_Size));
	}

	auto staged_sparse_memory_region_t::reserve(size_t capacity) -> void
	{
		if(capacity <= m_Reverse.capacity()) return;

		m_Reverse.reserve(capacity);
		grow();
	}

	auto staged_sparse_memory_region_t::resize(key_type size) -> void
	{
		key_type chunk_index;
		if constexpr(is_power_of_two)
		{
			chunk_index = (size - (size & mod_val)) / chunks_size;
		}
		else
		{
			chunk_index = (size - (size % mod_val)) / chunks_size;
		}
		if(m_Sparse.size() <= chunk_index) m_Sparse.resize(chunk_index + 1);
	}

	auto staged_sparse_memory_region_t::addressof(key_type index, stage_range_t stage) const noexcept -> const_pointer
	{
		key_type sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		psl_assert(has(index, stage), "missing index {} within [{}, {}] in sparse array", index, stage);
		return (static_cast<const_pointer>(m_DenseData.data()) +
				(get_chunk_from_index(chunk_index)[sparse_index] * m_Size));
	}

	auto staged_sparse_memory_region_t::addressof(key_type index, stage_range_t stage) noexcept -> pointer
	{
		key_type sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		psl_assert(has(index, stage, stage_count), "missing index {} within [{}, {}] in sparse array", index, stage);
		return (static_cast<pointer>(m_DenseData.data()) + (get_chunk_from_index(chunk_index)[sparse_index] * m_Size));
	}

	auto staged_sparse_memory_region_t::erase(key_type first, key_type last) noexcept -> void
	{
		for(auto i = first; i < last; ++i)
		{
			auto sub_index = i;
			auto& chunk	   = chunk_for(sub_index);

			if(has_impl(chunk, sub_index)) erase_impl(chunk, sub_index, i);
		}
	}

	auto staged_sparse_memory_region_t::promote() noexcept -> void
	{
		for(auto i = m_StageStart[to_underlying(stage_t::REMOVED)]; i < m_Reverse.size(); ++i)
		{
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

	auto staged_sparse_memory_region_t::merge(const staged_sparse_memory_region_t& other) noexcept -> void
	{
		psl_assert(m_Size == other.m_Size,
				   "staged_sparse_memory_region_t should have the same component size, but instead got {} and {}",
				   m_Size,
				   other.m_Size);
		for(key_type i = 0; i < static_cast<key_type>(other.m_Reverse.size()); ++i)
		{
			auto sub_index = other.m_Reverse[i];
			auto& chunk	   = chunk_for(sub_index);
			if(!has_impl(chunk, sub_index))
			{
				insert_impl(chunk, sub_index, other.m_Reverse[i]);
			}

			memcpy((std::byte*)m_DenseData.data() + (chunk[sub_index] * m_Size),
				   (std::byte*)other.m_DenseData.data() + (i * m_Size),
				   m_Size);
		}
		for(key_type i = other.m_StageStart[2]; i < other.m_StageStart[3]; ++i)
		{
			erase(other.m_Reverse[i]);
		}
	}

	/// ---------------------------------------------------------------------------
	/// private member functions
	/// ---------------------------------------------------------------------------

	inline auto staged_sparse_memory_region_t::grow() -> void
	{
		auto capacity = psl::utility::narrow_cast<key_type>(m_Reverse.capacity() + 1);

		if(m_DenseData.size() < capacity * m_Size)
		{
			auto new_capacity =
			  std::max<key_type>(capacity, psl::utility::narrow_cast<key_type>(m_DenseData.size() * 2 / m_Size));

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

	inline auto staged_sparse_memory_region_t::insert_impl(chunk_type& chunk, key_type offset, key_type user_index)
	  -> void
	{
		for(auto i = m_StageStart[2]; i < m_Reverse.size(); ++i)
		{
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
		if(orig_cap != m_Reverse.capacity()) grow();
		psl_assert((m_Reverse.capacity() + 1) * m_Size <= m_DenseData.size(),
				   "{} <= {}",
				   (m_Reverse.capacity() + 1) * m_Size,
				   m_DenseData.size());
		m_StageStart[2] += 1;
		m_StageStart[3] += 1;
		m_StageSize[1] += 1;
	}

	constexpr inline auto staged_sparse_memory_region_t::has_impl(key_type chunk_index,
																  key_type offset,
																  stage_range_t stage) const noexcept -> bool
	{
		if(m_Sparse[chunk_index])
		{
			const auto& chunk = get_chunk_from_index(chunk_index);
			return chunk[offset] != std::numeric_limits<key_type>::max() &&
				   chunk[offset] >= m_StageStart[stage_begin(stage)] && chunk[offset] < m_StageStart[stage_end(stage)];
		}
		return false;
	}

	constexpr inline auto staged_sparse_memory_region_t::has_impl(chunk_type& chunk,
																  key_type offset,
																  stage_range_t stage) const noexcept -> bool
	{
		return chunk[offset] != std::numeric_limits<key_type>::max() &&
			   chunk[offset] >= m_StageStart[stage_begin(stage)] && chunk[offset] < m_StageStart[stage_end(stage)];
	}

	inline auto staged_sparse_memory_region_t::erase_impl(chunk_type& chunk, key_type offset, key_type user_index)
	  -> void
	{
		auto orig_value	   = this->operator[](user_index);
		auto reverse_index = chunk[offset];

		// figure out which stage it belonged to
		auto what_stage = (reverse_index < m_StageStart[1]) ? 0 : (reverse_index < m_StageStart[2]) ? 1 : 2;
		if(what_stage == 2) return;
		auto scratch_memory = malloc(m_Size);

		// we swap it out
		for(auto i = what_stage; i < 2; ++i)
		{
			if(reverse_index != m_StageStart[i + 1] - 1)
			{
				std::iter_swap(std::next(std::begin(m_Reverse), reverse_index),
							   std::next(std::begin(m_Reverse), m_StageStart[i + 1] - 1));

				std::memcpy(scratch_memory, (std::byte*)m_DenseData.data() + (reverse_index * m_Size), m_Size);
				std::memcpy((std::byte*)m_DenseData.data() + (reverse_index * m_Size),
							(std::byte*)m_DenseData.data() + ((m_StageStart[i + 1] - 1) * m_Size),
							m_Size);
				std::memcpy(
				  (std::byte*)m_DenseData.data() + ((m_StageStart[i + 1] - 1) * m_Size), scratch_memory, m_Size);
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

	constexpr inline auto staged_sparse_memory_region_t::get_chunk_from_index(key_type index) const noexcept
	  -> const chunk_type&
	{
		return m_Sparse[index].value();
	}
	constexpr inline auto staged_sparse_memory_region_t::get_chunk_from_index(key_type index) noexcept -> chunk_type&
	{
		return m_Sparse[index].value();
	}

	constexpr inline auto staged_sparse_memory_region_t::chunk_for(key_type& index) noexcept -> chunk_type&
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

	constexpr inline auto staged_sparse_memory_region_t::get_chunk_from_user_index(key_type index) const noexcept
	  -> const chunk_type&
	{
		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size) return *m_CachedChunk;
		m_CachedChunkUserIndex = index;
		key_type chunk_index;
		if constexpr(is_power_of_two)
		{
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
		}
		else
		{
			chunk_index = (index - (index % mod_val)) / chunks_size;
		}
		m_CachedChunk = const_cast<chunk_type*>(&m_Sparse[chunk_index].value());
		return *m_CachedChunk;
	}
	constexpr inline auto staged_sparse_memory_region_t::get_chunk_from_user_index(key_type index) noexcept
	  -> chunk_type&
	{
		if(index >= m_CachedChunkUserIndex && index < m_CachedChunkUserIndex + chunks_size) return *m_CachedChunk;
		m_CachedChunkUserIndex = index;
		key_type chunk_index;
		if constexpr(is_power_of_two)
		{
			const auto element_index = index & (mod_val);
			chunk_index				 = (index - element_index) / chunks_size;
		}
		else
		{
			chunk_index = (index - (index % mod_val)) / chunks_size;
		}
		m_CachedChunk = &m_Sparse[chunk_index].value();
		return *m_CachedChunk;
	}

	constexpr inline auto staged_sparse_memory_region_t::chunk_info_for(key_type index,
																		key_type& element_index,
																		key_type& chunk_index) const noexcept -> void
	{
		if constexpr(is_power_of_two)
		{
			element_index = index & (mod_val);
			chunk_index	  = (index - element_index) / chunks_size;
		}
		else
		{
			chunk_index	  = (index - (index % mod_val)) / chunks_size;
			element_index = index % mod_val;
		}
	}


	constexpr inline auto staged_sparse_memory_region_t::stage_begin(stage_range_t stage) const noexcept -> size_t
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

	constexpr inline auto staged_sparse_memory_region_t::stage_end(stage_range_t stage) const noexcept -> size_t
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

}	 // namespace psl::ecs::details
