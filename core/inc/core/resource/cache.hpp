#pragma once
#include "core/fwd/resource/resource.hpp"
#include "psl/library.hpp"
#include "psl/meta.hpp"
#include <cstdint>		  // uintptr_t
#include <type_traits>	  // std::remove_const/etc
//#include "psl/memory/region.hpp"
#include "core/logging.hpp"
#include "psl/profiling/profiler.hpp"
#include "psl/serialization/serializer.hpp"
#include "psl/static_array.hpp"
#include "psl/unique_ptr.hpp"
#include "psl/view_ptr.hpp"


namespace psl {
template <class InputIt, class OutputIt, class Pred, class Fct>
void transform_if(InputIt first, InputIt last, OutputIt dest, Pred pred, Fct transform) {
	while(first != last) {
		if(pred(*first))
			*dest++ = transform(*first);

		++first;
	}
}
}	 // namespace psl

namespace core::resource {
using resource_key_t = const std::uintptr_t* (*)();
namespace details {
	// added to trick the compiler to not throw away the results at compile time
	template <typename T>
	constexpr const std::uintptr_t resource_key_var {0u};

	template <typename T>
	constexpr const std::uintptr_t* resource_key() noexcept {
		return &resource_key_var<T>;
	}

	template <typename T>
	constexpr resource_key_t key_for() {
		return resource_key<typename std::decay<T>::type>;
	};

	template <typename T>
	class container;

	template <typename C, typename T>
	struct is_valid_alias : std::false_type {};

	template <typename C, typename... Ts>
	struct is_valid_alias<C, alias<Ts...>> {
		static const bool value {std::is_constructible_v<C, handle<alias<Ts...>>&>};
	};

	template <typename T, typename SFINAE = void>
	struct alias_type : std::false_type {
		using type = void;
	};


	template <typename T>
	struct alias_type<T, std::void_t<typename T::alias_type>> : std::true_type {
		using type	  = typename T::alias_type;
		using alias_t = handle<type>;
	};

	template <typename... Ts>
	constexpr psl::static_array<resource_key_t, sizeof...(Ts)> keys_for(handle<alias<Ts...>>*) {
		return {key_for<Ts>()...};
	}

	template <typename T, typename Y>
	struct alias_has_type : std::false_type {};

	template <typename T, typename... Ts>
	struct alias_has_type<T, alias<Ts...>> : std::disjunction<std::is_same<T, Ts>...> {};
}	 // namespace details


/// \brief represents a container of resources and their UID mappings
///
/// The resource cache is a specialized container that can handle lifetime and ID mapping
/// of resources. It also controls the memory that the resource will be allocated to.
/// This means that every resource is guaranteed to be part of atleast one cache.
/// Resource can also access other resources within the same cache with ease through the
/// shared psl::meta::library instance. Libraries can be shared between caches, but remember that
/// neither the psl::meta::library nor core::resource::cache_t are thread-safe by themselves.
/// \todo check multi-cache
struct metadata {
	psl::UID uid;
	psl::UID resource_uid;
	resource_key_t type;
	status state;
	size_t reference_count;
	bool strong_type;
};

class cache_t {
  public:
	struct description {
		metadata metaData;
		void* resource {nullptr};
		size_t age {0};
	};
	struct entry {
		psl::array<psl::unique_ptr<description>> descriptions;
		psl::view_ptr<psl::meta::file> metaFile;	// owned by the library
	};

  public:
	cache_t(psl::meta::library library) : m_Library(std::move(library)) {};
	~cache_t() { free(true); };

	cache_t(const cache_t& other)			 = delete;
	cache_t(cache_t&& other)				 = default;
	cache_t& operator=(const cache_t& other) = delete;
	cache_t& operator=(cache_t&& other)		 = default;

