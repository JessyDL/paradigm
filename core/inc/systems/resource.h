#pragma once

#include "meta.h"
#include "library.h"
#include "memory/region.h"
#include "memory/allocator.h"
#include "logging.h"

namespace core::resource
{
	// creates a deep_copy of the handle that will be detached from the original handle
	struct deep_copy_t
	{
		explicit deep_copy_t() = default;
	};
	constexpr deep_copy_t deep_copy{};


	/// \brief represents the various states a resource can be in.
	enum class state
	{
		NOT_LOADED = 0,
		LOADING	= 1,
		LOADED	 = 2,
		UNLOADING  = 3,
		UNLOADED   = 4,
		MISSING	= -1,
		/// \brief special sentinel state in case something disastrous has happened, like OOM
		INVALID = -2
	};


	class cache;

	template <typename T>
	class handle;


	template<typename T>
	class tag
	{
	public:
		tag(const psl::UID& uid) : m_UID(uid) {};
		~tag() = default;
		tag(const tag&) = default;
		tag(tag&&) = default;
		tag& operator=(const tag&) = default;
		tag& operator=(tag&&) = default;

		operator const psl::UID&() const noexcept{return m_UID;};
		const psl::UID& uid() const noexcept { return m_UID;};
	private:
		psl::UID m_UID{};
	};

	template <typename T, bool use_custom_uid = false>
	class dependency
	{
	  public:
		constexpr bool custom_uid() const { return use_custom_uid; }
	};


	namespace details
	{
		template <typename T>
		class container;

		struct vtable
		{
			void (*clear)(void* this_);
			const uint64_t& (*ID)(void* this_);
		};


		template <typename T>
		details::vtable const vtable_for = {[](void* this_) { static_cast<T*>(this_)->clear(); },

											[](void* this_) -> const uint64_t& { return static_cast<T*>(this_)->id; }};

		template <typename T>
		class packet
		{
		  public:
			packet() { static_assert(utility::templates::always_false_v<T>, "unsupported type"); }
		};

		template <>
		class packet<psl::UID>
		{
		  public:
			packet(const psl::UID& uid) noexcept : m_UID(uid){};
			packet() noexcept : m_UID(psl::UID::invalid_uid){};
			packet(const packet& other) noexcept : m_UID(other.m_UID){};
			packet(packet&& other) noexcept : m_UID(other.m_UID){};
			packet& operator=(const packet& other) noexcept
			{
				if(this != &other)
				{
					m_UID = other.m_UID;
				}
				return *this;
			};
			packet& operator=(packet&& other) noexcept
			{
				if(this != &other)
				{
					m_UID = other.m_UID;
				}
				return *this;
			};

			psl::UID& value() { return m_UID; }
			const psl::UID& uid() const noexcept { return m_UID; }

		  private:
			  psl::UID m_UID;
		};


		template <>
		class packet<core::resource::cache>
		{
		  public:
			packet(core::resource::cache& cache) noexcept : m_Cache(&cache){};
			packet() noexcept : m_Cache(nullptr){};
			packet(const packet& other) noexcept : m_Cache(other.m_Cache){};
			packet(packet&& other) noexcept : m_Cache(other.m_Cache){};
			packet& operator=(const packet& other) noexcept
			{
				if(this != &other)
				{
					m_Cache = other.m_Cache;
				}
				return *this;
			};
			packet& operator=(packet&& other) noexcept
			{
				if(this != &other)
				{
					m_Cache = other.m_Cache;
				}
				return *this;
			};

			core::resource::cache& cache() const { return *m_Cache; };
			core::resource::cache& value() { return *m_Cache; }

		  private:
			core::resource::cache* m_Cache;
		};

		template <typename T, bool use_custom_uid>
		class packet<dependency<T, use_custom_uid>>
		{
		  public:
			packet() = default;
			static constexpr bool uses_custom_uid() { return use_custom_uid; }

			core::resource::handle<T>& value() { return handle; }

			core::resource::handle<T> handle{};
		};


