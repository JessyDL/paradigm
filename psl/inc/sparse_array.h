#pragma once
#include "vector.h"
#include <array_view.h>

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

		sparse_array() noexcept					   = default;
		~sparse_array()							   = default;
		sparse_array(const sparse_array&) noexcept = default;
		sparse_array(sparse_array&&) noexcept	  = default;
		sparse_array& operator=(const sparse_array&) noexcept = default;
		sparse_array& operator=(sparse_array&&) noexcept = default;

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

		void insert(index_type index, const_reference value)
		{
			auto& chunk = chunk_for(index);

			chunk[index] = (index_type)m_Dense.size();
			m_Reverse.emplace_back(index);
			m_Dense.push_back(value);
		}

		void emplace(index_type index, value_type&& value)
		{
			auto& chunk = chunk_for(index);

			chunk[index] = (index_type)m_Dense.size();
			m_Reverse.emplace_back(index);
			m_Dense.emplace_back(std::forward<value_type>(value));
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
				if(m_Sparse[chunk_index].size() > 0)
				{
					return m_Sparse[chunk_index][index] != std::numeric_limits<index_type>::max();
				}
			}
			return false;
		}

		constexpr bool empty() const noexcept { return std::empty(m_Dense); };

		void* data() noexcept { return m_Dense.data(); };
		void* cdata() const noexcept { return m_Dense.data(); };

		void erase(index_type index)
		{
			if(has(index))
			{
				auto sparse_index = index;
				auto& chunk		  = chunk_for(index);

				auto dense_index = chunk[index];
				std::iter_swap(std::next(std::begin(m_Dense), dense_index), std::prev(std::end(m_Dense)));
				std::iter_swap(std::next(std::begin(m_Reverse), dense_index), std::prev(std::end(m_Reverse)));
				m_Dense.resize(m_Dense.size() - 1);
				m_Reverse.resize(m_Reverse.size() - 1);

				chunk[index] = std::numeric_limits<index_type>::max();

				index		 = m_Reverse[dense_index];
				chunk		 = chunk_for(index);
				chunk[index] = sparse_index;
			}
		}

	  private:
		psl::array<index_type>& chunk_for(index_type& index)
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

		psl::array<value_type> m_Dense;
		psl::array<index_type> m_Reverse;
		psl::array<psl::array<index_type>> m_Sparse;
	};
} // namespace psl