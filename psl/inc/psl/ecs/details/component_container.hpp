#pragma once
#include "component_key.hpp"
#include "psl/array_view.hpp"
#include "psl/ecs/component_traits.hpp"
#include "psl/ecs/details/staged_sparse_array.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/memory/sparse_array.hpp"
#include "psl/sparse_array.hpp"
#include "psl/static_array.hpp"
#include <functional>
#include <numeric>

namespace psl {
template <typename... Ts>
class pack_view;
}

namespace psl::ecs {
class state_t;
}

namespace psl::ecs::details {
class components_cache_t;

/// \brief collects all handy information we need of a component both for identification purposes, and for safe de/serialization
struct component_type_info_t {
	component_key_t id;
	size_t version;
	size_t size;
	size_t alignment;
	psl::ecs::component_mutability_behaviour_t mutability;
	bool serializable;
};

/// \brief implementation detail that stores the component data
///
/// This class serves as a base to the actual component storage.
/// It contains some primitive functionality itself, but mostly serves
/// as a safer way of accessing the component data from the state.
/// \warning this should never be used by anything other than the psl::ecs::state_t
/// The 'public' API is not safe to use.
class component_container_t {
	friend class psl::ecs::state_t;
	friend class psl::ecs::details::components_cache_t;

  public:
	component_container_t(component_type_info_t info);

	component_container_t(const component_container_t& other) = delete;
	component_container_t(component_container_t&& other);
	virtual ~component_container_t()									 = default;
	component_container_t& operator=(const component_container_t& other) = delete;
	component_container_t& operator=(component_container_t&& other);

	bool is_flag() const noexcept;

	void add(psl::array_view<entity_t> entities, void* data = nullptr, bool repeat = false) {
		add_impl(entities, data, repeat);
	}
	void add(entity_t entity, void* data = nullptr) {
		add_impl(entity, data);
	}
	void add(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
			 void* data	 = nullptr,
			 bool repeat = false) {
		add_impl(entities, data, repeat);
	}
	void destroy(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) {
		remove_impl(entities);
	};
	void destroy(psl::array_view<entity_t> entities) noexcept {
		remove_impl(entities);
	}
	void destroy(entity_t entity) noexcept {
		remove_impl(entity);
	}
	virtual void* data() noexcept			  = 0;
	virtual void* const data() const noexcept = 0;
	inline bool has(entity_t entity, stage_range_t stage = stage_range_t::ALL) {
		return has_impl(entity, stage);
	}
	inline bool has_component(entity_t entity) const noexcept {
		return has_impl(entity, stage_range_t::ALIVE);
	}
	inline bool has_added(entity_t entity) const noexcept {
		return has_impl(entity, stage_range_t::ADDED);
	}
	inline bool has_removed(entity_t entity) const noexcept {
		return has_impl(entity, stage_range_t::REMOVED);
	}

	inline entity_t*
	remove_if_has(entity_t* begin, entity_t* end, stage_range_t stage = stage_range_t::ALL) const noexcept {
		return remove_if_has_impl(begin, end, stage, false);
	}
	inline entity_t*
	remove_if_has_not(entity_t* begin, entity_t* end, stage_range_t stage = stage_range_t::ALL) const noexcept {
		return remove_if_has_impl(begin, end, stage, true);
	}

	virtual bool has_storage_for(entity_t entity) const noexcept = 0;
	inline psl::array_view<entity_t> entities(bool include_removed = false) const noexcept {
		return entities_impl((include_removed) ? stage_range_t::ALL : stage_range_t::ALIVE);
	}
	inline psl::array_view<entity_t> entities(stage_range_t range) const noexcept {
		return entities_impl(range);
	}

	virtual void* get_if(entity_t entity, stage_range_t stage = stage_range_t::ALL) {
		return nullptr;
	}

	inline void purge() noexcept {
		purge_impl();
	}
	constexpr component_key_t const& id() const noexcept {
		return m_Info.id;
	}