		template <typename, typename = void>
		struct has_resource_dependency : std::false_type
		{};
		template <typename T>
		struct has_resource_dependency<T, std::void_t<typename T::resource_dependency*>> : std::true_type
		{};
	} // namespace details

	template <typename... Ts>
	class packet
	{
	  public:
		packet(){};
		~packet() = default;

		template <typename T>
		T& get()
		{
			return std::get<details::packet<T>>(data).value();
		}
		std::tuple<details::packet<Ts>...> data{};
	};

	namespace details
	{
		template <typename T>
		struct is_packet : std::false_type
		{};
		template <typename... Ts>
		struct is_packet<packet<Ts...>> : std::true_type
		{};


		template <typename T, typename Tuple>
		struct has_type;

		template <typename T>
		struct has_type<T, std::tuple<>> : std::false_type
		{};

		template <typename T, typename U, typename... Ts>
		struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>>
		{};

		template <typename T, typename... Ts>
		struct has_type<T, std::tuple<T, Ts...>> : std::true_type
		{};


		// template<typename PackType, typename... Ts>
		// struct has_packet : public has_type < details::packet<PackType>, typename decltype(
		// std::declval<packet<Ts...>>().data) > {};


		template <typename PackType, typename T>
		struct has_packet : public has_type<details::packet<PackType>, decltype(std::declval<T>().data)>
		{};
	} // namespace details


	// forward declare the static function as it needs access to cache internals
	class cache;
	template <typename T>
	static handle<T> create_shared(cache& cache, const psl::UID& uid);


	/// \brief represents a container of resources and their UID mappings
	///
	/// The resource cache is a specialized container that can handle lifetime and ID mapping
	/// of resources. It also controls the memory that the resource will be allocated to.
	/// This means that every resource is guaranteed to be part of atleast one cache.
	/// Resource can also access other resources within the same cache with ease through the
	/// shared psl::meta::library instance. Libraries can be shared between caches, but remember that
	/// neither the psl::meta::library nor core::resource::cache are thread-safe by themselves.
	/// \todo check multi-cache
	class cache final
	{
		template <typename T>
		friend class handle;
		template <typename T>
		friend class details::container;

		struct entry
		{
			entry(uint64_t id, std::shared_ptr<void> container, state& state, const details::vtable& table)
				: id(id), container(container), state(state), m_Table(table){};
			entry(const entry& other)
				: id(other.id), container(other.container), state(other.state), m_Table(other.m_Table){};
			const uint64_t id;
			state& state;
			std::shared_ptr<void> container;
			const details::vtable& m_Table;
		};

	  public:
		/*cache(psl::meta::library&& library, size_t size, size_t alignment,
			  memory::allocator_base* allocator = new memory::default_allocator(true))
			: m_Library(std::move(library)), m_Region(size, alignment, allocator)
		{
			LOG_INFO("creating cache");
		};
*/
		cache(psl::meta::library&& library, memory::allocator_base* allocator);

		cache(const cache&) = delete;
		cache(cache&&)		= delete;
		cache& operator=(const cache&) = delete;
		cache& operator=(cache&&) = delete;

		~cache();

		void free(bool recursive = true);


		// const memory::region& region() const { return m_Region; };
		psl::meta::library& library() { return m_Library; };
		memory::allocator_base* allocator() const { return m_Allocator; };

		template <typename T>
		handle<T> find(const psl::UID& uid)
		{
			auto it = m_Handles.find(uid);
			if(it == m_Handles.end()) return handle<T>(*this, psl::UID::invalid_uid);

			for(auto& e : it->second)
			{
				if(e.id == details::container<T>::id)
					return handle<T>(*this, uid, *((std::shared_ptr<details::container<T>>*)(&e.container)));
			}
			return handle<T>(*this, psl::UID::invalid_uid);
		}

	  private:
		template <typename T>
		friend handle<T> core::resource::create_shared(cache& cache, const psl::UID& uid);
		std::optional<entry* const> get(const psl::UID& uid, uint64_t ID)
		{
			const auto& it = m_Handles.find(uid);
			if(it == std::end(m_Handles)) return {};
			for(auto& c : it->second)
			{
				if(c.id == ID) return &c;
			}

			return {};
		}


