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
}