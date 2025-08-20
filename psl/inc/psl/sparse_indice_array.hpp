#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/details/buffer_growth_strategy.hpp"
#include "psl/static_array.hpp"
#include <algorithm>

namespace psl {
template <typename T, T chunks_size = 4096, typename BufferGrowthStrategy = details::default_buffer_growth_strategy_t>
class sparse_indice_array {
	static constexpr bool is_power_of_two {chunks_size && ((chunks_size & (chunks_size - 1)) == 0)};
	static constexpr T mod_val {(is_power_of_two) ? chunks_size - 1 : chunks_size};

	static_assert(std::is_integral_v<T>, "T must be an integral type");

  public:
	sparse_indice_array()									   = default;
	~sparse_indice_array()									   = default;
	sparse_indice_array(const sparse_indice_array&)			   = default;
	sparse_indice_array(sparse_indice_array&&)				   = default;
	sparse_indice_array& operator=(const sparse_indice_array&) = default;
	sparse_indice_array& operator=(sparse_indice_array&&)	   = default;

	size_t capacity() const noexcept {
		return m_Sparse.size() * chunks_size;
	}
	size_t size() const noexcept {
		return m_Reverse.size();
	}

	T& operator[](const T& index) {
		if(index < m_Offset) {
			auto aligned_index = chunk_aligned_index(index);
			if(m_Offset != OFFSET_START) {
				pad_front((m_Offset - aligned_index) / chunks_size);
			}
			m_Offset = aligned_index;
		}
		auto chunk_index		   = index;
		auto& chunk				   = chunk_for(chunk_index);
		chunk[(size_t)chunk_index] = (T)m_Reverse.size();
		m_Reverse.emplace_back(index);
		return chunk[(size_t)chunk_index];
	}

	T& at(const T& index) {
		if(index < m_Offset) {
			auto aligned_index = chunk_aligned_index(index);
			if(m_Offset != OFFSET_START) {
				pad_front((m_Offset - aligned_index) / chunks_size);
			}
			m_Offset = aligned_index;
		}
		auto chunk_index		   = index;
		auto& chunk				   = chunk_for(chunk_index);
		chunk[(size_t)chunk_index] = (T)m_Reverse.size();
		m_Reverse.emplace_back(index);
		return chunk[(size_t)chunk_index];
	}

	void resize(size_t size) {
		size_t chunk_index;
		if constexpr(is_power_of_two) {
			chunk_index = (size - (size & mod_val)) / chunks_size;
		} else {
			chunk_index = (size - (size % mod_val)) / chunks_size;
		}
		if(m_Sparse.size() <= chunk_index) {
			m_Sparse.resize(chunk_index + 1);
		}
	}
	void insert(const T& index) {
		if(index < m_Offset) {
			auto aligned_index = chunk_aligned_index(index);
			if(m_Offset != OFFSET_START) {
				pad_front((m_Offset - aligned_index) / chunks_size);
			}
			m_Offset = aligned_index;
		}
		auto chunk_index   = index;
		auto& chunk		   = chunk_for(chunk_index);
		chunk[chunk_index] = (T)m_Reverse.size();
		m_Reverse.emplace_back(index);
	}

	bool try_insert(const T& index) {
		if(index < m_Offset) {
			auto aligned_index = chunk_aligned_index(index);
			if(m_Offset != OFFSET_START) {
				pad_front((m_Offset - aligned_index) / chunks_size);
			}
			m_Offset = aligned_index;
		}
		auto chunk_index = index;
		auto& chunk		 = chunk_for(chunk_index);
		if(chunk[chunk_index] == std::numeric_limits<T>::max()) {
			chunk[chunk_index] = (T)m_Reverse.size();
			m_Reverse.emplace_back(index);
			return true;
		}
		return false;
	}

