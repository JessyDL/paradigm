#pragma once
#include "../details/staged_sparse_array.hpp"
#include "../details/staged_sparse_memory_region.hpp"
#include "../entity.hpp"
#include "component_key.hpp"
#include "psl/array_view.hpp"
#include "psl/ecs/component_traits.hpp"
#include "psl/memory/sparse_array.hpp"
#include "psl/sparse_array.hpp"
#include "psl/sparse_indice_array.hpp"
#include "psl/static_array.hpp"
#include <functional>
#include <numeric>

namespace psl {
template <typename... Ts>
class pack_view;
}

namespace psl::ecs::details {
/// \brief implementation detail that stores the component data
///
/// This class serves as a base to the actual component storage.
/// It contains some primitive functionality itself, but mostly serves
/// as a safer way of accessing the component data from the state.
/// \warning this should never be used by anything other than the psl::ecs::state_t
/// The 'public' API is not safe to use.
class component_container_t {
  public:
	component_container_t(const component_key_t& id, size_t size, size_t alignment);

	component_container_t(const component_container_t& other) = delete;
	component_container_t(component_container_t&& other);
	virtual ~component_container_t()									 = default;
	component_container_t& operator=(const component_container_t& other) = delete;
	component_container_t& operator=(component_container_t&& other);

	bool is_tag() const noexcept;

	void add(psl::array_view<entity_t> entities, void* data = nullptr, bool repeat = false) {
		add_impl(entities, data, repeat);
	}
	void add(entity_t entity, void* data = nullptr) { add_impl(entity, data); }
	void add(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
			 void* data	 = nullptr,
			 bool repeat = false) {
		add_impl(entities, data, repeat);
	}
	void destroy(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) {
		remove_impl(entities);
	};
	void destroy(psl::array_view<entity_t> entities) noexcept { remove_impl(entities); }
	void destroy(entity_t entity) noexcept { remove_impl(entity); }
	virtual void* data() noexcept			  = 0;
	virtual void* const data() const noexcept = 0;
	inline bool has(entity_t entity, stage_range_t stage = stage_range_t::ALL) { return has_impl(entity, stage); }
	inline bool has_component(entity_t entity) const noexcept { return has_impl(entity, stage_range_t::ALIVE); }
	inline bool has_added(entity_t entity) const noexcept { return has_impl(entity, stage_range_t::ADDED); }
	inline bool has_removed(entity_t entity) const noexcept { return has_impl(entity, stage_range_t::REMOVED); }
	virtual bool has_storage_for(entity_t entity) const noexcept = 0;
	inline psl::array_view<entity_t> entities(bool include_removed = false) const noexcept {
		return entities_impl((include_removed) ? stage_range_t::ALL : stage_range_t::ALIVE);
	}

	virtual void* get_if(entity_t entity, stage_range_t stage = stage_range_t::ALL) { return nullptr; }

	inline void purge() noexcept { purge_impl(); }
	constexpr component_key_t const& id() const noexcept { return m_ID; }

	inline psl::array_view<entity_t> added_entities() const noexcept { return entities_impl(stage_range_t::ADDED); };
	inline psl::array_view<entity_t> removed_entities() const noexcept {
		return entities_impl(stage_range_t::REMOVED);
	};

	virtual entity_t::size_type* write_memory_location_offsets_for(psl::array_view<entity_t> entities,
																   entity_t::size_type* target) const noexcept {
		return target;
	}
	virtual size_t copy_to(psl::array_view<entity_t> entities, void* destination) const noexcept { return 0; };
	virtual size_t copy_from(psl::array_view<entity_t> entities, void* source, bool repeat = false) noexcept {
		return 0;
	};

	inline size_t component_size() const noexcept { return m_Size; };
	size_t size(bool include_removed = false) const noexcept {
		return entities_impl((include_removed) ? stage_range_t::ALL : stage_range_t::ALIVE).size();
	}
	size_t alignment() const noexcept { return m_Alignment; }

	void set(entity_t entity, void* data) noexcept { set_impl(entity, data); }
	virtual bool should_serialize() const noexcept { return false; }
	virtual bool should_serialize(bool value) noexcept { return false; }

