#pragma once
#include "cache.hpp"
#include "fwd/resource/resource.hpp"
#include "psl/meta.hpp"
#include <memory>

namespace core::resource {
template <typename T>
class weak_handle;

/// \brief wraps around a resource for sharing and management purposes.
///
/// resource handles are used to manage the lifetime of resources, and tracking
/// dependencies (i.e. who uses what). It is supposed to be used with a core::resource::cache_t.
template <typename T>
class handle {
  public:
	using value_type = std::remove_cv_t<std::remove_const_t<T>>;
	// using meta_type  = typename details::meta_type<value_type>::type;
	using meta_type	 = typename resource_traits<value_type>::meta_type;
	using alias_type = typename details::alias_type<value_type>::type;

  private:
	friend class cache_t;
	friend class weak_handle<T>;
	template <typename Y>
	friend class handle;

	handle(void* resource, cache_t* cache, metadata* metaData, psl::meta::file* meta) noexcept
		: m_Resource(reinterpret_cast<value_type*>(resource)), m_Cache(cache), m_MetaData(metaData),
		  m_MetaFile(reinterpret_cast<meta_type*>(meta)) {
		m_MetaData->reference_count += 1;
	};

  public:
	handle() = default;
	~handle() {
		if constexpr(!std::is_same_v<alias_type, void>) {
			static_assert(details::is_valid_alias<value_type, alias_type>::value);
		}
		if(m_MetaData) {
			psl_assert(m_MetaData->reference_count != 0,
					   "reference count was {}, but should be != 0",
					   m_MetaData->reference_count);
			m_MetaData->reference_count -= 1;
		}
	};

	template <typename... Ts>
	handle(const handle<alias<Ts...>>& other) noexcept : handle::handle(other.template get<T>()) {};


	template <typename Y>
	handle(const handle<Y>& other) noexcept {
		static_assert(details::alias_has_type<T, typename details::alias_type<value_type>::type>::value,
					  "type must be an alias");
		if(!other)
			return;
		handle<T> res = other.m_Cache->template find<T>(other.uid());
		m_Resource	  = res.m_Resource;
		m_Cache		  = res.m_Cache;
		m_MetaData	  = res.m_MetaData;
		m_MetaFile	  = res.m_MetaFile;
		if(m_MetaData)
			m_MetaData->reference_count += 1;
	};

	handle(const handle& other) noexcept
		: m_Resource(other.m_Resource), m_Cache(other.m_Cache), m_MetaData(other.m_MetaData),
		  m_MetaFile(other.m_MetaFile) {
		if(m_MetaData)
			m_MetaData->reference_count += 1;
	};

	handle(handle&& other) noexcept
		: m_Resource(other.m_Resource), m_Cache(other.m_Cache), m_MetaData(other.m_MetaData),
		  m_MetaFile(other.m_MetaFile) {
		other.m_MetaData = nullptr;
	};
	handle& operator=(const handle& other) {
		if(this != &other) {
			if(m_MetaData)
				m_MetaData->reference_count -= 1;
			m_Resource = other.m_Resource;
			m_Cache	   = other.m_Cache;
			m_MetaData = other.m_MetaData;
			m_MetaFile = other.m_MetaFile;
			if(m_MetaData)
				m_MetaData->reference_count += 1;
		}
		return *this;
	};
	handle& operator=(handle&& other) noexcept {
		if(this != &other) {
			if(m_MetaData)
				m_MetaData->reference_count -= 1;
			m_Resource = other.m_Resource;
			m_Cache	   = other.m_Cache;
			m_MetaData = other.m_MetaData;
			m_MetaFile = other.m_MetaFile;
			if(m_MetaData)
				m_MetaData->reference_count += 1;
		}
		return *this;
	};


	inline status state() const noexcept { return m_MetaData ? m_MetaData->state : status::invalid; }

	inline value_type& value() noexcept {
		psl_assert(state() == status::loaded,
				   "state was expected to be loaded, but was {}",
				   static_cast<std::underlying_type_t<status>>(state()));
		return *m_Resource;
	}

	inline const value_type& value() const noexcept {
		psl_assert(state() == status::loaded,
				   "state was expected to be loaded, but was {}",
				   static_cast<std::underlying_type_t<status>>(state()));
		return *m_Resource;
	}