	template <typename InputIt>
		requires std::input_iterator<InputIt>
	size_t try_insert_presorted(InputIt first, InputIt last) {
		auto const total {std::distance(first, last)};
		if(total == 0) {
			return 0;
		}
		if(total == 1) {
			return try_insert(*first) ? 1 : 0;
		}

		m_Reverse.reserve(BufferGrowthStrategy {}.growth(m_Reverse.capacity(), m_Reverse.size() + total));

		psl::array_view<T> indices {first, last};
		if(indices[0] < m_Offset) {
			auto aligned_index = chunk_aligned_index(indices[0]);
			if(m_Offset != OFFSET_START) {
				pad_front((m_Offset - aligned_index) / chunks_size);
			}
			m_Offset = aligned_index;
		}

		{
			auto const last_index = *std::prev(last);
			T element_index, chunk_index;
			chunk_info_for(last_index, element_index, chunk_index);
			if(m_Sparse.size() <= chunk_index) {
				m_Sparse.resize(chunk_index + 1);
			}
		}


		size_t count {0};
		size_t i {0};
		do {
			auto const first_index = indices[i];
			T element_index, chunk_index;
			chunk_info_for(first_index, element_index, chunk_index);
			auto& chunk = m_Sparse[chunk_index];
			if(chunk.size() == 0) {
				chunk.resize(chunks_size, std::numeric_limits<T>::max());
			}
			size_t next_treshold {(chunk_index + 1) * chunks_size};

			do {
				auto const next_index = indices[i];
				auto const diff		  = next_index - first_index;
				if(chunk[element_index + diff] == std::numeric_limits<T>::max()) {
					chunk[element_index + diff] = (T)m_Reverse.size();
					m_Reverse.emplace_back(next_index);
					++count;
				}
				++i;
			} while(i < indices.size() && indices[i] < next_treshold);
		} while(i < indices.size());

		return count;
	}

	template <typename InputIt>
		requires std::input_iterator<InputIt>
	size_t try_insert(InputIt first, InputIt last) {
		if(std::is_sorted(first, last)) {
			return try_insert_presorted(first, last);
		}
		psl::array<T> indices(first, last);
		std::sort(std::begin(indices), std::end(indices));
		return try_insert_presorted(std::begin(indices), std::end(indices));
	}

	void emplace(T&& index) {
		if(index < m_Offset) {
			auto aligned_index = chunk_aligned_index(index);
			if(m_Offset != OFFSET_START) {
				pad_front((m_Offset - aligned_index) / chunks_size);
			}
			m_Offset = aligned_index;
		}
		auto chunk_index   = index;
		auto& chunk		   = chunk_for(chunk_index);
		chunk[chunk_index] = m_Reverse.size();
		m_Reverse.emplace_back(std::move(index));
	}

	void erase(const T& index) {
		T sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);

		const auto dense_index				= m_Sparse[chunk_index][sparse_index];
		m_Sparse[chunk_index][sparse_index] = std::numeric_limits<T>::max();

		if(dense_index != m_Reverse.size() - 1) {
			std::iter_swap(std::next(std::begin(m_Reverse), dense_index), std::prev(std::end(m_Reverse)));

			chunk_info_for(m_Reverse[dense_index], sparse_index, chunk_index);
			m_Sparse[chunk_index][sparse_index] = dense_index;
		}

		m_Reverse.pop_back();
	}

	void erase(T first, T last) {
		if(m_Sparse.size() == 0)
			return;

		first = std::min<T>(first, (T)capacity() - 1);
		last  = std::min<T>(last, (T)capacity() - 1);

		if(last - first == 1) {
			erase(first);
			return;
		}

		psl_assert(std::all_of(&first, &last, [this](T i) { return has(i); }),
				   "Tried to erase an invalid index from the sparse array when erasing the range [{}, {})",
				   first,
				   last);

		auto first_index = first;
		auto last_index	 = last;
		T first_chunk;
		T last_chunk;
		chunk_info_for(first, first_index, first_chunk);
		chunk_info_for(last, last_index, last_chunk);

		T count {0};
		T processed {0};

		auto index = first_index;
		for(auto i = first_chunk; i < last_chunk; ++i) {
			if(m_Sparse[i].size() == 0) {
				index = 0u;
				continue;
			}

			for(auto x = index; x < chunks_size; ++x) {
				const auto reverse_index {m_Sparse[i][x]};
				// if(dense_index == std::numeric_limits<index_type>::max())
				//	continue;
				++count;
				m_Sparse[i][x] = std::numeric_limits<T>::max();
				if(std::size(m_Reverse) - reverse_index == count)
					continue;
				std::iter_swap(std::next(std::begin(m_Reverse), reverse_index), std::prev(std::end(m_Reverse), count));

				const auto new_index = m_Reverse[reverse_index];

				// todo check if this optimization holds true
				// if(new_index > first && new_index < last)
				//	continue;
				T new_element_index, new_chunk_index;
				chunk_info_for(new_index, new_element_index, new_chunk_index);
				m_Sparse[new_chunk_index][new_element_index] = reverse_index;
			}
			index = 0u;
		}
		if(m_Sparse[last_chunk].size() > 0) {
			for(auto x = index; x < last_index; ++x) {
				const auto reverse_index {m_Sparse[last_chunk][x]};
				// if(dense_index == std::numeric_limits<index_type>::max())
				//	continue;
				++count;
				m_Sparse[last_chunk][x] = std::numeric_limits<T>::max();
				if(std::size(m_Reverse) - reverse_index == count)
					continue;
				std::iter_swap(std::next(std::begin(m_Reverse), reverse_index), std::prev(std::end(m_Reverse), count));

				const auto new_index = m_Reverse[reverse_index];

				// todo check if this optimization holds true
				// if(new_index > first && new_index < last)
				//	continue;
				T new_element_index, new_chunk_index;
				chunk_info_for(new_index, new_element_index, new_chunk_index);
				m_Sparse[new_chunk_index][new_element_index] = reverse_index;
			}
		}

		m_Reverse.erase(std::prev(m_Reverse.end(), count), m_Reverse.end());
	}

