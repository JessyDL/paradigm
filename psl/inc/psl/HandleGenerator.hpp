#pragma once
#include "psl/IDGenerator.hpp"

namespace psl
{
template <class T, class precision = uint32_t>
struct handle
{
	friend struct generator;

	struct generator
	{
		generator() : m_Generator(), m_AllHandles(65535), m_Items(65535) {};

		generator(psl::IDGenerator<precision>& generator) : m_Generator(generator), m_AllHandles(4), m_Items(4) {};
		generator(precision limit) : m_Generator(limit), m_AllHandles(4), m_Items(4) {};

		handle& create(const T& target)
		{
			precision id;
			if(m_Generator.create(id))
			{
				if(id >= m_AllHandles.capacity())
				{
					m_AllHandles.resize(m_AllHandles.size() * 2);
					m_Items.resize(m_AllHandles.size() * 2);
				}
				m_AllHandles[id].m_ID		 = id;
				m_AllHandles[id].m_Generator = this;
				m_Items[id]					 = target;
			}

			return m_AllHandles[id];
		}

		bool destroy(const handle& handle) { return m_Generator.destroy(handle.ID()); }

		void set(handle& handle, const T& val) { m_Items[handle.ID()] = val; };

		void set(precision id, const T& val) { m_Items[id] = val; };

		T& get(handle& handle) { return m_Items[handle.ID()]; };

		T& get(precision id) { return m_Items[id]; };

	  private:
		psl::IDGenerator<precision> m_Generator;
		std::vector<handle> m_AllHandles;
		std::vector<T> m_Items;
	};

	handle() : m_Generator(nullptr) {};
	handle(const precision& id, Generator* generator) : m_ID(id), m_Generator(generator) {};

	bool operator==(const handle& other) { return (m_ID == other.m_ID); }

	bool operator!=(const handle& other) { return (m_ID != other.m_ID); }

	precision ID() const { return m_ID; };
	bool IsValid() { return m_Generator != nullptr; };

	const T& value() const { return m_Generator->get(m_ID); }

	T* operator->() const { return &m_Generator->get(m_ID); }

  private:
	precision m_ID;
	generator* m_Generator;
};


template <class T, class precision>
struct handle<T*, precision>
{
	friend struct generator;

	struct generator
	{
		generator() : m_Generator(), m_AllHandles(65535), m_Items(65535) {};

		generator(psl::IDGenerator<precision>& generator) : m_Generator(generator), m_AllHandles(4), m_Items(4) {};
		generator(precision limit) : m_Generator(limit), m_AllHandles(4), m_Items(4) {};

		handle& create(T* target)
		{
			precision id;
			if(m_Generator.create(id))
			{
				if(id >= m_AllHandles.capacity())
				{
					m_AllHandles.resize(m_AllHandles.size() * 2);
					m_Items.resize(m_AllHandles.size() * 2);
				}
				m_AllHandles[id].m_ID		 = id;
				m_AllHandles[id].m_Generator = this;
				m_Items[id]					 = target;
				++m_Used;
			}

			return m_AllHandles[id];
		}

		bool destroy(const handle& handle)
		{
			--m_Used;
			return m_Generator.destroy(handle.ID());
		}

		void set(handle& handle, T* val) { m_Items[handle.ID()] = val; };

		void set(precision id, T* val) { m_Items[id] = val; };

		T* get(handle& handle) const { return m_Items[handle.ID()]; };

		T* get(precision id) const { return m_Items[id]; };

		const std::vector<handle>& all() const { return m_AllHandles; };
		const precision& Used() { return m_Used; }

	  private:
		psl::IDGenerator<precision> m_Generator;
		std::vector<handle> m_AllHandles;
		std::vector<T*> m_Items;

		precision m_Used;
	};

	handle() : m_Generator(nullptr) {};
	handle(const precision& id, Generator* generator) : m_ID(id), m_Generator(generator) {};

	bool operator==(const handle& other) { return (m_ID == other.m_ID); }

	bool operator!=(const handle& other) { return (m_ID != other.m_ID); }

	precision ID() const { return m_ID; };
	bool IsValid() const { return m_Generator != nullptr; };

	T* value() const { return m_Generator->get(m_ID); }

	T* operator->() const { return m_Generator->get(m_ID); }

  private:
	precision m_ID;
	generator* m_Generator;
};
}	 // namespace psl