	template <typename T, typename... Args>
	handle<T> instantiate(const psl::UID& resource_uid, Args&&... args) {
		return instantiate_using<T>(psl::UID::generate(), resource_uid, std::forward<Args>(args)...);
	}
	template <typename T, typename... Args>
	handle<T> instantiate_using(const psl::UID& uid, const psl::UID& resource_uid, Args&&... args) {
		using value_type   = std::remove_cv_t<std::remove_const_t<T>>;
		using meta_type	   = typename resource_traits<T>::meta_type;
		constexpr auto key = details::key_for<value_type>();
		if(auto it = m_Deleters.find(key); it == std::end(m_Deleters)) {
			m_Deleters[key]	 = [](void* resource) { delete((T*)(resource)); };
			m_TypeNames[key] = typeid(T).name();
		}
		auto& data						 = m_Cache[uid];
		m_RemappedResource[resource_uid] = uid;

		auto& descr = *data.descriptions.emplace_back(
		  new description {metadata {uid,
									 resource_uid,
									 key,
									 status::initial,
									 0u,
									 std::is_same_v<typename details::alias_type<value_type>::type, void>},
						   nullptr,
						   m_AgeCounter++});

		if(data.metaFile == nullptr) {
			if(auto optMetaFile = m_Library.get<meta_type>(resource_uid); optMetaFile)
				data.metaFile = static_cast<psl::meta::file*>(optMetaFile.value());
			else {
				core::log->error("could not load resource [uid: '{}'] reason: missing", resource_uid.to_string());
				descr.metaData.state = status::missing;
				return {nullptr, this, &descr.metaData, nullptr};
			}
		}

		auto task = [&descr, &cache = *this, &library = m_Library, metaFile = data.metaFile](auto&&... values) {
			descr.metaData.state = status::loading;
			T* resource			 = nullptr;
			resource =
			  new T(cache, descr.metaData, (meta_type*)&metaFile.get(), std::forward<decltype(values)>(values)...);
			if constexpr(psl::serialization::details::is_collection<T>::value) {
				if(auto result = library.load(descr.metaData.resource_uid); result) {
					psl::serialization::serializer s;
					psl::format::container cont {result.value()};
					s.deserialize<psl::serialization::decode_from_format, T>(*resource, cont);
				} else {
					core::log->error("could not load resource [uid: '{}'] reason: missing",
									 descr.metaData.resource_uid.to_string());
					descr.metaData.state = status::missing;
					return;
				}
			}
			descr.resource		 = (void*)resource;
			descr.metaData.state = status::loaded;
		};
		std::invoke(task, std::forward<Args>(args)...);

		return {descr.resource, this, &descr.metaData, data.metaFile};
	}

	template <typename T, typename... Args>
	handle<T> create(Args&&... args) {
		return create_using<T>(psl::UID::generate(), std::forward<Args>(args)...);
	}


	template <typename T, typename... Args>
	handle<T> create_using(const psl::UID& uid, Args&&... args) {
		using value_type   = std::remove_cv_t<std::remove_const_t<T>>;
		using meta_type	   = typename resource_traits<T>::meta_type;
		constexpr auto key = details::key_for<value_type>();
		if(auto it = m_Deleters.find(key); it == std::end(m_Deleters)) {
			m_Deleters[key]	 = [](void* resource) { delete((T*)(resource)); };
			m_TypeNames[key] = typeid(T).name();
		}

		auto& data = m_Cache[uid];
		if(data.metaFile == nullptr) {
			data.metaFile = static_cast<psl::meta::file*>(&m_Library.create<meta_type>(uid).second);
		}


		auto& descr = *data.descriptions.emplace_back(
		  new description {metadata {uid,
									 psl::UID::invalid_uid,
									 details::key_for<value_type>(),
									 status::initial,
									 0u,
									 std::is_same_v<typename details::alias_type<value_type>::type, void>},
						   nullptr,
						   m_AgeCounter++});


		auto task = [&descr, &cache = *this, metaFile = data.metaFile](auto&&... values) {
			descr.metaData.state = status::loading;
			T* resource			 = nullptr;

			resource =
			  new T(cache, descr.metaData, (meta_type*)&metaFile.get(), std::forward<decltype(values)>(values)...);
			descr.resource		 = (void*)resource;
			descr.metaData.state = status::loaded;
		};
		std::invoke(task, std::forward<Args>(args)...);

		return {descr.resource, this, &descr.metaData, data.metaFile};
	}
	template <typename T, typename... Args>
	handle<T> create_using(std::unique_ptr<typename resource_traits<T>::meta_type> metaData, Args&&... args) {
		using value_type   = std::remove_cv_t<std::remove_const_t<T>>;
		using meta_type	   = typename resource_traits<T>::meta_type;
		constexpr auto key = details::key_for<value_type>();
		if(auto it = m_Deleters.find(key); it == std::end(m_Deleters)) {
			m_Deleters[key]	 = [](void* resource) { delete((T*)(resource)); };
			m_TypeNames[key] = typeid(T).name();
		}
		psl::UID uid = psl::UID::generate();
		auto pair	 = m_Library.add(uid, std::move(metaData));

		auto& data	  = m_Cache[uid];
		data.metaFile = &pair.second;

		auto& descr = *data.descriptions.emplace_back(
		  new description {metadata {uid,
									 psl::UID::invalid_uid,
									 details::key_for<value_type>(),
									 status::initial,
									 0u,
									 std::is_same_v<typename details::alias_type<value_type>::type, void>},
						   nullptr,
						   m_AgeCounter++});


		auto task = [&descr, &cache = *this, metaFile = data.metaFile](auto&&... values) {
			descr.metaData.state = status::loading;
			T* resource			 = nullptr;

			resource			 = new T(cache,
							 descr.metaData,
							 reinterpret_cast<meta_type*>(&metaFile.get()),
							 std::forward<decltype(values)>(values)...);
			descr.resource		 = (void*)resource;
			descr.metaData.state = status::loaded;
		};
		std::invoke(task, std::forward<Args>(args)...);

		return {descr.resource, this, &descr.metaData, data.metaFile};
	}