	inline bool try_get(value_type& out) const noexcept {
		if(state() == status::loaded) {
			out = *m_Resource;
			return true;
		}
		return false;
	}

	template <typename Y>
	bool operator==(const handle<Y>& other) const noexcept {
		return uid() == other.uid();
	}
	template <typename Y>
	bool operator!=(const handle<Y>& other) const noexcept {
		return uid() != other.uid();
	}

	template <typename Y>
	bool operator==(const weak_handle<Y>& other) const noexcept {
		return uid() == other.uid();
	}
	template <typename Y>
	bool operator!=(const weak_handle<Y>& other) const noexcept {
		return uid() != other.uid();
	}

	inline operator bool() const noexcept { return state() == status::loaded; }

	inline meta_type* meta() const noexcept { return m_MetaFile; }
	inline metadata const* resource_metadata() const noexcept { return m_MetaData; }
	inline cache_t* cache() const noexcept { return m_Cache; }

	inline value_type const* operator->() const { return m_Resource; }
	inline value_type* operator->() { return m_Resource; }

	inline psl::UID uid() const noexcept { return m_MetaData ? m_MetaData->uid : psl::UID::invalid_uid; }
	inline const psl::UID& uid() noexcept { return m_MetaData ? m_MetaData->uid : psl::UID::invalid_uid; }


	operator const psl::UID&() const noexcept { return m_MetaData ? m_MetaData->uid : psl::UID::invalid_uid; }
	operator psl::view_ptr<meta_type>() const noexcept { return m_MetaFile; }
	operator tag<T>() const noexcept { return {m_MetaData ? m_MetaData->uid : psl::UID::invalid_uid}; }

  private:
	value_type* m_Resource {nullptr};
	core::resource::cache_t* m_Cache {nullptr};
	metadata* m_MetaData {nullptr};
	meta_type* m_MetaFile {nullptr};
};

template <typename... Ts>
class handle<alias<Ts...>> {
	friend class cache_t;

	template <size_t... indices>
	void internal_set(psl::array<cache_t::description*> descriptions,
					  psl::meta::file* meta,
					  std::index_sequence<indices...>) {
		(
		  [&descriptions, this, meta](psl::array<cache_t::description*>::iterator it) {
			  if(it != std::end(descriptions)) {
				  std::get<indices>(m_Resource) = handle<std::tuple_element_t<indices, std::tuple<Ts...>>> {
					(*it)->resource, m_Cache, &(*it)->metaData, meta};
			  }
		  }(std::find_if(std::begin(descriptions),
						 std::end(descriptions),
						 [key = details::key_for<std::tuple_element_t<indices, std::tuple<Ts...>>>()](
						   cache_t::description* descr) { return descr->metaData.type == key; })),
		  ...);
	}

	handle(psl::array<cache_t::description*> descriptions, cache_t* cache, psl::meta::file* meta) noexcept
		: m_Cache(cache) {
		internal_set(descriptions, meta, std::make_index_sequence<sizeof...(Ts)>());
	}

  public:
	using value_type = std::tuple<handle<Ts>...>;

	handle() = default;
	~handle() {};

	handle(const handle& other)				   = default;
	handle(handle&& other) noexcept			   = default;
	handle& operator=(const handle& other)	   = default;
	handle& operator=(handle&& other) noexcept = default;

	template <typename T>
	handle& operator<<(const handle<T>& data) {
		std::get<handle<T>>(m_Resource) = data;
		return *this;
	}

	template <typename T>
	void unset() noexcept {
		std::get<handle<T>>(m_Resource) = {};
	}

	template <typename T>
	void set(const handle<T>& data) {
		std::get<handle<T>>(m_Resource) = data;
	}

	template <size_t I>
	auto get() const noexcept {
		return std::get<I>(m_Resource);
	}

	template <typename T>
	auto get() const noexcept {
		return std::get<handle<T>>(m_Resource);
	}

	template <typename T>
	constexpr bool contains() const noexcept {
		return std::get<handle<T>>(m_Resource);
	}

	template <typename T>
	T& value() noexcept {
		return std::get<handle<T>>(m_Resource).value();
	}
	template <typename T>
	const T& value() const noexcept {
		return std::get<handle<T>>(m_Resource).value();
	}

