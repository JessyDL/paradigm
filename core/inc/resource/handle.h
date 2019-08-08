#pragma once
#include "fwd/resource/resource.h"
#include "meta.h"
#include "cache.h"
#include <memory>

namespace core::resource
{
	/// \brief wraps around a resource for sharing and management purposes.
	///
	/// resource handles are used to manage the lifetime of resources, and tracking
	/// dependencies (i.e. who uses what). It is supposed to be used with a core::resource::cache.
	template <typename T>
	class handle final
	{
		friend class resource::cache;

		friend class indirect_handle<T>;
		handle(resource::cache& cache, const psl::UID& uid, std::shared_ptr<details::container<T>>& container)
			: m_Cache(&cache), uid(uid), resource_uid(uid), m_Container(container){};

	  public:
		handle()
			: m_Cache(nullptr), uid(psl::UID::invalid_uid), resource_uid(uid){

															};

		handle(resource::cache& cache)
			: m_Cache(&cache), uid(cache.library().create().first),
			  m_Container(std::make_shared<details::container<T>>()), resource_uid(uid)
		{
			PROFILE_SCOPE(core::profiler)
			m_Cache->reg(details::container<T>::id, resource_uid, (std::shared_ptr<void>)(m_Container),
						 m_Container->m_State, m_Container->m_vTable);
		};

		/// \note only used in the scenario of "create_shared"
		handle(resource::cache& cache, const psl::UID& uid)
			: m_Cache(&cache), uid(uid), resource_uid(uid), m_Container(std::make_shared<details::container<T>>())
		{
			PROFILE_SCOPE(core::profiler)
			// todo need to subscribe to the library, somehow figure out what type of meta type I use
			m_Cache->reg(details::container<T>::id, resource_uid, (std::shared_ptr<void>)(m_Container),
						 m_Container->m_State, m_Container->m_vTable);
		};

		handle(resource::cache& cache, const psl::UID& uid, const psl::UID& resource)
			: m_Cache(&cache), uid(uid), resource_uid(resource), m_Container(std::make_shared<details::container<T>>())
		{
			PROFILE_SCOPE(core::profiler)
			// todo need to subscribe to the library, somehow figure out what type of meta type I use
			m_Cache->reg(details::container<T>::id, resource_uid, (std::shared_ptr<void>)(m_Container),
						 m_Container->m_State, m_Container->m_vTable);
		};

		/// \details special form of the constructor that will create a detached clone of the handle you use as the
		/// source. this will create a new resource, that branches off from the current state the other resource is in.
		/// it invokes the copy constructor of the contained type, and so if it is not present, this will not compile.
		template <typename... Args, typename = typename std::enable_if<std::is_constructible<
										T, const T&, const psl::UID&, resource::cache&, Args...>::value>::type>
		handle(deep_copy_t, const handle& other, Args&&... args)
			: m_Cache(other.m_Cache), uid(psl::UID::generate()),
			  resource_uid((other.m_Cache->library().is_physical_file(other.resource_uid)) ? other.resource_uid : uid),
			  m_Container(std::make_shared<details::container<T>>())
		{
			PROFILE_SCOPE(core::profiler)
			m_Cache->reg(details::container<T>::id, resource_uid, (std::shared_ptr<void>)(m_Container),
						 m_Container->m_State, m_Container->m_vTable);

			if(other.m_Container && other.m_Container->has_value())
			{
				m_Container->copy_set(*other.m_Container->resource(), uid, *m_Cache, std::forward<Args>(args)...);
				m_Container->m_State = other.m_Container->m_State;
			}
		};
		handle(const handle& other) = default;
		handle& operator=(const handle& other) = default;
		handle(handle&& other)				   = default;
		handle& operator=(handle&& other) = default;

		operator const T&() const
		{
			if(m_Container && m_Container->has_value())
			{
				return *(m_Container->resource());
			}

			throw std::runtime_error("tried to derefence a missing, or not loaded resource");
		}

		operator tag<T>() const { return tag<T>(uid); }

		/// \returns true of the handle has a valid, loaded object.
		operator bool() const { return uid && (m_Container) && m_Container->m_State == state::LOADED; }

		state resource_state() const noexcept { return (!m_Container) ? state::INVALID : m_Container->m_State; }

