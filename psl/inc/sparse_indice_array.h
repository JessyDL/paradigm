#pragma once
#include "vector.h"
#include "array.h"
#include <array_view.h>

namespace psl
{
	template <typename T, T chunks_size = 4096>
	class sparse_indice_array
	{
		static constexpr bool is_power_of_two{chunks_size && ((chunks_size & (chunks_size - 1)) == 0)};
		static constexpr T mod_val{(is_power_of_two) ? chunks_size - 1 : chunks_size};

		static_assert(std::is_integral_v<T>, "T must be an integral type");

	  public:
		sparse_indice_array()							= default;
		~sparse_indice_array()							= default;
		sparse_indice_array(const sparse_indice_array&) = default;
		sparse_indice_array(sparse_indice_array&&)		= default;
		sparse_indice_array& operator=(const sparse_indice_array&) = default;
		sparse_indice_array& operator=(sparse_indice_array&&) = default;

		size_t capacity() const noexcept { return m_Sparse.size() * chunks_size; }
		size_t size() const noexcept { return m_Reverse.size(); }

		T& operator[](const T& index)
		{
			auto chunk_index		   = index;
			auto& chunk				   = chunk_for(chunk_index);
			chunk[(size_t)chunk_index] = (T)m_Reverse.size();
			m_Reverse.emplace_back(index);
			return chunk[(size_t)chunk_index];
		}

		T& at(const T& index)
		{
			auto chunk_index		   = index;
			auto& chunk				   = chunk_for(chunk_index);
			chunk[(size_t)chunk_index] = (T)m_Reverse.size();
			m_Reverse.emplace_back(index);
			return chunk[(size_t)chunk_index];
		}

		void resize(size_t size)
		{
			size_t chunk_index;
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
		void insert(const T& index)
		{
			auto chunk_index   = index;
			auto& chunk		   = chunk_for(chunk_index);
			chunk[chunk_index] = (T)m_Reverse.size();
			m_Reverse.emplace_back(index);
		}

		void emplace(T&& index)
		{
			auto chunk_index   = index;
			auto& chunk		   = chunk_for(chunk_index);
			chunk[chunk_index] = m_Reverse.size();
			m_Reverse.emplace_back(std::move(index));
		}

		void erase(const T& index)
		{
			T sparse_index, chunk_index;
			chunk_info_for(index, sparse_index, chunk_index);

			const auto dense_index				= m_Sparse[chunk_index][sparse_index];
			m_Sparse[chunk_index][sparse_index] = std::numeric_limits<T>::max();

			if(dense_index != m_Reverse.size() - 1)
			{
				std::iter_swap(std::next(std::begin(m_Reverse), dense_index), std::prev(std::end(m_Reverse)));

				chunk_info_for(m_Reverse[dense_index], sparse_index, chunk_index);
				m_Sparse[chunk_index][sparse_index] = dense_index;
			}

			m_Reverse.pop_back();
		}

		void erase(const T& first, const T& last)
		{
			for(auto i = first; i < last; ++i) erase(i);
		}

		void clear()
		{
			m_Reverse.clear();
			m_Sparse.clear();
		}

		bool has(const T& index) const noexcept
		{
			T element_index, chunk_index;
			chunk_info_for(index, element_index, chunk_index);
			return m_Sparse.size() > chunk_index && m_Sparse[chunk_index].size() > 0 &&
				   m_Sparse[chunk_index][element_index] != std::numeric_limits<T>::max();
		}

		void reserve(size_t capacity) { m_Reverse.reserve(capacity); }

		psl::array_view<T> indices() const noexcept { return m_Reverse; }

	  private:
		inline psl::array<T>& chunk_for(T& index)
		{
			if(index >= capacity()) resize(index + 1);
			T chunk_index;
			if constexpr(is_power_of_two)
			{
				chunk_index = (index - (index & mod_val)) / chunks_size;
				index		= index & mod_val;
			}
			else
			{
				chunk_index = (index - (index % mod_val)) / chunks_size;
				index		= index % mod_val;
			}
			auto& chunk = m_Sparse[chunk_index];
			if(chunk.size() == 0)
			{
				chunk.resize(chunks_size);
				std::fill(std::begin(chunk), std::end(chunk), std::numeric_limits<T>::max());
			}

			return chunk;
		}

		inline void chunk_info_for(T index, T& element_index, T& chunk_index) const noexcept
		{
			if constexpr(is_power_of_two)
			{
				chunk_index   = (index - (index & mod_val)) / chunks_size;
				element_index = index & mod_val;
			}
			else
			{
				chunk_index   = (index - (index % mod_val)) / chunks_size;
				element_index = index % mod_val;
			}
		}


		psl::array<T> m_Reverse;
		psl::array<psl::array<T>> m_Sparse;
	};

} // namespace psl