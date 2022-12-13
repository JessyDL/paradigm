#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "raw_region.hpp"
#include <algorithm>
#include <cstring>

namespace memory
{
/// \brief container type that is fast to iterate, but has non-continuous interface
template <typename T, typename Key = size_t, Key chunks_size = 4096>
class sparse_array
{
	using type_t	 = T;
	using index_type = Key;

	static constexpr bool is_power_of_two {chunks_size && ((chunks_size & (chunks_size - 1)) == 0)};
	static constexpr Key mod_val {(is_power_of_two) ? chunks_size - 1 : chunks_size};

  public:
	class iterator
	{
		friend class sparse_array<T, Key, chunks_size>;
		iterator(T* value, Key* index) : value(value), dense_index(index) {};

	  public:
		using difference_type	= index_type;
		using value_type		= T;
		using pointer			= value_type*;
		using const_pointer		= value_type* const;
		using reference			= value_type&;
		using const_reference	= const value_type&;
		using iterator_category = std::random_access_iterator_tag;

		iterator() noexcept							  = default;
		~iterator()									  = default;
		iterator(const iterator&) noexcept			  = default;
		iterator(iterator&&) noexcept				  = default;
		iterator& operator=(const iterator&) noexcept = default;
		iterator& operator=(iterator&&) noexcept =
		  delete;	 // disables mutating the container, todo we might support this but not right now.


		iterator& operator++() noexcept
		{
			++value;
			++dense_index;

			return *this;
		}

		iterator operator++(int) noexcept
		{
			iterator orig = *this;
			++(*this);
			return orig;
		}

		iterator& operator--() noexcept
		{
			--value;
			--dense_index;

			return *this;
		}

		iterator operator--(int) noexcept
		{
			iterator orig = *this;
			--(*this);
			return orig;
		}

		bool operator==(const iterator& other) const noexcept { return value == other.value; }
		bool operator!=(const iterator& other) const noexcept { return value != other.value; }

		pointer operator->() const noexcept { return value; }

		inline reference operator*() const noexcept { return *value; }

		index_type& index_of() const noexcept { return *dense_index; }

	  private:
		pointer value;
		index_type* dense_index;
	};
	using size_type			= typename psl::array<index_type>::size_type;
	using difference_type	= typename psl::array<index_type>::difference_type;
	using value_type		= T;
	using pointer			= value_type*;
	using const_pointer		= value_type* const;
	using reference			= value_type&;
	using const_reference	= const value_type&;
	using iterator_category = std::random_access_iterator_tag;

	sparse_array() noexcept : m_Reverse(), m_DenseData(m_Reverse.capacity() * sizeof(T)) { reserve(1024); };
	~sparse_array() = default;
	sparse_array(const sparse_array& other) noexcept :
		m_DenseData(other.m_DenseData), m_Reverse(other.m_Reverse), m_Sparse(other.m_Sparse) {};
	sparse_array(sparse_array&& other) noexcept :
		m_DenseData(std::move(other.m_DenseData)), m_Reverse(std::move(other.m_Reverse)),
		m_Sparse(std::move(other.m_Sparse)) {};
	sparse_array& operator=(const sparse_array& other) noexcept
	{
		if(this != &other)
		{
			m_DenseData = other.m_DenseData;
			m_Reverse	= other.m_Reverse;
			m_Sparse	= other.m_Sparse;
		}
		return *this;
	}
	sparse_array& operator=(sparse_array&& other) noexcept
	{
		if(this != &other)
		{
			m_DenseData = std::move(other.m_DenseData);
			m_Reverse	= std::move(other.m_Reverse);
			m_Sparse	= std::move(other.m_Sparse);
		}
		return *this;
	}
	size_type size() const noexcept { return m_Reverse.size(); }
	size_type capacity() const noexcept { return std::size(m_Sparse) * chunks_size; }

	iterator begin() noexcept { return iterator {(T*)m_DenseData.data(), m_Reverse.data()}; };
	iterator end() noexcept
	{
		return iterator {(T*)m_DenseData.data() + m_Reverse.size(), m_Reverse.data() + m_Reverse.size()};
	};

	void grow()
	{
		if(m_DenseData.size() < m_Reverse.capacity() * sizeof(T))
		{
			auto new_capacity = std::max(m_Reverse.capacity(), m_DenseData.size() * 2 / sizeof(T));

			::memory::raw_region reg(new_capacity * sizeof(T));
			std::memcpy(reg.data(), m_DenseData.data(), m_DenseData.size());
			m_DenseData = std::move(reg);

			m_Reverse.reserve(m_DenseData.size() / sizeof(T));
			psl_assert(m_Reverse.capacity() * sizeof(T) <= m_DenseData.size(), "capacity was less than density");
		}
	}