		bool operator==(const handle& other) { return other.uid == uid; }
		bool operator!=(const handle& other) { return other.uid != uid; }
		/// \returns true if the resource is loaded
		/// \brief loads the resource with the given arguments.
		/// \details loads the resource with the given arguments. In case
		/// the resource already exists, the arguments are ignored and the already
		/// loaded resource is returned instead.
		template <typename... Args>
		bool load(Args&&... args)
		{
			PROFILE_SCOPE(core::profiler)
			if constexpr(std::is_constructible<T, const psl::UID&, resource::cache&, psl::meta::file*,
													Args...>::value)
			{
				if(!uid) return false;

				if(m_Container->m_State == state::NOT_LOADED || m_Container->m_State == state::UNLOADED)
				{
					m_Container->m_State = state::LOADING;
					auto metaPtr		 = m_Cache->library().get(resource_uid);
					if(metaPtr && m_Container->set(uid, *m_Cache, metaPtr.value(), std::forward<Args>(args)...))
					{
						if constexpr(psl::serialization::details::is_collection<T>::value)
						{
							if(auto result = m_Cache->library().load(resource_uid); result)
							{
								psl::serialization::serializer s;
								psl::format::container cont{result.value()};
								s.deserialize<psl::serialization::decode_from_format, T>(*(m_Container->resource()),
																						 cont);
							}
						}
						m_Container->m_State = state::LOADED;
					}
					else
					{
						m_Container->m_State = state::INVALID;
					}
				}

				return m_Container->m_State == state::LOADED;
			}
			else if constexpr(std::is_constructible<T, const psl::UID&, resource::cache&, Args...>::value)
			{
				if(!uid) return false;

				if(m_Container->m_State == state::NOT_LOADED || m_Container->m_State == state::UNLOADED)
				{
					m_Container->m_State = state::LOADING;
					if(m_Container->set(uid, *m_Cache, std::forward<Args>(args)...))
					{
						if constexpr(psl::serialization::details::is_collection<T>::value)
						{
							if(auto result = m_Cache->library().load(resource_uid); result)
							{
								psl::serialization::serializer s;
								psl::format::container cont{result.value()};
								s.deserialize<psl::serialization::decode_from_format, T>(*(m_Container->resource()),
																						 cont);
							}
						}
						m_Container->m_State = state::LOADED;
					}
					else
					{
						m_Container->m_State = state::INVALID;
					}
				}

				return m_Container->m_State == state::LOADED;
			}
			else
			{
				static_assert(std::is_destructible_v<T>, "no destructor was provided, one should be provided");
				static_assert(
					utility::templates::always_false_v<T>,
					"there was no suitable constructor for the given type that would accept these arguments.");
			}
		}

		T* operator->() { return (m_Container && m_Container->has_value()) ? m_Container->resource() : nullptr; }


		const T* operator->() const
		{
			return (m_Container && m_Container->has_value()) ? m_Container->resource() : nullptr;
		}

		const T* cvalue() const
		{
			return (m_Container && m_Container->has_value()) ? m_Container->resource() : nullptr;
		}

		T& value()
		{
			if(m_Container && m_Container->has_value())
			{
				return *(m_Container->resource());
			}

			throw std::runtime_error("tried to derefence a missing, or not loaded resource");
		}

		/// \returns true on success.
		/// \param[in] force forcibly unload regardless of dependencies.
		/// \details >ill try to unload the resource. In case that this is the last handle to the resource, it will
		/// clean itself up and return success, otherwise it will fail and return false. It will also fail when the
		/// resource is not yet loaded, and it will _not_ unload when it finally is loaded, you'll have to call this
		/// again. \warning Regardless of success, the handle will become invalid.
		bool unload(bool force = false)
		{
			PROFILE_SCOPE(core::profiler)
			m_Container.reset();
			return m_Cache->reset<T>(resource_uid, force);
		}

		/// \returns the UID assigned to the handle
		const psl::UID& ID() const noexcept { return uid; }

		/// \returns the resource UID, this is shared between all handles of different types that are based on the same
		/// resource.
		/// \note multiple handles (different UID's) can point to the same RUID. Simple example is a
		/// handle<TextFile> and handle<Shader> could point to the same file on disk, but are very much different
		/// resources. The same is true for handles of the same type.
		/// \note the ID() and RUID() are the same for generated resources, as then the "Resource UID" is considered
		/// the memory location of the handle.
		const psl::UID& RUID() const noexcept { return resource_uid; }