	inline psl::array_view<entity_t> added_entities() const noexcept {
		return entities_impl(stage_range_t::ADDED);
	};
	inline psl::array_view<entity_t> removed_entities() const noexcept {
		return entities_impl(stage_range_t::REMOVED);
	};

	virtual entity_t::size_type* write_memory_location_offsets_for(psl::array_view<entity_t> entities,
																   entity_t::size_type* target) const noexcept {
		return target;
	}
	virtual size_t copy_to(psl::array_view<entity_t> entities, void* destination) const noexcept {
		return 0;
	};
	virtual size_t copy_from(psl::array_view<entity_t> entities, void* source, bool repeat = false) noexcept {
		return 0;
	};

	virtual size_t apply_mutation(component_container_t* source) noexcept {
		psl_assert(false, "Component {} does not support mutation", m_Info.id.name());
		return 0;
	};

	inline component_type_info_t const& component_type_info() const noexcept {
		return m_Info;
	};
	size_t size(bool include_removed = false) const noexcept {
		return size_impl((include_removed) ? stage_range_t::ALL : stage_range_t::ALIVE);
	}
	size_t size(stage_range_t range) const noexcept {
		return size_impl(range);
	}

	void set(entity_t entity, void* data) noexcept {
		set_impl(entity, data);
	}
	virtual void should_serialize(bool value) noexcept {
		psl_assert(false, "Component {} is not a type that can be serialized", m_Info.id.name());
	}

	virtual void remap(const psl::sparse_array<entity_t::size_type, entity_t::size_type>& mapping,
					   std::function<bool(entity_t)> pred) noexcept = 0;
	virtual bool merge(const component_container_t& other) noexcept = 0;
	virtual void clear()											= 0;
	virtual void reserve(size_t count)								= 0;

  protected:
	virtual entity_t*
	remove_if_has_impl(entity_t* begin, entity_t* end, stage_range_t stage, bool has_not = false) const noexcept = 0;
	virtual void purge_impl() noexcept																			 = 0;
	virtual void add_impl(entity_t entity, void* data)															 = 0;
	virtual void add_impl(psl::array_view<entity_t> entities, void* data, bool repeat)							 = 0;
	virtual void add_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
						  void* data,
						  bool repeat)																			 = 0;
	virtual psl::array_view<entity_t> entities_impl(stage_range_t stage) const noexcept							 = 0;
	virtual void set_impl(entity_t entity, void* data) noexcept													 = 0;
	virtual void remove_impl(entity_t entity)																	 = 0;
	virtual void remove_impl(psl::array_view<entity_t> entities)												 = 0;
	virtual void remove_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities)		 = 0;
	virtual bool has_impl(entity_t entity, stage_range_t stage) const noexcept									 = 0;
	virtual size_t size_impl(stage_range_t range) const noexcept												 = 0;

  protected:
	component_type_info_t m_Info;
};

template <typename T>
struct component_container_type_for;

template <typename T>
class component_container_typed_t final : public component_container_t {
	static_assert(std::is_same_v<typename component_container_type_for<T>::type, component_container_typed_t<T>>,
				  "The complex container should only be used for complex types");

  public:
	component_container_typed_t()
		: component_container_t(component_type_info_t {.id			 = details::component_key_t::generate<T>(),
													   .version		 = 0,
													   .size		 = sizeof(T),
													   .alignment	 = std::alignment_of_v<T>,
													   .mutability	 = component_traits_t<T>::mutability,
													   .serializable = component_traits_t<T>::serializable}) {};
	~component_container_typed_t() override = default;
	auto& entity_data() noexcept {
		return m_Entities;
	};

	void reserve(size_t count) override {
		m_Entities.reserve(psl::narrow_cast<entity_t::size_type>(count));
	}

	void* data() noexcept override {
		return m_Entities.data();
	}
	void* const data() const noexcept override {
		return m_Entities.data();
	}

