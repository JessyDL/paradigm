#pragma once
#include <cstdint>	 // uintptr_t
#include <type_traits> // std::remove_const/etc
#include "meta.h"
#include "library.h"
#include "logging.h"
#include "serialization.h"
#include "static_array.h"

/*
resource requirements:

resource can be "alias" for a variant-like type, i.e. core::gfx::texture (which can be either core::ivk::texture or
core::igles::texture) can be either, or both at the same time. const resource support (as well as resource reference).

async loading (using std::future)

explicit instantiation (i.e. distinguishment between on-disk loading and creation)
"load" operations are immediate, but can be deferred.

trivial copyable version
tag-only version

resource has weak/strong mechanics

all operations through cache (create/destroy/etc)
functional operations with cache as first parameter

unsafe on multi threads (ref count), but safe on singular threads
variation that does allow multithreaded safe ref count (opt-in).


explore:
should handles contain their meta?
should objects be able to query a cache to figure out their handle data?

*/

namespace psl
{
	template <class InputIt, class OutputIt, class Pred, class Fct>
	void transform_if(InputIt first, InputIt last, OutputIt dest, Pred pred, Fct transform)
	{
		while(first != last)
		{
			if(pred(*first)) *dest++ = transform(*first);

			++first;
		}
	}
} // namespace psl

namespace core::r2
{
	template <typename... Ts>
	struct alias
	{};

	template <typename T>
	class handle;

	template <typename... Ts>
	class alias_handle;

	using resource_key_t = const std::uintptr_t* (*)();
	namespace details
	{
		template <typename T, typename Y>
		struct is_resource_alias : std::false_type
		{};

		// added to trick the compiler to not throw away the results at compile time
		template <typename T>
		constexpr const std::uintptr_t resource_key_var{0u};

		template <typename T>
		constexpr const std::uintptr_t* resource_key() noexcept
		{
			return &resource_key_var<T>;
		}

		template <typename T>
		constexpr resource_key_t key_for()
		{
			return resource_key<typename std::decay<T>::type>;
		};

		template <typename T>
		class container;

		template <typename T, typename SFINAE = void>
		struct meta_type
		{
			using type = psl::meta::file;
		};

		template <typename T>
		struct meta_type<T, std::void_t<typename T::meta_type>>
		{
			using type = typename T::meta_type;
		};

		template <typename C, typename T>
		struct is_valid_alias : std::false_type
		{};

		template <typename C, typename... Ts>
		struct is_valid_alias<C, alias<Ts...>>
		{
			static const bool value{std::is_constructible_v<C, handle<alias<Ts...>>&>};
		};

		template <typename T, typename SFINAE = void>
		struct alias_type : std::false_type
		{
			using type = void;
		};


		template <typename T>
		struct alias_type<T, std::void_t<typename T::alias_type>> : std::true_type
		{
			using type	= typename T::alias_type;
			using alias_t = handle<type>;
		};

		template <typename... Ts>
		constexpr psl::static_array<resource_key_t, sizeof...(Ts)> keys_for(handle<alias<Ts...>>*)
		{
			return {key_for<Ts>()...};
		}
	} // namespace details

	enum class type
	{
		shared = 0,
		weak   = 1,
		strong = 2
	};

	enum class state : int
	{
		initial   = 0,
		loading   = 1,
		loaded	= 2,
		unloading = 3,
		unloaded  = 4,
		missing   = -1,
		/// \brief special sentinel state in case something disastrous has happened, like OOM
		invalid = -2
	};


	struct metadata
	{
		psl::UID uid;
		psl::UID resource_uid;
		resource_key_t type;
		state state;
		size_t reference_count;
		bool strong_type;
	};

	namespace details
	{

		template <typename T>
		class container
		{
		  public:
			container()  = default;
			~container() = default;

			container(const container& other)	 = delete;
			container(container&& other) noexcept = delete;
			container& operator=(const container& other) = delete;
			container& operator=(container&& other) noexcept = delete;

		  private:
		};
	} // namespace details

	class cache
	{
	  public:
		struct description
		{
			metadata metaData;
			void* resource{nullptr};
		};
		struct entry
		{
			psl::array<description*> descriptions;
			psl::meta::file* metaFile;
		};

	  public:
		cache(psl::meta::library library) : m_Library(std::move(library)){};
		~cache() { free(true); };

		cache(const cache& other)	 = delete;
		cache(cache&& other) noexcept = default;
		cache& operator=(const cache& other) = delete;
		cache& operator=(cache&& other) noexcept = default;