	virtual void remap(const psl::sparse_array<entity_t::size_type>& mapping,
					   std::function<bool(entity_t)> pred) noexcept = 0;
	virtual bool merge(const component_container_t& other) noexcept = 0;
	virtual void clear()											= 0;

  protected:
	virtual void purge_impl() noexcept																		= 0;
	virtual void add_impl(entity_t entity, void* data)														= 0;
	virtual void add_impl(psl::array_view<entity_t> entities, void* data, bool repeat)						= 0;
	virtual void add_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
						  void* data,
						  bool repeat)																		= 0;
	virtual psl::array_view<entity_t> entities_impl(stage_range_t stage) const noexcept						= 0;
	virtual void set_impl(entity_t entity, void* data) noexcept												= 0;
	virtual void remove_impl(entity_t entity)																= 0;
	virtual void remove_impl(psl::array_view<entity_t> entities)											= 0;
	virtual void remove_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) = 0;
	virtual bool has_impl(entity_t entity, stage_range_t stage) const noexcept								= 0;

  protected:
	component_key_t m_ID;
	size_t m_Size;
	size_t m_Alignment;
};

template <typename T>
struct component_container_type_for;

template <typename T>
class component_container_typed_t final : public component_container_t {
	static_assert(std::is_same_v<typename component_container_type_for<T>::type, component_container_typed_t<T>>,
				  "The complex container should only be used for complex types");

  public:
	component_container_typed_t()
		: component_container_t(details::component_key_t::generate<T>(), sizeof(T), std::alignment_of_v<T>) {};
	~component_container_typed_t() override = default;
	auto& entity_data() noexcept { return m_Entities; };


	void* data() noexcept override { return m_Entities.data(); }
	void* const data() const noexcept override { return m_Entities.data(); }

	bool has_storage_for(entity_t entity) const noexcept override {
		return m_Entities.has(static_cast<entity_t::size_type>(entity), stage_range_t::ALL);
	}

	void* get_if(entity_t entity, stage_range_t stage = stage_range_t::ALL) override {
		return m_Entities.addressof_if(static_cast<entity_t::size_type>(entity), stage);
	}

	entity_t::size_type* write_memory_location_offsets_for(psl::array_view<entity_t> entities,
														   entity_t::size_type* destination) const noexcept override {
		for(auto e : entities) {
			*destination = m_Entities.dense_index_for(static_cast<entity_t::size_type>(e), stage_range_t::ALL);
			++destination;
		}
		return destination;
	}

	size_t copy_to(psl::array_view<entity_t> entities, void* destination) const noexcept override {
		psl_assert((std::uintptr_t)destination % alignment() == 0, "pointer has to be aligned");
		T* dest = (T*)destination;
		for(auto e : entities) {
			std::memcpy(dest, m_Entities.addressof(static_cast<entity_t::size_type>(e), stage_range_t::ALL), sizeof(T));
			++dest;
		}
		return entities.size() * sizeof(T);
	}
	size_t copy_from(psl::array_view<entity_t> entities, void* source, bool repeat) noexcept override {
		psl_assert((std::uintptr_t)source % alignment() == 0, "pointer has to be aligned");
		T* src = (T*)source;
		if(repeat) {
			for(auto e : entities) {
				std::memcpy(
				  (void*)&(m_Entities.at(static_cast<entity_t::size_type>(e), stage_range_t::ALL)), src, sizeof(T));
			}
		} else {
			for(auto e : entities) {
				std::memcpy(
				  (void*)&(m_Entities.at(static_cast<entity_t::size_type>(e), stage_range_t::ALL)), src, sizeof(T));
				++src;
			}
		}
		return sizeof(T) * entities.size();
	};

	void remap(const psl::sparse_array<entity_t::size_type>& mapping,
			   std::function<bool(entity_t)> pred) noexcept override {
		m_Entities.remap(mapping, [pred](entity_t::size_type e) -> bool { return pred(entity_t {e}); });
	}
	bool merge(const component_container_t& other) noexcept override {
		if(other.id() != id())
			return false;

		component_container_typed_t<T>* other_ptr = (component_container_typed_t<T>*)(&other);
		m_Entities.merge(other_ptr->m_Entities);
		return true;
	}