	bool has_storage_for(entity_t entity) const noexcept override {
		return m_Entities.has(entity, stage_range_t::ALL);
	}

	void* get_if(entity_t entity, stage_range_t stage = stage_range_t::ALL) override {
		return m_Entities.addressof_if(entity, stage);
	}

	entity_t::size_type* write_memory_location_offsets_for(psl::array_view<entity_t> entities,
														   entity_t::size_type* destination) const noexcept override {
		m_Entities.write_dense_indices(entities.begin(), entities.end(), destination);
		return destination + entities.size();
	}

	size_t copy_to(psl::array_view<entity_t> entities, void* destination) const noexcept override {
		psl_assert((std::uintptr_t)destination % m_Info.alignment == 0, "pointer has to be aligned");
		T* dest = (T*)destination;
		m_Entities.copy_dense_into(entities.begin(), entities.end(), dest);
		return entities.size() * sizeof(T);
	}
	size_t copy_from(psl::array_view<entity_t> entities, void* source, bool repeat) noexcept override {
		psl_assert((std::uintptr_t)source % m_Info.alignment == 0, "pointer has to be aligned");
		T* src = (T*)source;
		m_Entities.assign(entities.begin(), entities.end(), src, repeat ? nullptr : src + entities.size());
		return sizeof(T) * entities.size();
	};

	void remap(const psl::sparse_array<entity_t::size_type, entity_t::size_type>& mapping,
			   std::function<bool(entity_t)> pred) noexcept override {
		m_Entities.remap(mapping, [pred](entity_t e) -> bool { return pred(e); });
	}
	bool merge(const component_container_t& other) noexcept override {
		if(other.id() != id())
			return false;

		component_container_typed_t<T>* other_ptr = (component_container_typed_t<T>*)(&other);
		m_Entities.merge(other_ptr->m_Entities);
		return true;
	}

	void set(entity_t e, const T& data) noexcept {
		m_Entities[e] = data;
	}

  protected:
	void set_impl(entity_t entity, void* data) noexcept override {
		m_Entities.set(&entity, &entity + 1, (T*)data);
	}
	psl::array_view<entity_t> entities_impl(stage_range_t stage) const noexcept override {
		return m_Entities.indices(stage);
	}
	void add_impl(psl::array_view<entity_t> entities, void* data, bool repeat) override {
		T* source = (T*)data;
		if(source == nullptr) {
			m_Entities.insert(entities.begin(), entities.end());
		} else if(repeat) {
			m_Entities.insert(entities.begin(), entities.end(), source);
		} else {
			m_Entities.insert(entities.begin(), entities.end(), source, source + entities.size());
		}
	}
	void add_impl(entity_t entity, void* data) override {
		if(data) {
			m_Entities.insert(&entity, &entity + 1, (T*)data);
		} else {
			m_Entities.insert(&entity, &entity + 1);
		}
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
		T* source  = (T*)data;
		if(data == nullptr) {
			for(auto range : entities) {
				psl::array<entity_t> range_view(range.second - range.first);
				std::iota(range_view.begin(), range_view.end(), range.first);
				m_Entities.insert(range_view.begin(), range_view.end());
			}
		} else if(repeat) {
			for(auto range : entities) {
				psl::array<entity_t> range_view(range.second - range.first);
				std::iota(range_view.begin(), range_view.end(), range.first);
				m_Entities.insert(range_view.begin(), range_view.end(), source);
			}
		} else {
			for(auto range : entities) {
				for(auto e = range.first; e < range.second; ++e) {
					psl::array<entity_t> range_view(range.second - range.first);
					std::iota(range_view.begin(), range_view.end(), range.first);
					m_Entities.insert(
					  range_view.begin(), range_view.end(), source, source + (range.second - range.first));
				}
			}
		}
	}
	void purge_impl() noexcept override {
		m_Entities.promote();
	}

