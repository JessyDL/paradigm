#pragma once

namespace core::ecs
{

	/// ----------------------------------------------------------------------------------------------
	/// jmp_buffer
	/// ----------------------------------------------------------------------------------------------

	template<typename T>
	class range
	{
	public:
		class iterator
		{
			friend class range;
			iterator(T* target, std::vector<size_t>::iterator index, std::vector<size_t>::const_iterator end) : target(target), index(index), end(end)
			{

			}
		public:
			T& operator*()
			{
				return *target;
			}

			bool operator!=(iterator other)
			{
				return index != other.index;
			}

			iterator operator++() const
			{
				std::uintptr_t jmp_size = (*std::next(index) - *index) * sizeof(T);
				return iterator{ (T*)((std::uintptr_t)target + jmp_size), std::next(index), end };
			}

			iterator& operator++()
			{
				auto next = std::next(index);
				if (end == next)
				{
					target = (T*)((std::uintptr_t)target + sizeof(T));
					index = next;
					return *this;
				}
				std::uintptr_t jmp_size = (*next - *index) * sizeof(T);
				target = (T*)((std::uintptr_t)target + jmp_size);
				index = next;
				return *this;
			}
		private:
			T* target;
			std::vector<size_t>::iterator index;
			std::vector<size_t>::const_iterator end;
		};

		range(std::vector<T>& target, const std::vector<size_t>& indices)
			: m_Target(&target), m_Indices(indices)
		{
			std::sort(m_Indices.begin(), m_Indices.end());
		};

		range() : m_Target(nullptr), m_Indices({})
		{

		}


		T& operator[](size_t index)
		{
			return (*m_Target)[m_Indices[index]];
		}

		iterator begin()
		{
			size_t index = *m_Indices.begin();
			return iterator{ &(*m_Target)[index], m_Indices.begin(), m_Indices.end() };
		}

		iterator end()
		{
			size_t index = *std::prev(m_Indices.end());
			return iterator{ (T*)((std::uintptr_t)&(*m_Target)[index] + sizeof(T)), m_Indices.end() , m_Indices.end() };
		}
	private:
		std::vector<T>* m_Target{ nullptr };
		std::vector<size_t> m_Indices;
	};

	enum access
	{
		READ_WRITE = 0,
		READ_ONLY = 1
	};

	class state;

	template<typename T,access access_level = access::READ_WRITE>
	struct vector
	{
		friend class core::ecs::state;
	public:
		struct iterator
		{
		public:
			constexpr iterator(T* data) noexcept : data(data) {};
			constexpr T& operator*()  noexcept
			{
				return *data;
			}

			constexpr const T& operator*() const noexcept
			{
				return *data;
			}

			constexpr bool operator!=(iterator other) const noexcept
			{
				return data != other.data;
			}

			constexpr iterator operator++() const noexcept
			{
				return iterator(data + 1);
			}

			constexpr iterator& operator++() noexcept
			{
				++data;
				return *this;
			}
		private:
			T* data;
		};

		constexpr const T& operator[](size_t index) const noexcept
		{			
			return *(data + index);
		}
		constexpr T& operator[](size_t index)  noexcept
		{			
			return *(data + index);
		}

		iterator begin() const noexcept
		{
			return iterator{data };
		}

		iterator end() const noexcept
		{
			return iterator{ tail };
		}

	private:
		T* data;
		T* tail;
	};

	template<typename T>
	struct vector<T, access::READ_ONLY>
	{
		friend class core::ecs::state;
	public:
		struct iterator
		{
		public:
			constexpr iterator(T* data) noexcept : data(data) {};
			constexpr const T& operator*() const noexcept
			{
				return *data;
			}

			constexpr bool operator!=(iterator other) const noexcept
			{
				return data != other.data;
			}

			constexpr iterator operator++() const noexcept 
			{
				return iterator(data + 1);
			}

			constexpr iterator& operator++() noexcept
			{
				++data;
				return *this;
			}
		private:
			T* data;
		};

		constexpr const T& operator[](size_t index) const noexcept
		{			
			return *(data + index);
		}

		iterator begin() const noexcept
		{
			return iterator{data };
		}

		iterator end() const noexcept
		{
			return iterator{ tail };
		}

	private:
		T* data;
		T* tail;
	};
}