		template <typename T, typename... Args>
		handle<T> instantiate(const psl::UID& resource_uid, Args&&... args)
		{
			return instantiate_using<T>(psl::UID::generate(), resource_uid, std::forward<Args>(args)...);
		}
		template <typename T, typename... Args>
		handle<T> instantiate_using(const psl::UID& uid, const psl::UID& resource_uid, Args&&... args)
		{
			using value_type   = std::remove_cv_t<std::remove_const_t<T>>;
			using meta_type	= typename handle<T>::meta_type;
			constexpr auto key = details::key_for<value_type>();
			if(auto it = m_Deleters.find(key); it == std::end(m_Deleters))
			{
				m_Deleters[key] = [](void* resource) { delete((T*)(resource)); };
			}
			auto& data = m_Cache[uid];

			auto& descr = *data.descriptions.emplace_back(
				new description{metadata{uid, resource_uid, key, state::initial, 0u, false}, nullptr});

			if(data.metaFile == nullptr)
			{
				if(auto optMetaFile = m_Library.get<meta_type>(resource_uid); optMetaFile)
					data.metaFile = static_cast<psl::meta::file*>(optMetaFile.value());
				else
				{
					core::log->error("could not load");
					descr.metaData.state = state::missing;
					return {nullptr, this, &descr.metaData, nullptr};
				}
			}

			auto task = [&descr, &cache = *this, &library = m_Library, metaFile = data.metaFile](auto&&... values) {
				descr.metaData.state = state::loading;
				T* resource			 = nullptr;
				if constexpr(std::is_constructible_v<T, r2::cache&, const metadata&, meta_type*, decltype(values)...>)
					resource =
						new T{cache, descr.metaData, (meta_type*)metaFile, std::forward<decltype(values)>(values)...};
				else if constexpr(std::is_constructible_v<T, r2::cache&, psl::UID, decltype(values)...>)
					resource = new T{cache, descr.metaData.uid, std::forward<decltype(values)>(values)...};
				else
					resource = new T{std::forward<decltype(values)>(values)...};
				if constexpr(psl::serialization::details::is_collection<T>::value)
				{
					if(auto result = library.load(descr.metaData.resource_uid); result)
					{
						psl::serialization::serializer s;
						psl::format::container cont{result.value()};
						s.deserialize<psl::serialization::decode_from_format, T>(*resource, cont);
					}
					else
					{
						core::log->error("could not load");
						descr.metaData.state = state::missing;
						return;
					}
				}
				descr.resource		 = (void*)resource;
				descr.metaData.state = state::loaded;
			};
			std::invoke(task, std::forward<Args>(args)...);

			return {descr.resource, this, &descr.metaData, data.metaFile};
		}

		template <typename T, typename... Args>
		handle<T> create(Args&&... args);


		template <typename T, typename... Args>
		handle<T> create_using(const psl::UID& uid, Args&&... args)
		{
			using value_type   = std::remove_cv_t<std::remove_const_t<T>>;
			using meta_type	= typename handle<T>::meta_type;
			constexpr auto key = details::key_for<value_type>();
			if(auto it = m_Deleters.find(key); it == std::end(m_Deleters))
			{
				m_Deleters[key] = [](void* resource) { delete((T*)(resource)); };
			}

			auto& data = m_Cache[uid];
			if(data.metaFile == nullptr)
			{
				data.metaFile = static_cast<psl::meta::file*>(&m_Library.create<meta_type>(uid).second);
			}


			auto& descr = *data.descriptions.emplace_back(new description{
				metadata{uid, psl::UID::invalid_uid, details::key_for<value_type>(), state::initial, 0u, false},
				nullptr});


			auto task = [&descr, &cache = *this](auto&&... values) {
				descr.metaData.state = state::loading;
				T* resource			 = nullptr;
				if constexpr(std::is_constructible_v<T, r2::cache&, psl::UID, decltype(values)...>)
					resource = new T{cache, descr.metaData.uid, std::forward<decltype(values)>(values)...};
				else
					resource = new T{std::forward<decltype(values)>(values)...};
				descr.resource		 = (void*)resource;
				descr.metaData.state = state::loaded;
			};
			std::invoke(task, std::forward<Args>(args)...);

			return {descr.resource, this, &descr.metaData, data.metaFile};
		}