	reference operator[](index_type index)
	{
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		if(!has(index))
		{
			chunk[sub_index] = (index_type)m_Reverse.size();

			m_Reverse.emplace_back(index);
			grow();
		}
		return *((T*)m_DenseData.data() + chunk[sub_index]);
	}

	reference at(index_type index)
	{
		auto sub_index = index;
		auto& chunk	   = chunk_for(sub_index);

		if(!has(index))
		{
			chunk[sub_index] = (index_type)m_Reverse.size();
			m_Reverse.emplace_back(index);
			grow();
		}
		return *((T*)m_DenseData.data() + chunk[sub_index]);
	}

	const_reference at(index_type index) const noexcept
	{
		index_type sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		psl_assert(m_Sparse.size() > chunk_index && m_Sparse[chunk_index].size() > 0 &&
					 m_Sparse[chunk_index][sparse_index] != std::numeric_limits<index_type>::max(),
				   "unknown at location");
		return *((T*)m_DenseData.data() + m_Sparse[chunk_index][sparse_index]);
	}

	void resize(index_type size)
	{
		index_type chunk_index;
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
	void insert(index_type index)
	{
		const auto should_grow = m_Reverse.capacity() == m_Reverse.size();
		m_Reverse.emplace_back(index);
		auto& chunk	 = chunk_for(index);
		chunk[index] = (index_type)m_Reverse.size() - 1;
		if(should_grow) grow();
	}
	void insert(index_type index, const_reference value)
	{
		const auto should_grow = m_Reverse.capacity() == m_Reverse.size();
		m_Reverse.emplace_back(index);
		auto& chunk = chunk_for(index);

		chunk[index] = (index_type)m_Reverse.size() - 1;
		if(should_grow) grow();
		*((T*)m_DenseData.data() + chunk[index]) = value;
	}

	template <typename ItF, typename ItL>
	void insert(index_type index, ItF&& first, ItL&& last)
	{
		auto first_index = index;
		auto last_index	 = (index_type)std::distance(first, last) + index;
		auto end_index	 = last_index;
		index_type first_chunk;
		index_type last_chunk;
		chunk_info_for(first_index, first_index, first_chunk);
		chunk_info_for(last_index, last_index, last_chunk);

		if(end_index >= capacity()) resize(end_index + 1);

		for(auto i = first_index; i < last_index + 1; ++i)
		{
			if(m_Sparse[i].size() == 0)
			{
				m_Sparse[i].resize(chunks_size);
				std::fill_n(std::begin(m_Sparse[i]), chunks_size, std::numeric_limits<index_type>::max());
			}
		}

		auto it = first;

		for(auto i = first_chunk; i < last_chunk; ++i)
		{
			for(auto x = first_index; x < chunks_size; ++x)
			{
				m_Sparse[i][x] = (index_type)m_Reverse.size();
				m_Reverse.emplace_back(index++);
				grow();
				*((T*)m_DenseData.data() + m_Reverse.size() - 1) = *it;
				it												 = std::next(it);
			}
			first_index = 0;
		}

		for(auto x = 0u; x < last_index; ++x)
		{
			m_Sparse[last_index][x] = (index_type)m_Reverse.size();
			m_Reverse.emplace_back(index++);
			*((T*)m_DenseData.data() + m_Reverse.size() - 1) = *it;
			it												 = std::next(it);
		}
	}

	void emplace(index_type index, value_type&& value)
	{
		m_Reverse.emplace_back(index);
		auto& chunk = chunk_for(index);

		chunk[index] = (index_type)m_Reverse.size() - 1;

		*((T*)m_DenseData.data() + m_Reverse.size() - 1) = value;
	}

	constexpr bool has(index_type index) const noexcept
	{
		if(index < capacity())
		{
			index_type chunk_index;
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
			return m_Sparse[chunk_index].size() > 0 &&
				   m_Sparse[chunk_index][index] != std::numeric_limits<index_type>::max();
		}
		return false;
	}

	constexpr bool empty() const noexcept { return std::empty(m_Reverse); };

	void* data() noexcept { return m_DenseData.data(); };
	const void* data() const noexcept { return m_DenseData.data(); };

	void erase(index_type index) noexcept
	{
		index_type sparse_index, chunk_index;
		chunk_info_for(index, sparse_index, chunk_index);
		// assert(m_Sparse[chunk_index].size() > 0 && m_Sparse[chunk_index][sparse_index] !=
		// std::numeric_limits<index_type>::max()); if(m_Sparse[chunk_index].size() > 0 &&
		// m_Sparse[chunk_index][sparse_index] != std::numeric_limits<index_type>::max())
		{
			const auto dense_index				= m_Sparse[chunk_index][sparse_index];
			m_Sparse[chunk_index][sparse_index] = std::numeric_limits<index_type>::max();

			if(dense_index != m_Reverse.size() - 1)
			{
				std::swap(*((T*)m_DenseData.data() + dense_index), *((T*)m_DenseData.data() + m_Reverse.size() - 1));
				std::iter_swap(std::next(std::begin(m_Reverse), dense_index), std::prev(std::end(m_Reverse)));

				sparse_index = m_Reverse[dense_index];
				chunk_info_for(sparse_index, index, chunk_index);
				m_Sparse[chunk_index][index] = dense_index;
			}

			m_Reverse.pop_back();
		}
	}

	void clear() noexcept
	{
		m_Reverse.clear();
		m_Sparse.clear();
		reserve(1024);
	}
	void erase(index_type first, index_type last) noexcept
	{
		if(m_Sparse.size() == 0) return;

		first = std::min<index_type>(first, (index_type)capacity() - 1);
		last  = std::min<index_type>(last, (index_type)capacity() - 1);

		if(last - first == 1)
		{
			erase(first);
			return;
		}

		psl_assert(std::all_of(&first, &last, [this](index_type i) { return has(i); }),
				   "expected all indices between {} and {} to be available",
				   first,
				   last);

		if(last - first == m_Reverse.size())
		{
			clear();
			return;
		}
		auto first_index = first;
		auto last_index	 = last;
		index_type first_chunk;
		index_type last_chunk;
		chunk_info_for(first, first_index, first_chunk);
		chunk_info_for(last, last_index, last_chunk);

		index_type count {0};
		index_type processed {0};

		auto index = first_index;
		for(auto i = first_chunk; i < last_chunk; ++i)
		{
			if(m_Sparse[i].size() == 0)
			{
				index = 0u;
				continue;
			}

			for(auto x = index; x < chunks_size; ++x)
			{
				const auto dense_index {m_Sparse[i][x]};
				// if(dense_index == std::numeric_limits<index_type>::max())
				//	continue;
				++count;
				m_Sparse[i][x] = std::numeric_limits<index_type>::max();
				if(std::size(m_Reverse) - dense_index == count) continue;
				std::swap(*((T*)m_DenseData.data() + dense_index),
						  *((T*)m_DenseData.data() + m_Reverse.size() - count));
				std::iter_swap(std::next(std::begin(m_Reverse), dense_index), std::prev(std::end(m_Reverse), count));

				const auto new_index = m_Reverse[dense_index];

				// todo check if this optimization holds true
				// if(new_index > first && new_index < last)
				//	continue;
				index_type new_element_index, new_chunk_index;
				chunk_info_for(new_index, new_element_index, new_chunk_index);
				m_Sparse[new_chunk_index][new_element_index] = dense_index;
			}
			index = 0u;
		}
		if(m_Sparse[last_chunk].size() > 0)
		{
			for(auto x = index; x < last_index; ++x)
			{
				const auto dense_index {m_Sparse[last_chunk][x]};
				// if(dense_index == std::numeric_limits<index_type>::max())
				//	continue;
				++count;
				m_Sparse[last_chunk][x] = std::numeric_limits<index_type>::max();
				if(std::size(m_Reverse) - dense_index == count) continue;
				std::swap(*((T*)m_DenseData.data() + dense_index),
						  *((T*)m_DenseData.data() + m_Reverse.size() - count));
				std::iter_swap(std::next(std::begin(m_Reverse), dense_index), std::prev(std::end(m_Reverse), count));

				const auto new_index = m_Reverse[dense_index];

				// todo check if this optimization holds true
				// if(new_index > first && new_index < last)
				//	continue;
				index_type new_element_index, new_chunk_index;
				chunk_info_for(new_index, new_element_index, new_chunk_index);
				m_Sparse[new_chunk_index][new_element_index] = dense_index;
			}
		}


		m_Reverse.resize(m_Reverse.size() - count);
	}

	void reserve(size_t capacity)
	{
		if(capacity <= m_Reverse.capacity()) return;

		m_Reverse.reserve(capacity);
		grow();
	}

	psl::array_view<index_type> indices() const noexcept { return m_Reverse; }
	psl::array_view<value_type> dense() const noexcept
	{
		return psl::array_view<value_type> {(T*)m_DenseData.data(), (T*)m_DenseData.data() + m_Reverse.size()};
	}

  private:
	inline psl::array<index_type>& chunk_for(index_type& index)
	{
		if(index >= capacity()) resize(index + 1);
		index_type chunk_index;
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
		auto& chunk = m_Sparse[chunk_index];
		if(chunk.size() == 0)
		{
			chunk.resize(chunks_size);
			std::fill(std::begin(chunk), std::end(chunk), std::numeric_limits<index_type>::max());
		}

		return chunk;
	}

	inline void chunk_info_for(index_type index, index_type& element_index, index_type& chunk_index) const noexcept
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

	psl::array<index_type> m_Reverse;
	::memory::raw_region m_DenseData;
	psl::array<psl::array<index_type>> m_Sparse;
};
}	 // namespace memory