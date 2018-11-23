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
	
	class state;

	template<typename T>
	struct vector
	{
		friend class core::ecs::state;
		using storage_type = std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
	public:
		using component_t = std::remove_pointer_t<std::remove_reference_t<T>>;
		static constexpr const bool is_read_only{std::is_const_v<T>};

		struct iterator
		{
		public:
			constexpr iterator(component_t* data) noexcept : data(data) {};

			constexpr component_t& operator*()  noexcept
			{
				return *data;
			}

			constexpr const component_t& operator*() const noexcept
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
			storage_type* data;
		};
		vector() noexcept = default;
		~vector() noexcept = default;
		vector(const vector&) noexcept = default;
		vector(vector&&) noexcept = default;
		vector& operator=(const vector&) noexcept = default;
		vector& operator=(vector&&) noexcept = default;

		constexpr const component_t& operator[](size_t index) const noexcept
		{
			return *(data + index);
		}

		constexpr component_t& operator[](size_t index) noexcept
		{
			return *(data + index);
		}

		iterator begin() const noexcept
		{
			return iterator{data};
		}

		iterator end() const noexcept
		{
			return iterator{tail};
		}
		constexpr size_t size() const noexcept
		{
			return (tail - data);
		}

	private:
		storage_type* data;
		storage_type* tail;
	};

	template<typename T>
	struct vector<const T>
	{
		friend class core::ecs::state;
		using storage_type = std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
	public:
		using component_t = std::remove_pointer_t<std::remove_reference_t<T>>;
		static constexpr const bool is_read_only{true};

		struct iterator
		{
		public:
			constexpr iterator(component_t* data) noexcept : data(data) {};
			constexpr const component_t& operator*() const noexcept
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
			storage_type* data;
		};
		vector() noexcept = default;
		~vector() noexcept = default;
		vector(const vector&) noexcept = default;
		vector(vector&&) noexcept = default;
		vector& operator=(const vector&) noexcept = default;
		vector& operator=(vector&&) noexcept = default;

		constexpr const component_t& operator[](size_t index) const noexcept
		{
			return *(data + index);
		}

		iterator begin() const noexcept
		{
			return iterator{data};
		}

		iterator end() const noexcept
		{
			return iterator{tail};
		}
		constexpr size_t size() const noexcept
		{
			return (tail - data);
		}

	private:
		storage_type* data;
		storage_type* tail;
	};


	template<>
	struct vector<core::ecs::entity>
	{
		friend class core::ecs::state;
	public:
		struct iterator
		{
		public:
			constexpr iterator(core::ecs::entity* data) noexcept : data(data) {};
			constexpr const core::ecs::entity& operator*() const noexcept
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
			core::ecs::entity* data;
		};
		vector() noexcept = default;
		~vector() noexcept = default;
		vector(const vector&) noexcept = default;
		vector(vector&&) noexcept = default;
		vector& operator=(const vector&) noexcept = default;
		vector& operator=(vector&&) noexcept = default;

		constexpr const core::ecs::entity& operator[](size_t index) const noexcept
		{
			return *(data + index);
		}

		iterator begin() const noexcept
		{
			return iterator{data};
		}

		iterator end() const noexcept
		{
			return iterator{tail};
		}
		constexpr size_t size() const noexcept
		{
			return (tail - data);
		}

	private:
		core::ecs::entity* data;
		core::ecs::entity* tail;
	};
}