		template <typename T, typename... Args>
		handle<T> find(const psl::UID& uid) noexcept
		{
			auto it = m_Cache.find(uid);
			if(it == std::end(m_Cache)) return {};

			constexpr auto key = details::key_for<T>();
			for(auto& entry : it->second.descriptions)
			{
				if(entry->metaData.type == key) return {entry->resource, this, &entry->metaData, it->second.metaFile};
			}

			if constexpr(details::alias_type<T>::value)
			{
				using alias_t			  = typename details::alias_type<T>::alias_t;
				constexpr auto alias_keys = details::keys_for((alias_t*)(nullptr));

				psl::array<description*> eligable;
				std::copy_if(std::begin(it->second.descriptions), std::end(it->second.descriptions),
							 std::back_inserter(eligable), [&alias_keys](description* descr) {
								 return std::find(std::begin(alias_keys), std::end(alias_keys), descr->metaData.type) !=
										std::end(alias_keys);
							 });

				if(eligable.size() == 0) return {};


				/*for(auto& entry : it->second.descriptions)
				{
					if(std::find(std::begin(alias_keys), std::end(alias_keys), entry->metaData.type) !=
					   std::end(alias_keys))
					{
						
						handle<std::tuple_element_t<0, alias_t>> handle(entry->resource, this, &entry->metaData,
																		it->second.metaFile);

						result.set()
						return create_using<T>(entry->metaData.uid, handle);
					}
				}*/

				alias_t result{eligable, this, it->second.metaFile};
				return create_using<T>(uid, result);
			}
			return {};
		}

		void free(bool clear_all = false)
		{
			LOG_INFO("destroying cache start");
			bool bErased	 = false;
			size_t iteration = 0u;
			bool bLeaks		 = false;
			do
			{
				bErased		 = false;
				bLeaks		 = false;
				size_t count = 0u;
				for(auto& pair : m_Cache)
				{
					for(auto& it : pair.second.descriptions)
					{
						bLeaks |= it->metaData.state == state::loaded;
						if(it->metaData.reference_count == 0 && it->metaData.state == state::loaded)
						{
							it->metaData.state = state::unloading;
							std::invoke(m_Deleters[it->metaData.type], it->resource);
							it->metaData.state = state::unloaded;
							bErased			   = true;
							++count;
						}
					}
				}
				LOG_INFO("iteration ", utility::to_string(iteration++), " destroyed ", utility::to_string(count),
						 " objects");
			} while(bErased); // keep looping as long as items are present


			if(bLeaks && clear_all)
			{
				LOG_WARNING("leaks detected!");

				for(auto& pair : m_Cache)
				{
					for(auto& it : pair.second.descriptions)
					{
						if(it->metaData.state == state::loaded)
						{
#ifdef DEBUG_CORE_RESOURCE

							core::log->warn("\ttype {0} uid {1}", m_TypeNames[it->metaData.type],
											utility::to_string(pair.first));
#else
							core::log->warn("\ttype {0} uid {1}", utility::to_string((std::uintptr_t)(it.id)),
											utility::to_string(pair.first));
#endif
						}
					}
				}
			}
			core::log->flush();

			LOG_INFO("destroying cache end");
		}

	  private:
		psl::meta::library m_Library;
		std::unordered_map<psl::UID, entry> m_Cache{};
		std::unordered_map<resource_key_t, psl::string8_t> m_TypeNames;
		std::unordered_map<resource_key_t, std::function<void(void*)>> m_Deleters;
	};

	namespace details
	{}

	template <typename T>
	class handle
	{
		friend class cache;
		using value_type = std::remove_cv_t<std::remove_const_t<T>>;
		using meta_type  = typename details::meta_type<value_type>::type;
		using alias_type = typename details::template alias_type<value_type>::type;


	  public:
		handle() = default;
		~handle()
		{
			if constexpr(!std::is_same_v<alias_type, void>)
			{
				static_assert(details::is_valid_alias<value_type, alias_type>::value);
			}
			if(m_MetaData) m_MetaData->reference_count -= 1;
		};

		handle(void* resource, cache* cache, metadata* metaData, psl::meta::file* meta) noexcept
			: m_Resource(reinterpret_cast<value_type*>(resource)), m_Cache(cache), m_MetaData(metaData),
			  m_MetaFile(reinterpret_cast<meta_type*>(meta))
		{
			m_MetaData->reference_count += 1;
		};

		handle(const handle& other) noexcept
			: m_Resource(other.m_Resource), m_Cache(other.m_Cache), m_MetaData(other.m_MetaData),
			  m_MetaFile(other.m_MetaFile)
		{
			if(m_MetaData) m_MetaData->reference_count += 1;
		};