		template <typename T>
		bool reset(const psl::UID& uid, bool force = false)
		{
			PROFILE_SCOPE(core::profiler)
			auto it = m_Handles.find(uid);
			if(it == m_Handles.end()) return false;

			for(auto& e : it->second)
			{
				if(e.id == details::container<T>::id && (e.container.use_count() == 1 || force) &&
				   e.state == state::LOADED)
				{
					e.state = state::UNLOADING;
					e.m_Table.clear(e.container.get());
					e.state = state::UNLOADED;
					return true;
				}
			}
			return false;
		}

		void reg(uint64_t id, const psl::UID& uid, std::shared_ptr<void> container, state& state,
				 const details::vtable& vtable)
		{
			m_Handles[uid].emplace_back(id, container, state, vtable);
		}

		psl::meta::library m_Library;
		// memory::region m_Region;
		memory::allocator_base* m_Allocator;
		std::unordered_map<psl::UID, std::vector<entry>> m_Handles;
		static uint64_t m_ID;
	};
	namespace details
	{
		template <typename T>
		class container
		{
			friend class handle<T>;

		  public:
			container() = default;
			~container() { clear(); }

			bool has_value() const { return m_Resource && segment; }

			template <typename... Args>
			bool set(psl::UID& uid, cache& cache, Args&&... args)
			{
				if(m_Resource && segment) // already set
					return false;

				if(auto region = cache.allocator()->allocate(sizeof(T)); region)
				{

#ifdef DBG_NEW
#undef new
#endif
					m_Resource = std::unique_ptr<T>(new((char*)(region.value().range().begin))
														T(uid, cache, std::forward<Args>(args)...));
					allocator  = cache.allocator();
					segment	= region.value();
#ifdef DBG_NEW
#define new DBG_NEW
#endif
					return true;
				}

				return false;
			}

			template <typename... Args>
			bool set(cache& cache, Args&&... args)
			{
				if(m_Resource && segment) // already set
					return false;

				PROFILE_SCOPE(core::profiler)
				if(auto region = cache.allocator()->allocate(sizeof(T)); region)
				{

#ifdef DBG_NEW
#undef new
#endif
					m_Resource =
						std::unique_ptr<T>(new((char*)(region.value().range().begin)) T(std::forward<Args>(args)...));
					allocator = cache.allocator();
					segment   = region.value();
#ifdef DBG_NEW
#define new DBG_NEW
#endif
					return true;
				}

				return false;
			}

			T* resource() { return m_Resource.get(); }
			void clear()
			{
				if(m_Resource && segment)
				{
					PROFILE_SCOPE(core::profiler)
					LOG_INFO("(container destroy start)");
					auto ptr = m_Resource.get();
					ptr->T::~T();
					m_Resource.release();
					allocator->deallocate(segment.value());
					m_Resource = nullptr;
					segment	= {};
					allocator  = nullptr;
					LOG_INFO("(container destroy end)");
				}
			}

			static const uint64_t id;

		  private:
			template <typename... Args>
			bool copy_set(const T& source, psl::UID& uid, cache& cache, Args&&... args)
			{
				if(m_Resource && segment) // already set
					return false;
				PROFILE_SCOPE(core::profiler)

				if(auto region = cache.allocator()->allocate(sizeof(T)); region)
				{

#ifdef DBG_NEW
#undef new
#endif
					m_Resource = std::unique_ptr<T>(new((char*)(region.value().range().begin))
														T(source, uid, cache, std::forward<Args>(args)...));
					allocator  = cache.allocator();
					segment	= region.value();
#ifdef DBG_NEW
#define new DBG_NEW
#endif
					return true;
				}

				return false;
			}

			state m_State{state::NOT_LOADED};
			std::unique_ptr<T> m_Resource = nullptr;
			memory::allocator_base* allocator{nullptr};
			std::optional<memory::segment> segment;