	bool contains(const psl::UID& uid) const noexcept { return m_Cache.find(uid) != std::end(m_Cache); }

	template <typename T, typename... Args>
	handle<T> find(const psl::UID& uid) noexcept {
		using value_type = std::remove_cv_t<std::remove_const_t<T>>;
		auto it			 = m_Cache.find(uid);
		if(it == std::end(m_Cache)) {
			auto resIt = m_RemappedResource.find(uid);
			if(resIt == std::end(m_RemappedResource))
				return {};
			it = m_Cache.find(resIt->second);
			if(it == std::end(m_Cache))
				return {};
		}

		constexpr auto key = details::key_for<T>();
		for(auto& entry : it->second.descriptions) {
			if(entry->metaData.type == key)
				return {entry->resource, this, &entry->metaData, it->second.metaFile};
		}

		using alias_t = typename details::alias_type<value_type>::type;
		// assemble all child types, and create an alias parent from them
		if constexpr(!std::is_same_v<alias_t, void>) {
			constexpr auto alias_keys = details::keys_for((alias_t*)(nullptr));

			psl::array<description*> eligable;
			psl::transform_if(
			  std::begin(it->second.descriptions),
			  std::end(it->second.descriptions),
			  std::back_inserter(eligable),
			  [&alias_keys](psl::unique_ptr<description>& descr) {
				  return std::find(std::begin(alias_keys), std::end(alias_keys), descr->metaData.type) !=
						 std::end(alias_keys);
			  },
			  [](psl::unique_ptr<description>& descr) -> description* { return &descr.get(); });

			if(eligable.size() == 0)
				return {};

			alias_t result {eligable, this, it->second.metaFile};
			return create_using<T>(uid, result);
		}
		return {};
	}

	template <typename T, typename... Args>
	handle<T> find_resource(const psl::UID& uid) noexcept {
		using value_type = std::remove_cv_t<std::remove_const_t<T>>;
		auto resIt		 = m_RemappedResource.find(uid);
		if(resIt == std::end(m_RemappedResource))
			return {};
		auto it = m_Cache.find(resIt->second);
		if(it == std::end(m_Cache))
			return {};

		constexpr auto key = details::key_for<T>();
		for(auto& entry : it->second.descriptions) {
			if(entry->metaData.type == key)
				return {entry->resource, this, &entry->metaData, it->second.metaFile};
		}

		using alias_t = typename details::alias_type<value_type>::type;
		// assemble all child types, and create an alias parent from them
		if constexpr(!std::is_same_v<alias_t, void>) {
			constexpr auto alias_keys = details::keys_for((alias_t*)(nullptr));

			psl::array<description*> eligable;
			psl::transform_if(
			  std::begin(it->second.descriptions),
			  std::end(it->second.descriptions),
			  std::back_inserter(eligable),
			  [&alias_keys](psl::unique_ptr<description>& descr) {
				  return std::find(std::begin(alias_keys), std::end(alias_keys), descr->metaData.type) !=
						 std::end(alias_keys);
			  },
			  [](psl::unique_ptr<description>& descr) -> description* { return &descr.get(); });

			if(eligable.size() == 0)
				return {};

			alias_t result {eligable, this, it->second.metaFile};
			return create_using<T>(uid, result);
		}
		return {};
	}

