#pragma once
#include "psl/array.h"
#include "psl/array_view.h"

namespace psl
{
	/// \brief container type that is fast to iterate, but has non-continuous interface
	template <typename T, typename Key = size_t, Key chunks_size = 4096>
	class sparse_array
	{
		using type_t	 = T;
		using index_type = Key;

		static constexpr bool is_power_of_two{chunks_size && ((chunks_size & (chunks_size - 1)) == 0)};
		static constexpr Key mod_val{(is_power_of_two) ? chunks_size - 1 : chunks_size};

	  public:
		class iterator
		{
			friend class sparse_array<T, Key, chunks_size>;
			iterator(T* value, Key* index) : value(value), dense_index(index){};

		  public:
			using difference_type   = index_type;
			using value_type		= T;
			using pointer			= value_type*;
			using const_pointer		= value_type* const;
			using reference			= value_type&;
			using const_reference   = const value_type&;
			using iterator_category = std::random_access_iterator_tag;

			iterator() noexcept				   = default;
			~iterator()						   = default;
			iterator(const iterator&) noexcept = default;
			iterator(iterator&&) noexcept	  = default;
			iterator& operator=(const iterator&) noexcept = default;
			iterator& operator							  =(iterator&&) noexcept =
				delete; // disables mutating the container, todo we might support this but not right now.


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
		using difference_type   = typename psl::array<index_type>::difference_type;
		using value_type		= T;
		using pointer			= value_type*;
		using const_pointer		= value_type* const;
		using reference			= value_type&;
		using const_reference   = const value_type&;
		using iterator_category = std::random_access_iterator_tag;

		sparse_array() noexcept = default;
		~sparse_array()			= default;
		sparse_array(const sparse_array& other) noexcept
			: m_Dense(other.m_Dense), m_Reverse(other.m_Reverse), m_Sparse(other.m_Sparse){};
		sparse_array(sparse_array&& other) noexcept
			: m_Dense(std::move(other.m_Dense)), m_Reverse(std::move(other.m_Reverse)),
			  m_Sparse(std::move(other.m_Sparse)){};
		sparse_array& operator=(const sparse_array& other) noexcept
		{
			if(this != &other)
			{
				m_Dense   = other.m_Dense;
				m_Reverse = other.m_Reverse;
				m_Sparse  = other.m_Sparse;
			}
			return *this;
		}
		sparse_array& operator=(sparse_array&& other) noexcept
		{
			if(this != &other)
			{
				m_Dense   = std::move(other.m_Dense);
				m_Reverse = std::move(other.m_Reverse);
				m_Sparse  = std::move(other.m_Sparse);
			}
			return *this;
		}
		size_type size() const noexcept { return std::size(m_Dense); }
		size_type capacity() const noexcept { return std::size(m_Sparse) * chunks_size; }

		iterator begin() noexcept { return iterator{m_Dense.data(), m_Reverse.data()}; };
		iterator end() noexcept
		{
			return iterator{m_Dense.data() + m_Dense.size(), m_Reverse.data() + m_Reverse.size()};
		};

		reference operator[](index_type index)
		{
			auto sub_index = index;
			auto& chunk	= chunk_for(sub_index);

			if(!has(index))
			{
				chunk[sub_index] = (index_type)m_Dense.size();
				m_Reverse.emplace_back(index);
				m_Dense.emplace_back(value_type{});
			}
			return m_Dense[chunk[sub_index]];
		}

		reference at(index_type index)
		{
			auto sub_index = index;
			auto& chunk	= chunk_for(sub_index);

			if(!has(index))
			{
				chunk[sub_index] = (index_type)m_Dense.size();
				m_Reverse.emplace_back(index);
				m_Dense.emplace_back(value_type{});
			}
			return m_Dense[chunk[sub_index]];
		}

		const_reference at(index_type index) const noexcept
		{
			index_type sparse_index, chunk_index;
			chunk_info_for(index, sparse_index, chunk_index);
			assert_debug_break(m_Sparse[chunk_index][sparse_index] != std::numeric_limits<index_type>::max());
			return m_Dense[m_Sparse[chunk_index][sparse_index]];
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
			m_Reverse.emplace_back(index);
			auto& chunk  = chunk_for(index);
			chunk[index] = (index_type)m_Dense.size();
			m_Dense.emplace_back();
		}
		void insert(index_type index, const_reference value)
		{
			m_Reverse.emplace_back(index);
			auto& chunk = chunk_for(index);

			chunk[index] = (index_type)m_Dense.size();
			m_Dense.push_back(value);
		}

		template <typename ItF, typename ItL>
		void insert(index_type index, ItF&& first, ItL&& last)
		{
			auto first_index = index;
			auto last_index  = (index_type)std::distance(first, last) + index;
			auto end_index   = last_index;
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
					m_Sparse[i][x] = (index_type)m_Dense.size();
					m_Reverse.emplace_back(index++);
					m_Dense.emplace_back((*it));
					it = std::next(it);
				}
				first_index = 0;
			}

			for(auto x = 0u; x < last_index; ++x)
			{
				m_Sparse[last_index][x] = (index_type)m_Dense.size();
				m_Reverse.emplace_back(index++);
				m_Dense.emplace_back((*it));
				it = std::next(it);
			}
		}