	void remove_impl(entity_t entity) override {
		m_Entities.try_erase(&entity, &entity + 1);
	}

	void remove_impl(psl::array_view<entity_t> entities) override {
		m_Entities.try_erase(entities.begin(), entities.end());
	}
	void remove_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) override {
		psl::array<entity_t> indices;
		for(auto range : entities) {
			auto const count = range.second - range.first;
			if(indices.size() < count) {
				indices.resize(count);
			}
			std::iota(indices.begin(), std::next(indices.begin(), count), range.first);

			m_Entities.try_erase(indices.begin(), std::next(indices.begin(), count));
		}
	}
	bool has_impl(entity_t entity, stage_range_t stage) const noexcept override {
		return m_Entities.has(entity, stage);
	}

	size_t size_impl(stage_range_t range) const noexcept override {
		return m_Entities.size(range);
	}

	void clear() override {
		m_Entities.clear();
	}

	entity_t* remove_if_has_impl(entity_t* begin,
								 entity_t* end,
								 stage_range_t stage,
								 bool has_not = false) const noexcept override {
		if(has_not) {
			return m_Entities.remove_if_has_not(begin, end, stage);
		} else {
			return m_Entities.remove_if_has(begin, end, stage);
		}
	}

  private:
	details::staged_sparse_array<T> m_Entities;
};

class component_container_flag_t final : public component_container_t {
  public:
	component_container_flag_t(component_type_info_t info)
		: component_container_t({
			.id		   = info.id,
			.version   = 0,
			.size	   = 0,
			.alignment = 0,
			.mutability =
			  component_mutability_behaviour_t::unrestricted, /* doesn't have backing memory to begin with */
			.serializable = info.serializable,
		  }) {
		psl_assert(info.version == 0, "Flag type component {} cannot have a version", info.id.name());
		psl_assert(
		  info.size == 0, "Flag type component {} cannot have a size, but had size of {}", info.id.name(), info.size);
		psl_assert(info.alignment == 0,
				   "Flag type component {} cannot have an alignment, but had one set to {}",
				   info.id.name(),
				   info.alignment);
		psl_assert(info.mutability == component_mutability_behaviour_t::unrestricted,
				   "Flag type component {} cannot be modified, so setting this trait has no effect",
				   info.id.name());
	};
	~component_container_flag_t() override = default;


	void reserve(size_t count) override {
		m_Entities.reserve(psl::narrow_cast<entity_t::size_type>(count));
	}

	void* data() noexcept override {
		return nullptr;
	}
	void* const data() const noexcept override {
		return nullptr;
	}

	bool has_storage_for(entity_t entity) const noexcept override {
		return m_Entities.has(entity, stage_range_t::ALL);
	}

	void remap(const psl::sparse_array<entity_t::size_type, entity_t::size_type>& mapping,
			   std::function<bool(entity_t)> pred) noexcept override {
		m_Entities.remap(mapping, [pred](entity_t entity) { return pred(entity); });
	}

	bool merge(const component_container_t& other) noexcept override {
		if(other.id() != id())
			return false;

		component_container_flag_t* other_ptr = (component_container_flag_t*)(&other);
		m_Entities.merge(other_ptr->m_Entities);
		return true;
	}


	void should_serialize(bool value) noexcept override {
		m_Serializable = value;
	}