	template <typename T>
	bool free(resource::handle<T>& target) {
		if(target.m_MetaData->reference_count <= 1 && target.m_MetaData->state == status::loaded) {
			target.m_MetaData->state = status::unloading;
			std::invoke(m_Deleters[target.m_MetaData->type], target.m_Resource);
			target.m_MetaData->state = status::unloaded;
			return true;
		}
		return false;
	}

	template <typename T>
	bool free(resource::weak_handle<T>& target) {
		if(target.m_MetaData->reference_count <= 1 && target.m_MetaData->state == status::loaded) {
			target.m_MetaData->state = status::unloading;
			std::invoke(m_Deleters[target.m_MetaData->type], target.m_Resource);
			target.m_MetaData->state = status::unloaded;
			return true;
		}
		return false;
	}

	void free(bool clear_all = false) {
		LOG_INFO("destroying cache start");
		bool bErased	 = false;
		size_t iteration = 0u;
		bool bLeaks		 = false;
		do {
			bErased		 = false;
			bLeaks		 = false;
			size_t count = 0u;
			for(auto& pair : m_Cache) {
				for(auto& it : pair.second.descriptions) {
					bLeaks |= it->metaData.state == status::loaded;
					if(it->metaData.reference_count == 0 && it->metaData.state == status::loaded) {
						it->metaData.state = status::unloading;
						std::invoke(m_Deleters[it->metaData.type], it->resource);
						it->metaData.state = status::unloaded;
						bErased			   = true;
						++count;
					}
				}
			}
			LOG_INFO(
			  "iteration ", utility::to_string(iteration++), " destroyed ", utility::to_string(count), " objects");
		} while(bErased);	 // keep looping as long as items are present


		for(auto it = std::begin(m_Cache); it != std::end(m_Cache);) {
			it->second.descriptions.erase(
			  std::remove_if(std::begin(it->second.descriptions),
							 std::end(it->second.descriptions),
							 [](const auto& it) { return it->metaData.state == status::unloaded; }),
			  std::end(it->second.descriptions));
			if(it->second.descriptions.size() == 0) {
				m_Library.unload(it->first);
				it = m_Cache.erase(it);
			} else
				++it;
		}

		if(bLeaks && clear_all) {
			LOG_WARNING("leaks detected!");

#ifdef PE_DEBUG
			size_t oldest = std::numeric_limits<size_t>::max();
			description* oldest_descr {nullptr};
			psl::UID oldest_uid;
#endif
			for(auto& pair : m_Cache) {
				for(auto& it : pair.second.descriptions) {
#ifdef PE_DEBUG
					if(it->age < oldest) {
						oldest_descr = &it.get();
						oldest_uid	 = pair.first;
						oldest		 = it->age;
					}
#endif
					if(it->metaData.state == status::loaded) {
#ifdef PE_DEBUG

						core::log->warn("\ttype {0} uid {1} age {2}",
										m_TypeNames[it->metaData.type],
										utility::to_string(pair.first),
										it->age);
#else
						core::log->warn("\ttype {0} uid {1}",
										utility::to_string((std::uintptr_t)(it->metaData.type)),
										utility::to_string(pair.first));
#endif
					}
				}
			}

#ifdef PE_DEBUG
			if(oldest_descr) {
				core::log->warn("possible source: type {0} uid {1}",
								m_TypeNames[oldest_descr->metaData.type],
								utility::to_string(oldest_uid));
			}
#endif
		}
		core::log->flush();

		LOG_INFO("destroying cache end");
	}

	const psl::meta::library& library() const noexcept {
		return m_Library;
	}
	psl::meta::library& library() noexcept {
		return m_Library;
	}

  private:
	size_t m_AgeCounter {0};
	psl::meta::library m_Library;
	std::unordered_map<psl::UID, entry> m_Cache {};
	std::unordered_map<psl::UID, psl::UID> m_RemappedResource {};
	std::unordered_map<resource_key_t, psl::string8_t> m_TypeNames {};
	std::unordered_map<resource_key_t, std::function<void(void*)>> m_Deleters {};
};
}	 // namespace core::resource