			static const details::vtable& m_vTable;
		};


		template <typename T>
		const uint64_t container<T>::id{cache::m_ID++};

		template <typename T>
		const details::vtable& container<T>::m_vTable = details::vtable_for<container<T>>;
	} // namespace details

	template<typename T>
	class indirect_handle;

	/// \brief wraps around a resource for sharing and management purposes.
	///
	/// resource handles are used to manage the lifetime of resources, and tracking
	/// dependencies (i.e. who uses what). It is supposed to be used with a core::resource::cache.
	template <typename T>
	class handle final
	{
		friend class cache;

		friend class indirect_handle<T>;
		handle(cache& cache, const psl::UID& uid, std::shared_ptr<details::container<T>>& container)
			: m_Cache(&cache), uid(uid), resource_uid(uid), m_Container(container){};

	  public:
		handle()
			: m_Cache(nullptr), uid(psl::UID::invalid_uid), resource_uid(uid){

													   };

		handle(cache& cache)
			: m_Cache(&cache), uid(cache.library().create().first),
			  m_Container(std::make_shared<details::container<T>>()), resource_uid(uid)
		{
			PROFILE_SCOPE(core::profiler)
			m_Cache->reg(details::container<T>::id, resource_uid, (std::shared_ptr<void>)(m_Container),
						 m_Container->m_State, m_Container->m_vTable);
		};

		/// \note only used in the scenario of "create_shared"
		handle(cache& cache, const psl::UID& uid)
			: m_Cache(&cache), uid(uid), resource_uid(uid), m_Container(std::make_shared<details::container<T>>())
		{
			PROFILE_SCOPE(core::profiler)
			// todo need to subscribe to the library, somehow figure out what type of meta type I use
			m_Cache->reg(details::container<T>::id, resource_uid, (std::shared_ptr<void>)(m_Container),
						 m_Container->m_State, m_Container->m_vTable);
		};

		handle(cache& cache, const psl::UID& uid, const psl::UID& resource)
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
		template <typename... Args, typename = typename std::enable_if<
										std::is_constructible<T, const T&, const psl::UID&, cache&, Args...>::value>::type>
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
		handle(handle&& other) = default;
		handle& operator=(handle&& other) = default;

		operator const T&() const
		{
			if(m_Container && m_Container->has_value())
			{
				return *(m_Container->resource());
			}

			throw std::runtime_error("tried to derefence a missing, or not loaded resource");
		}

		operator tag<T>() const
		{
			return tag<T>(uid);
		}

		/// \returns true of the handle has a valid, loaded object.
		operator bool() const { return uid && (m_Container) && m_Container->m_State == state::LOADED; }

		state resource_state() const noexcept { return (!m_Container) ? state::INVALID : m_Container->m_State; }