  protected:
	void set_impl(entity_t entity, void* data) noexcept override {};
	psl::array_view<entity_t> entities_impl(stage_range_t stage) const noexcept override {
		return m_Entities.indices(stage);
	}
	void add_impl(psl::array_view<entity_t> entities, void* data, bool repeat) override {
		m_Entities.insert(entities.begin(), entities.end());
	}
	void add_impl(entity_t entity, void* data) override {
		m_Entities.insert(&entity, &entity + 1);
	}
	void add_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
				  void* data,
				  bool repeat) override {
		auto count = psl::narrow_cast<entity_t::size_type>(
		  std::accumulate(std::begin(entities),
						  std::end(entities),
						  size_t {0},
						  [](size_t sum, const std::pair<entity_t::size_type, entity_t::size_type>& r) {
							  return sum + (r.second - r.first);
						  }));

		m_Entities.reserve(m_Entities.size(stage_range_t::ALL) + count);
		for(auto range : entities) {
			psl::array<entity_t> indices;
			auto count = range.second - range.first;
			std::iota(indices.begin(), indices.end(), range.first);
			m_Entities.insert(indices.begin(), indices.end());
		}
	}
	void purge_impl() noexcept override {
		m_Entities.promote();
	}

	void remove_impl(entity_t entity) override {
		m_Entities.try_erase(&entity, &entity + 1);
	}
	void remove_impl(psl::array_view<entity_t> entities) override {
		m_Entities.try_erase(entities.begin(), entities.end());
	}
	void remove_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) override {
		for(auto range : entities) {
			psl::array<entity_t> indices;
			auto count = range.second - range.first;
			std::iota(indices.begin(), indices.end(), range.first);
			m_Entities.try_erase(indices.begin(), indices.end());
		}
	}
	bool has_impl(entity_t entity, stage_range_t stage) const noexcept override {
		return m_Entities.has(entity, stage);
	}

	size_t size_impl(stage_range_t range) const noexcept override {
		return m_Entities.size(range);
	}

	void clear() override {
		m_Entities.clear();
		m_Serializable = false;
	}

	entity_t* remove_if_has_impl(entity_t* begin,
								 entity_t* end,
								 stage_range_t stage,
								 bool has_not = false) const noexcept override {
		if(has_not) {
			return m_Entities.remove_if_has_not(begin, end, stage);
		} else {
			return m_Entities.remove_if_has(begin, end, stage);
		}
	}

  private:
	details::staged_sparse_array<details::flag_tag_t> m_Entities;
	bool m_Serializable {false};
};

class component_container_untyped_t final : public component_container_t {
	using stage_range_t = details::stage_range_t;

  public:
	component_container_untyped_t(component_type_info_t info)
		: component_container_t(info), m_Entities(psl::narrow_cast<entity_t::size_type>(info.size),
												  psl::narrow_cast<entity_t::size_type>(info.alignment)) {};
	~component_container_untyped_t() override = default;
	auto& entity_data() noexcept {
		return m_Entities;
	};

	void reserve(size_t count) override {
		m_Entities.reserve(psl::narrow_cast<entity_t::size_type>(count));
	}

	void* data() noexcept override {
		return m_Entities.data();
	}
	void* const data() const noexcept override {
		return m_Entities.data();
	}

	bool has_storage_for(entity_t entity) const noexcept override {
		return m_Entities.has(entity, stage_range_t::ALL);
	}

	void* get_if(entity_t entity, stage_range_t stage = stage_range_t::ALL) override {
		return m_Entities.addressof_if(entity, stage);
	}

	entity_t::size_type* write_memory_location_offsets_for(psl::array_view<entity_t> entities,
														   entity_t::size_type* destination) const noexcept override {
		m_Entities.write_dense_indices(entities.begin(), entities.end(), destination);
		return destination + entities.size();
	}

	size_t copy_to(psl::array_view<entity_t> entities, void* destination) const noexcept override {
		psl_assert((std::uintptr_t)destination % m_Info.alignment == 0, "pointer has to be aligned");
		std::byte* dest = (std::byte*)destination;
		m_Entities.copy_dense_into(entities.begin(), entities.end(), details::untyped_iterator_t {dest, m_Info.size});
		return entities.size() * m_Info.size;
	}
	size_t copy_from(psl::array_view<entity_t> entities, void* source, bool repeat) noexcept override {
		psl_assert((std::uintptr_t)source % m_Info.alignment == 0, "pointer has to be aligned");
		std::byte* src = (std::byte*)source;
		if(repeat) {
			m_Entities.assign(entities.begin(), entities.end(), details::untyped_iterator_t {src, m_Info.size});

		} else {
			m_Entities.assign(entities.begin(),
							  entities.end(),
							  details::untyped_iterator_t {src, m_Info.size},
							  details::untyped_iterator_t {src + entities.size(), m_Info.size});
		}
		return m_Info.size * entities.size();
	};