	template <typename Fn, typename... Args, size_t... indices>
	void visit_all_impl(std::index_sequence<indices...>, Fn&& fn, Args&&... args) {
		(
		  [&fn](auto& handle, auto&&... values) {
			  if constexpr(std::is_invocable_v<Fn, size_t, decltype(handle.value()), decltype(values)...>) {
				  if(handle)
					  std::invoke(fn, indices, handle.value(), std::forward<decltype(values)>(values)...);
			  } else {
				  if(handle)
					  std::invoke(fn, handle.value(), std::forward<decltype(values)>(values)...);
			  }
		  }(std::get<indices>(m_Resource), std::forward<Args>(args)...),
		  ...);
	}

	template <typename Fn, typename... Args>
	void visit_all(Fn&& fn, Args&&... args) {
		visit_all_impl(std::make_index_sequence<sizeof...(Ts)>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
	}


	template <typename... T1s, typename Fn, typename... Args>
	void visit(Fn&& fn, Args&&... args) {
		(
		  [&fn](auto& handle, auto&&... args) {
			  if(handle)
				  std::invoke(fn, handle.value(), std::forward<decltype(args)>(args)...);
		  }(std::get<handle<T1s>>(m_Resource), std::forward<Args>(args)...),
		  ...);
	}

  private:
	value_type m_Resource;
	core::resource::cache_t* m_Cache {nullptr};
};

template <typename T>
class weak_handle final {
	friend class cache_t;

  public:
	using value_type = std::remove_cv_t<std::remove_const_t<T>>;
	using meta_type	 = typename resource_traits<value_type>::meta_type;

	weak_handle(const handle<T>& handle)
		: m_Resource(handle.m_Resource), m_Cache(handle.m_Cache), m_MetaData(handle.m_MetaData),
		  m_MetaFile(handle.m_MetaFile) {};

	weak_handle()								   = default;
	weak_handle(const weak_handle&)				   = default;
	weak_handle(weak_handle&&) noexcept			   = default;
	weak_handle& operator=(const weak_handle&)	   = default;
	weak_handle& operator=(weak_handle&&) noexcept = default;

	template <typename Y>
	bool operator==(const handle<Y>& other) const noexcept {
		return uid() == other.uid();
	}
	template <typename Y>
	bool operator!=(const handle<Y>& other) const noexcept {
		return uid() != other.uid();
	}

	template <typename Y>
	bool operator==(const weak_handle<Y>& other) const noexcept {
		return uid() == other.uid();
	}
	template <typename Y>
	bool operator!=(const weak_handle<Y>& other) const noexcept {
		return uid() != other.uid();
	}

	inline auto state() const noexcept { return m_MetaData ? m_MetaData->state : status::invalid; }
	inline operator bool() const noexcept { return state() == status::loaded; }

	inline value_type const* operator->() const { return m_Resource; }
	inline value_type* operator->() { return m_Resource; }

	value_type& value() noexcept { return *m_Resource; };
	const value_type& value() const noexcept { return *m_Resource; };

	inline psl::UID uid() const noexcept { return m_MetaData ? m_MetaData->uid : psl::UID::invalid_uid; }
	inline const psl::UID& uid() noexcept { return m_MetaData ? m_MetaData->uid : psl::UID::invalid_uid; }

	inline meta_type* meta() const noexcept { return m_MetaFile; }
	inline metadata const* resource_metadata() const noexcept { return m_MetaData; }
	inline cache_t* cache() const noexcept { return m_Cache; }

	operator const psl::UID&() const noexcept { return m_MetaData ? m_MetaData->uid : psl::UID::invalid_uid; }
	operator psl::view_ptr<meta_type>() const noexcept { return m_MetaFile; }
	operator tag<T>() const noexcept { return {m_MetaData ? m_MetaData->uid : psl::UID::invalid_uid}; }

	handle<T> make_shared() const noexcept {
		return handle<T>((void*)m_Resource, m_Cache, m_MetaData, (psl::meta::file*)m_MetaFile);
	}

  private:
	value_type* m_Resource {nullptr};
	core::resource::cache_t* m_Cache {nullptr};
	metadata* m_MetaData {nullptr};
	meta_type* m_MetaFile {nullptr};
};
}	 // namespace core::resource
