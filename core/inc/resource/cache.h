#pragma once
#include "fwd/resource/resource.h"
#include "library.h"
#include "memory/region.h"
#include "profiling/profiler.h"

namespace core::resource
{
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

		struct vtable
		{
			void (*clear)(void* this_);
			/*resource_key_t (*ID)(void* this_);*/
		};


		template <typename T>
		details::vtable const vtable_for = {[](void* this_) { static_cast<T*>(this_)->clear(); }/*,

											[](void* this_) -> resource_key_t { return T::id(); }*/};
	} // namespace details
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
			entry(resource_key_t id, std::shared_ptr<void> container, state& state, const details::vtable& table)
				: id(id), container(container), state(state), m_Table(table){};
			entry(const entry& other)
				: id(other.id), container(other.container), state(other.state), m_Table(other.m_Table){};
			resource_key_t id;
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
				if(e.id == details::container<T>::id())
					return handle<T>(*this, uid, *((std::shared_ptr<details::container<T>>*)(&e.container)));
			}

			return handle<T>(*this, psl::UID::invalid_uid);
		}

	  private:
		template <typename T>
		friend handle<T> core::resource::create_shared(cache& cache, const psl::UID& uid);
		std::optional<entry* const> get(const psl::UID& uid, resource_key_t ID)
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
				if(e.id == details::container<T>::id() && (e.container.use_count() == 1 || force) &&
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

		void reg(resource_key_t id, const psl::UID& uid, std::shared_ptr<void> container, state& state,
				 const details::vtable& vtable)
		{
			auto& vec = m_Handles[uid];
			if(std::find_if(std::begin(vec), std::end(vec), [id](const auto& data) { return data.id == id; }) ==
			   std::end(vec))
			{
				vec.emplace_back(id, container, state, vtable);
			}
		}

		psl::meta::library m_Library;
		// memory::region m_Region;
		memory::allocator_base* m_Allocator;
		std::unordered_map<psl::UID, std::vector<entry>> m_Handles;
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

			static resource_key_t id() noexcept { return details::key_for<T>(); }

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
		const details::vtable& container<T>::m_vTable = details::vtable_for<container<T>>;
	} // namespace details
} // namespace core::resource