	void remap(const psl::sparse_array<entity_t::size_type, entity_t::size_type>& mapping,
			   std::function<bool(entity_t)> pred) noexcept override {
		m_Entities.remap(mapping, [pred](entity_t entity) { return pred(entity); });
	}

	bool merge(const component_container_t& other) noexcept override {
		if(other.id() != id())
			return false;

		component_container_untyped_t* other_ptr = (component_container_untyped_t*)(&other);
		return m_Entities.merge(other_ptr->m_Entities).success;
	}

	template <typename T>
	void set(entity_t e, const T& data) noexcept {
		m_Entities.set(&e, &e + 1, &data);
	}

	void should_serialize(bool value) noexcept override {
		m_Info.serializable = value;
	}

	void clear() override {
		m_Entities.clear();
		m_Info.serializable = false;
	}

	size_t apply_mutation(component_container_t* source) noexcept override {
		auto entities	 = source->entities(stage_range_t::ALL);
		auto source_data = source->data();

		psl_assert((std::uintptr_t)source_data % m_Info.alignment == 0, "pointer has to be aligned");
		psl_assert(source->component_type_info().alignment == m_Info.alignment,
				   "components must have the same alignment");
		psl_assert(source->component_type_info().size == m_Info.size, "components have to be the same size");

		std::byte* diff = new std::byte[m_Info.size];

		m_Entities.for_each_guarantee(
		  [&diff, type_size = m_Info.size](entity_t::size_type index, std::byte* dst, std::byte* src) {
			  for(size_t i = 0; i < type_size; ++i) {
				  diff[i] = (src[i] != dst[i]) ? std::byte {0xff} : std::byte {0};
			  }

			  std::memcpy(dst, src, type_size);
			  std::memcpy(src, diff, type_size);
		  },
		  entities.begin(),
		  entities.end(),
		  details::untyped_iterator_t {(std::byte*)source_data, m_Info.size},
		  details::untyped_iterator_t {(std::byte*)source_data, m_Info.size} + entities.size());

		delete[] diff;

		return m_Info.size * entities.size();
	};

  protected:
	void set_impl(entity_t entity, void* data) noexcept override {
		m_Entities.set(&entity, &entity + 1, details::untyped_iterator_t {(std::byte*)data, m_Info.size});
	}
	psl::array_view<entity_t> entities_impl(stage_range_t stage) const noexcept override {
		return m_Entities.indices(stage);
	}

	void add_impl(psl::array_view<entity_t> entities, void* data, bool repeat) override {
		std::byte* source = (std::byte*)data;
		if(data == nullptr) {
			m_Entities.insert(entities.begin(), entities.end());
		} else if(repeat) {
			m_Entities.insert(entities.begin(), entities.end(), details::untyped_iterator_t {source, m_Info.size});
		} else {
			m_Entities.insert(entities.begin(),
							  entities.end(),
							  details::untyped_iterator_t {source, m_Info.size},
							  details::untyped_iterator_t {source, m_Info.size} + entities.size());
		}
	}

	void add_impl(entity_t entity, void* data) override {
		if(data) {
			m_Entities.insert(&entity, &entity + 1, details::untyped_iterator_t {(std::byte*)data, m_Info.size});
		} else {
			m_Entities.insert(&entity, &entity + 1);
		}
	}