		void emplace(index_type index, value_type&& value)
		{
			m_Reverse.emplace_back(index);
			auto& chunk = chunk_for(index);

			chunk[index] = (index_type)m_Dense.size();
			m_Dense.emplace_back(std::forward<value_type>(value));
		}

		constexpr value_type* try_get(index_type index) noexcept
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
				auto dense_index = (m_Sparse[chunk_index].size() > 0) ? m_Sparse[chunk_index][index]
																	  : std::numeric_limits<index_type>::max();

				if (dense_index == std::numeric_limits<index_type>::max())
					return nullptr;
				else
					return &m_Dense[dense_index];
			}
			return nullptr;
		}

		constexpr value_type const* try_get(index_type index) const noexcept
		{
			if (index < capacity())
			{
				index_type chunk_index;
				if constexpr (is_power_of_two)
				{
					chunk_index = (index - (index & mod_val)) / chunks_size;
					index = index & mod_val;
				}
				else
				{
					chunk_index = (index - (index % mod_val)) / chunks_size;
					index = index % mod_val;
				}
				auto dense_index = (m_Sparse[chunk_index].size() > 0) ? m_Sparse[chunk_index][index]
					: std::numeric_limits<index_type>::max();

				if (dense_index == std::numeric_limits<index_type>::max())
					return nullptr;
				else
					return &m_Dense[dense_index];
			}
			return nullptr;
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

		constexpr bool empty() const noexcept { return std::empty(m_Dense); };

		void* data() noexcept { return m_Dense.data(); };

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

				if(dense_index != m_Dense.size() - 1)
				{
					std::iter_swap(std::next(std::begin(m_Dense), dense_index), std::prev(std::end(m_Dense)));
					std::iter_swap(std::next(std::begin(m_Reverse), dense_index), std::prev(std::end(m_Reverse)));

					sparse_index = m_Reverse[dense_index];
					chunk_info_for(sparse_index, index, chunk_index);
					m_Sparse[chunk_index][index] = dense_index;
				}

				m_Dense.pop_back();
				m_Reverse.pop_back();
			}
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

			assert(std::all_of(&first, &last, [this](index_type i) { return has(i); }));

			if(last - first == m_Dense.size())
			{
				m_Dense.clear();
				m_Reverse.clear();
				m_Sparse.clear();
				return;
			}
			auto first_index = first;
			auto last_index  = last;
			index_type first_chunk;
			index_type last_chunk;
			chunk_info_for(first, first_index, first_chunk);
			chunk_info_for(last, last_index, last_chunk);

			index_type count{0};
			index_type processed{0};

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
					const auto dense_index{m_Sparse[i][x]};
					// if(dense_index == std::numeric_limits<index_type>::max())
					//	continue;
					++count;
					m_Sparse[i][x] = std::numeric_limits<index_type>::max();
					if(std::size(m_Dense) - dense_index == count) continue;
					std::iter_swap(std::next(std::begin(m_Dense), dense_index), std::prev(std::end(m_Dense), count));
					std::iter_swap(std::next(std::begin(m_Reverse), dense_index),
								   std::prev(std::end(m_Reverse), count));

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
					const auto dense_index{m_Sparse[last_chunk][x]};
					// if(dense_index == std::numeric_limits<index_type>::max())
					//	continue;
					++count;
					m_Sparse[last_chunk][x] = std::numeric_limits<index_type>::max();
					if(std::size(m_Dense) - dense_index == count) continue;
					std::iter_swap(std::next(std::begin(m_Dense), dense_index), std::prev(std::end(m_Dense), count));
					std::iter_swap(std::next(std::begin(m_Reverse), dense_index),
								   std::prev(std::end(m_Reverse), count));

					const auto new_index = m_Reverse[dense_index];

					// todo check if this optimization holds true
					// if(new_index > first && new_index < last)
					//	continue;
					index_type new_element_index, new_chunk_index;
					chunk_info_for(new_index, new_element_index, new_chunk_index);
					m_Sparse[new_chunk_index][new_element_index] = dense_index;
				}
			}

			m_Dense.resize(m_Dense.size() - count);
			m_Reverse.resize(m_Reverse.size() - count);
		}


		void reserve(size_t capacity)
		{
			if(capacity <= m_Reverse.capacity()) return;

			m_Reverse.reserve(capacity);
			m_Dense.reserve(capacity);
		}

		void clear()
		{
			m_Dense.clear();
			m_Reverse.clear();
			m_Sparse.clear();
		}

		psl::array_view<index_type> indices() const noexcept { return m_Reverse; }
		psl::array_view<value_type> dense() const noexcept { return m_Dense; }

	  private:
		inline psl::array<index_type>& chunk_for(index_type& index)
		{
			if(index >= capacity()) resize(index + 1);
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
				chunk_index   = (index - (index & mod_val)) / chunks_size;
				element_index = index & mod_val;
			}
			else
			{
				chunk_index   = (index - (index % mod_val)) / chunks_size;
				element_index = index % mod_val;
			}
		}

		psl::array<value_type> m_Dense;
		psl::array<index_type> m_Reverse;
		psl::array<psl::array<index_type>> m_Sparse;
	};
} // namespace psl