	void set(entity_t e, const T& data) noexcept {
		m_Entities.at(static_cast<entity_t::size_type>(e), stage_range_t::ALL) = data;
	}

  protected:
	void set_impl(entity_t entity, void* data) noexcept override {
		m_Entities.at(static_cast<entity_t::size_type>(entity), stage_range_t::ALL) = *(T*)data;
	}
	psl::array_view<entity_t> entities_impl(stage_range_t stage) const noexcept override {
		auto indices = m_Entities.indices(stage);
		return psl::array_view<entity_t>(reinterpret_cast<entity_t*>(indices.data()), indices.size());
	}
	void add_impl(psl::array_view<entity_t> entities, void* data, bool repeat) override {
		m_Entities.reserve(m_Entities.size(stage_range_t::ALL) + entities.size());
		T* source = (T*)data;
		if(data == nullptr) {
			for(size_t i = 0; i < entities.size(); ++i) {
				m_Entities.insert(static_cast<entity_t::size_type>(entities[i]));
			}
		} else if(repeat) {
			for(size_t i = 0; i < entities.size(); ++i) {
				m_Entities.insert(static_cast<entity_t::size_type>(entities[i]), *source);
			}
		} else {
			for(size_t i = 0; i < entities.size(); ++i) {
				m_Entities.insert(static_cast<entity_t::size_type>(entities[i]), *source);
				++source;
			}
		}
	}
	void add_impl(entity_t entity, void* data) override {
		if(data == nullptr)
			m_Entities.insert(static_cast<entity_t::size_type>(entity));
		else
			m_Entities.insert(static_cast<entity_t::size_type>(entity), *(T*)data);
	}
	void add_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
				  void* data,
				  bool repeat) override {
		auto count = std::accumulate(std::begin(entities),
									 std::end(entities),
									 size_t {0},
									 [](size_t sum, const std::pair<entity_t::size_type, entity_t::size_type>& r) {
										 return sum + (r.second - r.first);
									 });

		m_Entities.reserve(m_Entities.size(stage_range_t::ALL) + count);
		T* source = (T*)data;
		if(data == nullptr) {
			for(auto range : entities) {
				for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e);
			}
		} else if(repeat) {
			for(auto range : entities) {
				for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e, *source);
			}
		} else {
			for(auto range : entities) {
				for(auto e = range.first; e < range.second; ++e) {
					m_Entities.insert(e, *source);
					++source;
				}
			}
		}
	}
	void purge_impl() noexcept override { m_Entities.promote(); }

	void remove_impl(entity_t entity) override { m_Entities.erase(static_cast<entity_t::size_type>(entity)); }
	void remove_impl(psl::array_view<entity_t> entities) override {
		for(size_t i = 0; i < entities.size(); ++i) m_Entities.erase(static_cast<entity_t::size_type>(entities[i]));
	}
	void remove_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) override {
		for(auto range : entities) {
			for(auto i = range.first; i < range.second; ++i) m_Entities.erase(i);
		}
	}
	bool has_impl(entity_t entity, stage_range_t stage) const noexcept override {
		return m_Entities.has(static_cast<entity_t::size_type>(entity), stage);
	}

	void clear() override { m_Entities.clear(); }

  private:
	details::staged_sparse_array<T, entity_t::size_type> m_Entities;
};

class component_container_flag_t : public component_container_t {
  public:
	component_container_flag_t(const psl::ecs::details::component_key_t& key, bool serializable = false)
		: component_container_t(std::move(key), 0, 0) {};
	~component_container_flag_t() override = default;

	void* data() noexcept override { return nullptr; }
	void* const data() const noexcept override { return nullptr; }

	bool has_storage_for(entity_t entity) const noexcept override {
		return m_Entities.has(static_cast<entity_t::size_type>(entity), stage_range_t::ALL);
	}