	void clear() {
		m_Reverse.clear();
		m_Sparse.clear();
		m_Offset			   = OFFSET_START;
		m_CachedChunk		   = nullptr;
		m_CachedChunkUserIndex = std::numeric_limits<T>::max();
	}

	bool has(const T& index) const noexcept {
		T element_index, chunk_index;
		chunk_info_for(index, element_index, chunk_index);
		return m_Sparse.size() > chunk_index && m_Sparse[chunk_index].size() > 0 &&
			   m_Sparse[chunk_index][element_index] != std::numeric_limits<T>::max();
	}

	void reserve(size_t capacity) {
		m_Reverse.reserve(BufferGrowthStrategy {}.growth(m_Reverse.capacity(), capacity));
	}

	psl::array_view<T> indices() const noexcept {
		return m_Reverse;
	}

  private:
	void pad_front(size_t count) {
		m_Sparse.resize(m_Sparse.size() + count);
		std::rotate(std::rbegin(m_Sparse), std::rbegin(m_Sparse) + count, std::rend(m_Sparse));
	}
	static constexpr T chunk_aligned_index(const T& index) {
		if constexpr(is_power_of_two) {
			return index - (index & mod_val);
		} else {
			return index - (index % mod_val);
		}
	}
	inline psl::array<T>& chunk_for(T& index) {
		index -= m_Offset;
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

		T chunk_index;
		if constexpr(is_power_of_two) {
			auto mod_index		   = index & mod_val;
			m_CachedChunkUserIndex = (index - mod_index);
			chunk_index			   = (index - mod_index) / chunks_size;
			index				   = mod_index;
		} else {
			auto mod_index		   = index % mod_val;
			m_CachedChunkUserIndex = (index - mod_index);
			chunk_index			   = (index - mod_index) / chunks_size;
			index				   = mod_index;
		}
		auto& chunk = m_Sparse[chunk_index];
		if(chunk.size() == 0) {
			chunk.resize(chunks_size);
			std::fill(std::begin(chunk), std::end(chunk), std::numeric_limits<T>::max());
		}

		m_CachedChunk = &chunk;
		return chunk;
	}

	inline void chunk_info_for(T index, T& element_index, T& chunk_index) const noexcept {
		index -= m_Offset;
		if constexpr(is_power_of_two) {
			auto mod_index = index & mod_val;
			chunk_index	   = (index - mod_index) / chunks_size;
			element_index  = mod_index;
		} else {
			auto mod_index = index % mod_val;
			chunk_index	   = (index - mod_index) / chunks_size;
			element_index  = mod_index;
		}
	}
	static const T OFFSET_START {chunk_aligned_index(std::numeric_limits<T>::max())};

	T m_Offset {OFFSET_START};
	psl::array<T> m_Reverse;
	psl::array<psl::array<T>> m_Sparse;

	mutable psl::array<T>* m_CachedChunk {nullptr};
	mutable T m_CachedChunkUserIndex {std::numeric_limits<T>::max()};
};
}	 // namespace psl