		template <typename... Args>
		typename std::enable_if<std::is_constructible<T, const T&, const psl::UID&, resource::cache&, Args...>::value,
								handle<T>>::type
		copy(resource::cache& cache, Args&&... args) const
		{
			PROFILE_SCOPE(core::profiler)
			auto res = (m_Cache->library().is_physical_file(resource_uid)
							? handle<T>(cache, psl::UID::generate(), resource_uid)
							: handle<T>(cache));

			if(m_Container && m_Container->has_value())
			{
				res.m_Container->copy_set(*m_Container->resource(), res.uid, cache, std::forward<Args>(args)...);
				res.m_Container->m_State = m_Container->m_State;
			}


			return res;
		}

		const resource::cache& cache() const noexcept { return *m_Cache; };
		resource::cache& cache() noexcept { return *m_Cache; };

	  private:
		resource::cache* m_Cache;
		psl::UID uid;		   // my actual UID
		psl::UID resource_uid; // the disk based resource I'm based on, this can be the same like my actual uid if I'm a
							   // shared resource, or it can be the same if I'm not a physical file.
		std::shared_ptr<details::container<T>> m_Container;
	};

	template <typename T>
	static handle<T> create(cache& cache)
	{
		PROFILE_SCOPE_STATIC(core::profiler)
		return handle<T>(cache);
	}

	/// \brief creates a new resource based on the given UID resource.
	///
	/// if this resource is disk-based it might speed up loading due to using the already cached value (todo)
	/// The resulting resource has a new UID.
	template <typename T>
	static handle<T> create(cache& cache, const psl::UID& uid)
	{
		PROFILE_SCOPE_STATIC(core::profiler)
		return handle<T>{cache, psl::UID::generate(), uid};
	}

	/// \details will either find the resource with the given UID, and return that, or create a new one using that UID.
	/// this function should be used for resources that might be shared (like ivk::material), co-owned in disconnected
	/// systems, or read-only.
	///
	/// The resulting resource keeps the UID you create it with, meaning it can be found again using this function.
	template <typename T>
	static handle<T> create_shared(cache& cache, const psl::UID& uid)
	{
		PROFILE_SCOPE_STATIC(core::profiler)
		auto res = cache.find<T>(uid);
		if(res) return res;
		return handle<T>(cache, uid);
	}

	template <typename T, typename Y>
	static handle<T> create_shared(cache& cache, const core::resource::handle<Y>& base)
	{
		PROFILE_SCOPE_STATIC(core::profiler)
		auto res = cache.find<T>(base.RUID());
		if(res) return res;
		return handle<T>(cache, base.RUID());
	}

	/// \details this will create a disconnected copy based on the source handle. This means that, for example, when the
	/// source gets destroyed, this copy will keep existing, and when changes happen to either the source or copy, the
	/// changes will not be applied to the other.
	template <typename T, typename... Args>
	static handle<T> copy(cache& cache, const handle<T>& source, Args&&... args)
	{
		PROFILE_SCOPE_STATIC(core::profiler)
		static_assert(std::is_constructible<T, const T&, const psl::UID&, resource::cache&, Args...>::value,
					  "lacking a 'copy' constructor on T, cannot create a new handle with the given source.");
		return source.copy(cache, std::forward<Args>(args)...);
	}

	template <typename T>
	class indirect_handle
	{
	  public:
		indirect_handle() = default;
		indirect_handle(const psl::UID& uid, cache* cache) : m_Cache(cache), m_UID(uid){};
		indirect_handle(const handle<T>& handle) : m_Cache(handle.m_Cache), m_UID(handle.uid){};

		operator const core::resource::handle<T>() const noexcept { return m_Cache->find<T>(m_UID); }
		operator core::resource::handle<T>() noexcept { return m_Cache->find<T>(m_UID); }
		operator const T&() const noexcept { return m_Cache->find<T>(m_UID); }
		operator T&() noexcept { return m_Cache->find<T>(m_UID); }

		operator const psl::UID&() const noexcept { return m_UID; }

		operator psl::UID() noexcept { return m_UID; }
		operator tag<T>() const { return tag<T>(m_UID); }

		core::resource::handle<T> handle() const noexcept { return m_Cache->find<T>(m_UID); }

		const T& operator->() const noexcept { return m_Cache->find<T>(m_UID); }

		T& operator->() noexcept { return m_Cache->find<T>(m_UID); }

		const psl::UID& uid() const noexcept { return m_UID; };

	  private:
		cache* m_Cache{nullptr};
		psl::UID m_UID{};
	};
}