	void remap(const psl::sparse_array<entity_t::size_type>& mapping,
			   std::function<bool(entity_t)> pred) noexcept override {
		m_Entities.remap(mapping, [pred](entity_t::size_type entity) { return pred(entity_t {entity}); });
	}

	bool merge(const component_container_t& other) noexcept override {
		if(other.id() != id())
			return false;

		component_container_flag_t* other_ptr = (component_container_flag_t*)(&other);
		m_Entities.merge(other_ptr->m_Entities);
		return true;
	}

	bool should_serialize() const noexcept override { return m_Serializable; }
	bool should_serialize(bool value) noexcept override {
		m_Serializable = value;
		return true;
	}

  protected:
	void set_impl(entity_t entity, void* data) noexcept override {};
	psl::array_view<entity_t> entities_impl(stage_range_t stage) const noexcept override {
		auto indices = m_Entities.indices(stage);
		return psl::array_view<entity_t>(reinterpret_cast<entity_t*>(indices.data()), indices.size());
	}
	void add_impl(psl::array_view<entity_t> entities, void* data, bool repeat) override {
		m_Entities.reserve(m_Entities.size(stage_range_t::ALL) + entities.size());
		for(auto e : entities) m_Entities.insert(static_cast<entity_t::size_type>(e));
	}
	void add_impl(entity_t entity, void* data) override { m_Entities.insert(static_cast<entity_t::size_type>(entity)); }
	void add_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
				  void* data,
				  bool repeat) override {
		auto count = std::accumulate(std::begin(entities),
									 std::end(entities),
									 size_t {0},
									 [](size_t sum, const std::pair<entity_t::size_type, entity_t::size_type>& r) {
										 return sum + (r.second - r.first);
									 });

		m_Entities.reserve(m_Entities.size(stage_range_t::ALL) + count);
		for(auto range : entities) {
			for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e);
		}
	}
	void purge_impl() noexcept override { m_Entities.promote(); }

	void remove_impl(entity_t entity) override { m_Entities.erase(static_cast<entity_t::size_type>(entity)); }
	void remove_impl(psl::array_view<entity_t> entities) override {
		for(size_t i = 0; i < entities.size(); ++i) m_Entities.erase(static_cast<entity_t::size_type>(entities[i]));
	}
	void remove_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) override {
		for(auto range : entities) {
			for(auto i = range.first; i < range.second; ++i) m_Entities.erase(i);
		}
	}
	bool has_impl(entity_t entity, stage_range_t stage) const noexcept override {
		return m_Entities.has(static_cast<entity_t::size_type>(entity), stage);
	}

	void clear() override {
		m_Entities.clear();
		m_Serializable = false;
	}

  private:
	details::staged_sparse_array<void, entity_t::size_type> m_Entities;
	bool m_Serializable {false};
};

class component_container_untyped_t : public component_container_t {
	using stage_range_t = details::stage_range_t;

  public:
	component_container_untyped_t(const psl::ecs::details::component_key_t& key,
								  size_t size,
								  size_t alignment,
								  bool serializable = false)
		: component_container_t(std::move(key), size, alignment), m_Entities(size), m_Serializable(serializable) {};
	~component_container_untyped_t() override = default;
	auto& entity_data() noexcept { return m_Entities; };


	void* data() noexcept override { return m_Entities.data(); }
	void* const data() const noexcept override { return m_Entities.data(); }

	bool has_storage_for(entity_t entity) const noexcept override {
		return m_Entities.has(static_cast<entity_t::size_type>(entity), stage_range_t::ALL);
	}

	void* get_if(entity_t entity, stage_range_t stage = stage_range_t::ALL) override {
		return m_Entities.addressof_if(static_cast<entity_t::size_type>(entity), stage);
	}

	entity_t::size_type* write_memory_location_offsets_for(psl::array_view<entity_t> entities,
														   entity_t::size_type* destination) const noexcept override {
		for(auto e : entities) {
			*destination = m_Entities.dense_index_for(static_cast<entity_t::size_type>(e), stage_range_t::ALL);
			++destination;
		}
		return destination;
	}