		bool operator==(const handle& other)
		{
			return other.uid == uid;
		}
		bool operator!=(const handle& other)
		{
			return other.uid != uid;
		}
		/// \returns true if the resource is loaded
		/// \brief loads the resource with the given arguments.
		/// \details loads the resource with the given arguments. In case
		/// the resource already exists, the arguments are ignored and the already
		/// loaded resource is returned instead.
		template <typename... Args>
		bool load(Args&&... args)
		{
			PROFILE_SCOPE(core::profiler)
			if constexpr(details::has_resource_dependency<T>::value)
			{
				if constexpr(!std::is_constructible<T, typename T::resource_dependency, Args...>::value)
				{
					static_assert(
						utility::templates::always_false_v<T>,
						"there was no suitable constructor for the given type that would accept these arguments.");
				}
				typename T::resource_dependency pac;
				if constexpr(details::has_packet<psl::UID, typename T::resource_dependency>::value)
				{
					std::get<details::packet<psl::UID>>(pac.data) = details::packet<psl::UID>(uid);
				}
				if constexpr(details::has_packet<core::resource::cache, typename T::resource_dependency>::value)
				{
					std::get<details::packet<core::resource::cache>>(pac.data) =
						details::packet<core::resource::cache>(*m_Cache);
				}

				if(!uid) return false;

				if(m_Container->m_State == state::NOT_LOADED || m_Container->m_State == state::UNLOADED)
				{
					m_Container->m_State = state::LOADING;
					if(m_Container->set(*m_Cache, pac, std::forward<Args>(args)...))
					{
						if constexpr(psl::serialization::details::is_collection<T>::value)
						{
							if(auto result = m_Cache->library().load(resource_uid); result)
							{
								psl::serialization::serializer s;
								psl::format::container cont{result.value()};
								s.deserialize<psl::serialization::decode_from_format, T>(*(m_Container->resource()), cont);
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
			else if constexpr(std::is_constructible<T, const psl::UID&, cache&, Args...>::value)
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
								s.deserialize<psl::serialization::decode_from_format, T>(*(m_Container->resource()), cont);
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
			else if constexpr(std::is_constructible<T, const psl::UID&, cache&, psl::meta::file*, Args...>::value)
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
								s.deserialize<psl::serialization::decode_from_format, T>(*(m_Container->resource()), cont);
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
		typename std::enable_if<std::is_constructible<T, const T&, const psl::UID&, cache&, Args...>::value, handle<T>>::type
		copy(cache& cache, Args&&... args) const
		{
			PROFILE_SCOPE(core::profiler)
			auto res =
				(m_Cache->library().is_physical_file(resource_uid) ? handle<T>(cache, psl::UID::generate(), resource_uid)
																   : handle<T>(cache));

			if(m_Container && m_Container->has_value())
			{
				res.m_Container->copy_set(*m_Container->resource(), res.uid, cache, std::forward<Args>(args)...);
				res.m_Container->m_State = m_Container->m_State;
			}


			return res;
		}


	  private:
		cache* m_Cache;
		psl::UID uid;		  // my actual UID
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
	/// this function should be used for resources that might be shared (like gfx::material), co-owned in disconnected
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

	/// \details this will create a disconnected copy based on the source handle. This means that, for example, when the source
	/// gets destroyed, this copy will keep existing, and when changes happen to either the source or copy, the changes
	/// will not be applied to the other.
	template <typename T, typename... Args>
	static handle<T> copy(cache& cache, const handle<T>& source, Args&&... args)
	{
		PROFILE_SCOPE_STATIC(core::profiler)
		static_assert(std::is_constructible<T, const T&, const psl::UID&, resource::cache&, Args...>::value,
					  "lacking a 'copy' constructor on T, cannot create a new handle with the given source.");
		return source.copy(cache, std::forward<Args>(args)...);
	}

	template<typename T>
	class indirect_handle
	{
	public:
		indirect_handle() = default;
		indirect_handle(const psl::UID& uid, cache* cache)  : m_Cache(cache), m_UID(uid) {};
		indirect_handle(const handle<T>& handle) : m_Cache(handle.m_Cache), m_UID(handle.uid) {};

		operator const core::resource::handle<T>() const noexcept
		{
			return m_Cache->find<T>(m_UID);
		}
		operator core::resource::handle<T>() noexcept
		{
			return m_Cache->find<T>(m_UID);
		}
		operator const T&() const noexcept
		{
			return m_Cache->find<T>(m_UID);
		}
		operator T&() noexcept
		{
			return m_Cache->find<T>(m_UID);
		}

		operator const psl::UID&() const noexcept
		{
			return m_UID;
		}

		operator psl::UID() noexcept
		{
			return m_UID;
		}
		operator tag<T>() const
		{
			return tag<T>(m_UID);
		}

		core::resource::handle<T> handle() const noexcept
		{
			return m_Cache->find<T>(m_UID);
		}

		const T& operator->() const noexcept
		{
			return m_Cache->find<T>(m_UID);
		}

		T& operator->() noexcept
		{
			return m_Cache->find<T>(m_UID);
		}


	private:
		cache* m_Cache{nullptr};
		psl::UID m_UID{};
	};


} // namespace core::resource