	void add_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
				  void* data,
				  bool repeat) override {
		std::byte* source = (std::byte*)data;
		if(data == nullptr) {
			for(auto range : entities) {
				psl::array<entity_t> range_view(range.second - range.first);
				std::iota(range_view.begin(), range_view.end(), range.first);
				m_Entities.insert(range_view.begin(), range_view.end());
			}
		} else if(repeat) {
			for(auto range : entities) {
				psl::array<entity_t> range_view(range.second - range.first);
				std::iota(range_view.begin(), range_view.end(), range.first);
				m_Entities.insert(
				  range_view.begin(), range_view.end(), details::untyped_iterator_t {source, m_Info.size});
			}
		} else {
			for(auto range : entities) {
				psl::array<entity_t> range_view(range.second - range.first);
				std::iota(range_view.begin(), range_view.end(), range.first);
				m_Entities.insert(range_view.begin(),
								  range_view.end(),
								  details::untyped_iterator_t {source, m_Info.size},
								  details::untyped_iterator_t {source, m_Info.size} + range.second - range.first);
			}
		}
	}
	void purge_impl() noexcept override {
		m_Entities.promote();
	}

	void remove_impl(entity_t entity) override {
		m_Entities.try_erase(&entity, &entity + 1);
	}
	void remove_impl(psl::array_view<entity_t> entities) override {
		m_Entities.try_erase(entities.begin(), entities.end());
	}
	void remove_impl(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) override {
		for(auto range : entities) {
			psl::array<entity_t> range_view(range.second - range.first);
			std::iota(range_view.begin(), range_view.end(), range.first);
			m_Entities.try_erase(range_view.begin(), range_view.end());
		}
	}
	bool has_impl(entity_t entity, stage_range_t stage) const noexcept override {
		return m_Entities.has(entity, stage);
	}

	size_t size_impl(stage_range_t range) const noexcept override {
		return m_Entities.size(range);
	}

	entity_t* remove_if_has_impl(entity_t* begin,
								 entity_t* end,
								 stage_range_t stage,
								 bool has_not = false) const noexcept override {
		if(has_not) {
			return m_Entities.remove_if_has_not(begin, end, stage);
		} else {
			return m_Entities.remove_if_has(begin, end, stage);
		}
	}

  private:
	details::staged_sparse_array<details::untyped_tag_t> m_Entities;
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
	static_assert(IsComponentTypeSerializable<T> || !component_traits_t<T>::serializable,
				  "Unsupported. Component type cannot support serialization, please fix your `component_traits<T>` "
				  "specialization for this component type");

	if constexpr(IsComponentFlagType<T>) {
		return std::unique_ptr<component_container_t>(std::make_unique<component_container_flag_t>(
		  component_type_info_t {.id		   = psl::ecs::details::component_key_t::generate<T>(),
								 .version	   = component_traits_t<T>::version,
								 .size		   = 0,
								 .alignment	   = 0,
								 .mutability   = component_traits_t<T>::mutability,
								 .serializable = component_traits_t<T>::serializable}));
	} else if constexpr(IsComponentTrivialType<T>) {
		return std::unique_ptr<component_container_t>(std::make_unique<component_container_untyped_t>(
		  component_type_info_t {.id		   = psl::ecs::details::component_key_t::generate<T>(),
								 .version	   = component_traits_t<T>::version,
								 .size		   = sizeof(T),
								 .alignment	   = std::alignment_of_v<T>,
								 .mutability   = component_traits_t<T>::mutability,
								 .serializable = component_traits_t<T>::serializable}));
	} else if constexpr(IsComponentComplexType<T>) {
		return std::unique_ptr<component_container_t>(std::make_unique<component_container_typed_t<T>>());
	}
}

inline auto instantiate_component_container(component_type_info_t info) -> std::unique_ptr<component_container_t> {
	switch(info.id.type()) {
	case psl::ecs::component_type::TRIVIAL:
		return std::make_unique<component_container_untyped_t>(info);
	case psl::ecs::component_type::FLAG:
		return std::make_unique<component_container_flag_t>(info);
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