	size_t copy_to(psl::array_view<entity_t> entities, void* destination) const noexcept override {
		psl_assert((std::uintptr_t)destination % alignment() == 0, "pointer has to be aligned");
		std::byte* dest = (std::byte*)destination;
		for(auto e : entities) {
			std::memcpy(dest, m_Entities.addressof(static_cast<entity_t::size_type>(e), stage_range_t::ALL), m_Size);
			dest += m_Size;
		}
		return entities.size() * m_Size;
	}
	size_t copy_from(psl::array_view<entity_t> entities, void* source, bool repeat) noexcept override {
		psl_assert((std::uintptr_t)source % alignment() == 0, "pointer has to be aligned");
		std::byte* src = (std::byte*)source;
		if(repeat) {
			for(auto e : entities) {
				std::memcpy(m_Entities.addressof(static_cast<entity_t::size_type>(e), stage_range_t::ALL), src, m_Size);
			}
		} else {
			for(auto e : entities) {
				std::memcpy(m_Entities.addressof(static_cast<entity_t::size_type>(e), stage_range_t::ALL), src, m_Size);
				src += m_Size;
			}
		}
		return m_Size * entities.size();
	};

	void remap(const psl::sparse_array<entity_t::size_type>& mapping,
			   std::function<bool(entity_t)> pred) noexcept override {
		m_Entities.remap(mapping, [pred](entity_t::size_type entity) { return pred(entity_t {entity}); });
	}

	bool merge(const component_container_t& other) noexcept override {
		if(other.id() != id())
			return false;

		component_container_untyped_t* other_ptr = (component_container_untyped_t*)(&other);
		return m_Entities.merge(other_ptr->m_Entities).success;
	}

	template <typename T>
	void set(entity_t e, const T& data) noexcept {
		m_Entities.template at<T>(static_cast<entity_t::size_type>(e), stage_range_t::ALL) = data;
	}

	bool should_serialize() const noexcept override { return m_Serializable; }
	bool should_serialize(bool value) noexcept override {
		m_Serializable = value;
		return true;
	}

	void clear() override {
		m_Entities.clear();
		m_Serializable = false;
	}

  protected:
	void set_impl(entity_t entity, void* data) noexcept override {
		auto* ptr = m_Entities.addressof(static_cast<entity_t::size_type>(entity), stage_range_t::ALL);
		std::memcpy(ptr, data, m_Size);
	}
	psl::array_view<entity_t> entities_impl(stage_range_t stage) const noexcept override {
		auto indices = m_Entities.indices(stage);
		return psl::array_view<entity_t>(reinterpret_cast<entity_t*>(indices.data()), indices.size());
	}

	void add_impl(psl::array_view<entity_t> entities, void* data, bool repeat) override {
		std::byte* source = (std::byte*)data;
		if(data == nullptr) {
			for(size_t i = 0; i < entities.size(); ++i) {
				m_Entities.insert(static_cast<entity_t::size_type>(entities[i]));
			}
		} else if(repeat) {
			for(size_t i = 0; i < entities.size(); ++i) {
				auto ptr = m_Entities.get_or_insert(static_cast<entity_t::size_type>(entities[i]));
				memcpy(ptr, source, m_Size);
			}
		} else {
			for(size_t i = 0; i < entities.size(); ++i) {
				auto ptr = m_Entities.get_or_insert(static_cast<entity_t::size_type>(entities[i]));
				memcpy(ptr, source, m_Size);
				source += m_Size;
			}
		}
	}
	void add_impl(entity_t entity, void* data) override {
		auto ptr = m_Entities.get_or_insert(static_cast<entity_t::size_type>(entity));

		if(data != nullptr)
			memcpy(ptr, data, m_Size);
	}