		handle(handle&& other) noexcept
			: m_Resource(other.m_Resource), m_Cache(other.m_Cache), m_MetaData(other.m_MetaData),
			  m_MetaFile(other.m_MetaFile){};
		handle& operator=(const handle& other)
		{
			if(this != &other)
			{
				if(m_MetaData) m_MetaData->reference_count -= 1;
				m_Resource = other.m_Resource;
				m_Cache	= other.m_Cache;
				m_MetaData = other.m_MetaData;
				m_MetaFile = other.m_MetaFile;
				if(m_MetaData) m_MetaData->reference_count += 1;
			}
			return *this;
		};
		handle& operator=(handle&& other) noexcept
		{
			if(this != &other)
			{
				if(m_MetaData) m_MetaData->reference_count -= 1;
				m_Resource = other.m_Resource;
				m_Cache	= other.m_Cache;
				m_MetaData = other.m_MetaData;
				m_MetaFile = other.m_MetaFile;
				if(m_MetaData) m_MetaData->reference_count += 1;
			}
			return *this;
		};

		inline state state() const noexcept { return m_MetaData ? m_MetaData->state : state::invalid; }

		inline value_type& value()
		{
			assert(state() == state::loaded);
			return *m_Resource;
		}

		inline bool try_get(value_type& out) noexcept
		{
			if(state() == state::loaded)
			{
				out = *m_Resource;
				return true;
			}
			return false;
		}

		operator bool() const noexcept { return state() == state::loaded; }

		meta_type* meta() const noexcept { return m_MetaFile; }
		metadata const* resource_metadata() const noexcept { return m_MetaData; }
		cache* cache() const noexcept { return m_Cache; }

		value_type const* operator->() const { return m_Resource; }
		value_type* operator->() { return m_Resource; }

	  private:
		value_type* m_Resource{nullptr};
		core::r2::cache* m_Cache{nullptr};
		metadata* m_MetaData{nullptr};
		meta_type* m_MetaFile{nullptr};
	};

	template <typename... Ts>
	class handle<alias<Ts...>>
	{
	  public:
		using value_type = std::tuple<handle<Ts>...>;

		template <size_t... indices>
		void internal_set(psl::array<cache::description*> descriptions, psl::meta::file* meta,
						  std::index_sequence<indices...>)
		{
			([&descriptions, this, meta](psl::array<cache::description*>::iterator it) 
				{
				if(it != std::end(descriptions))
				{
					std::get<indices>(m_Resource) = handle<std::tuple_element_t<indices, std::tuple<Ts...>>>{
						(*it)->resource, m_Cache, &(*it)->metaData, meta};
				}
				}(std::find_if(std::begin(descriptions),  std::end(descriptions), [key = details::key_for<std::tuple_element_t<indices, std::tuple<Ts...>>>()](cache::description* descr){
				return descr->metaData.type == key;})), ...);
		}

		handle(psl::array<cache::description*> descriptions, cache* cache, psl::meta::file* meta) noexcept
			: m_Cache(cache)
		{
			internal_set(descriptions, meta, std::make_index_sequence<sizeof...(Ts)>());
		}
		handle(handle& data) : m_Resource(data.m_Resource) {}

		handle()  = default;
		~handle() = default;

		template <typename T>
		void set(handle<T> data)
		{
			std::get<handle<T>>(m_Resource) = data;
		}
		template <typename T>
		void set(handle<T>& data)
		{
			std::get<handle<T>>(m_Resource) = data;
		}

		template <size_t I>
		auto get()
		{
			return std::get<I>(m_Resource);
		}

		template <typename T>
		auto get()
		{
			return std::get<handle<T>>(m_Resource);
		}

		template <typename Fn, typename... Args, size_t... indices>
		void visit_all_impl(std::index_sequence<indices...>, Fn&& fn, Args&&... args)
		{
			(
				[&fn](auto& handle, auto&&... values) {
					if(handle) std::invoke(fn, handle.value(), std::forward<decltype(values)>(values)...);
				}(std::get<indices>(m_Resource), std::forward<Args>(args)...),
				...);
			//(void(std::invoke(fn, std::get<indices>(m_Resource), std::forward<Args>(args)...)), ...);
		}

		template <typename Fn, typename... Args>
		void visit_all(Fn&& fn, Args&&... args)
		{
			visit_all_impl(std::make_index_sequence<sizeof...(Ts)>(), std::forward<Fn>(fn),
						   std::forward<Args>(args)...);
		}

		template <typename... Ts, typename Fn, typename... Args>
		void visit(Fn&& fn, Args&&... args)
		{
			(
				[&fn](auto& handle, auto&&... args) {
					if(handle) std::invoke(fn, handle.value(), std::forward<decltype(args)>(args)...);
				}(std::get<handle<Ts>>(m_Resource), std::forward<Args>(args)...),
				...);
		}

		value_type m_Resource;
		core::r2::cache* m_Cache{nullptr};
	};


	template <typename T, typename... Args>
	handle<T> cache::create(Args&&... args)
	{
		return create_using<T>(psl::UID::generate(), std::forward<Args>(args)...);
	}
} // namespace core::r2