	void add_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
				  void* data,
				  bool repeat) override {
		auto count = std::accumulate(std::begin(entities),
									 std::end(entities),
									 size_t {0},
									 [](size_t sum, const std::pair<entity_t::size_type, entity_t::size_type>& r) {
										 return sum + (r.second - r.first);
									 });

		std::byte* source = (std::byte*)data;
		if(data == nullptr) {
			for(auto range : entities) {
				for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e);
			}
		} else if(repeat) {
			for(auto range : entities) {
				for(auto e = range.first; e < range.second; ++e) {
					auto ptr = m_Entities.get_or_insert(e);
					memcpy(ptr, source, m_Size);
				}
			}
		} else {
			for(auto range : entities) {
				for(auto e = range.first; e < range.second; ++e) {
					auto ptr = m_Entities.get_or_insert(e);
					memcpy(ptr, source, m_Size);
					source += m_Size;
				}
			}
		}
	}
	void purge_impl() noexcept override { m_Entities.promote(); }

	void remove_impl(entity_t entity) override { m_Entities.erase(static_cast<entity_t::size_type>(entity)); }
	void remove_impl(psl::array_view<entity_t> entities) override {
		for(size_t i = 0; i < entities.size(); ++i) m_Entities.erase(static_cast<entity_t::size_type>(entities[i]));
	}
	void remove_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) override {
		for(auto range : entities) {
			for(auto i = range.first; i < range.second; ++i) m_Entities.erase(i);
		}
	}
	bool has_impl(entity_t entity, stage_range_t stage) const noexcept override {
		return m_Entities.has(static_cast<entity_t::size_type>(entity), stage);
	}

  private:
	details::staged_sparse_memory_region_t m_Entities;
	bool m_Serializable {false};
};

template <typename T>
struct component_container_type_for {
	using type = std::conditional_t<
	  IsComponentFlagType<T>,
	  component_container_flag_t,
	  std::conditional_t<IsComponentTrivialType<T>, component_container_untyped_t, component_container_typed_t<T>>>;
};

template <typename T>
using component_container_type_for_t = typename component_container_type_for<T>::type;

template <typename T>
inline auto instantiate_component_container() -> std::unique_ptr<component_container_t> {
	using return_type = component_container_type_for_t<T>;

	// mostly future proofed check, this basically checks that either serialization is turned off, _or_ the type
	// supports it.
	static_assert(IsComponentTypeSerializable<T> || !component_traits<T>::serializable,
				  "Unsupported. Component type cannot support serialization, please fix your `component_traits<T>` "
				  "specialization for this component type");

	if constexpr(IsComponentFlagType<T>) {
		return std::unique_ptr<component_container_t>(std::make_unique<component_container_flag_t>(
		  psl::ecs::details::component_key_t::generate<T>(), component_traits<T>::serializable));
	} else if constexpr(IsComponentTrivialType<T>) {
		return std::unique_ptr<component_container_t>(
		  std::make_unique<component_container_untyped_t>(psl::ecs::details::component_key_t::generate<T>(),
														  sizeof(T),
														  std::alignment_of_v<T>,
														  component_traits<T>::serializable));
	} else if constexpr(IsComponentComplexType<T>) {
		return std::unique_ptr<component_container_t>(std::make_unique<component_container_typed_t<T>>());
	}
}

inline auto instantiate_component_container(const psl::ecs::details::component_key_t& key,
											size_t size,
											size_t alignment,
											bool should_serialize) -> std::unique_ptr<component_container_t> {
	switch(key.type()) {
	case psl::ecs::component_type::TRIVIAL:
		return std::make_unique<component_container_untyped_t>(std::move(key), size, alignment, should_serialize);
	case psl::ecs::component_type::FLAG:
		return std::make_unique<component_container_flag_t>(std::move(key), should_serialize);
	case psl::ecs::component_type::COMPLEX:
		throw std::runtime_error("Cannot runtime instantiate a complex type without type information");
	}
	psl::unreachable("invalid key value for component type");
}

template <typename T>
inline auto cast_component_container(component_container_t* container) -> component_container_type_for_t<T>* {
	return reinterpret_cast<component_container_type_for_t<T>*>(container);
}
}	 // namespace psl::ecs::details

namespace std {
template <>
struct hash<psl::ecs::details::component_container_t> {
	std::size_t operator()(psl::ecs::details::component_container_t const& ci) const noexcept {
		return std::hash<psl::ecs::details::component_key_t> {}(ci.id());
	}
};
}	 // namespace std
