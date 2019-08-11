#pragma once

//#include "handle.h"
#include <variant>
//#include "async/async.hpp"

namespace core::resource
{
	namespace details
	{

		template <typename T>
		struct get_container_type
		{};

		template <typename T>
		struct get_container_type<std::shared_ptr<core::resource::details::container<T>>>
		{
			using type = T;
		};
	} // namespace details
	template <typename... Ts>
	class handle<std::variant<Ts...>>
	{
		using resource_type = std::variant<Ts...>;
		friend class resource::cache;

		friend class indirect_handle<resource_type>;
		friend class handle;
		template <typename T>
		handle(resource::cache& cache, const psl::UID& uid, std::shared_ptr<details::container<T>>& container)
			: m_Cache(&cache), uid(uid), resource_uid(uid), m_Container(container){};

	  public:
		handle() = default;

		template <typename T>
		handle(handle<T> _handle)
			: m_Key(details::key_for<T>()), m_Cache(_handle.m_Cache), uid(_handle.uid),
			  resource_uid(_handle.resource_uid), m_Container(_handle.m_Container)
		{}
		handle(resource::cache& cache) : m_Cache(&cache), uid(cache.library().create().first), resource_uid(uid){};
		handle(resource::cache& cache, psl::UID uid, psl::UID resource_uid)
			: m_Cache(&cache), uid(uid), resource_uid(resource_uid){};
		~handle() = default;

		handle(const handle& other)		= default;
		handle(handle&& other) noexcept = default;
		handle& operator=(const handle& other) = default;
		handle& operator=(handle&& other) noexcept = default;

		template <typename T, typename... Args>
		bool load(Args&&... args)
		{
			assert(m_Key == nullptr);
			m_Container = std::make_shared<details::container<T>>();
			m_Key		= details::key_for<T>();
			auto h		= get<T>();
			h.registrate();

			return h.load(std::forward<Args>(args)...);
		}

		template <typename T>
		handle<T> get()
		{
			handle<T> v;
			v.m_Cache	  = m_Cache;
			v.uid		   = uid;
			v.resource_uid = resource_uid;

			assert(m_Key == details::key_for<T>());

			if(auto res = std::get_if<std::shared_ptr<details::container<T>>>(&m_Container))
			{
				v.m_Container = *res;
			}
			return v;
		}

		template <typename T>
		const T& value() const
		{
			if(auto res = std::get_if<std::shared_ptr<details::container<T>>>(&m_Container)) return *(*res)->resource();
			throw std::runtime_error("incorrect cast");
		}

		template <typename T>
		T& value()
		{
			if(auto res = std::get_if<std::shared_ptr<details::container<T>>>(&m_Container)) return *(*res)->resource();
			throw std::runtime_error("incorrect cast");
		}

		template <typename T>
		bool contains() const noexcept
		{
			return m_Key == details::key_for<T>();
		}

		operator tag<resource_type>() const { return tag<resource_type>(uid); }

		/// \returns true of the handle has a valid, loaded object.
		operator bool() const { return uid && m_Key != nullptr && resource_state() == state::LOADED; }

		state resource_state() const noexcept
		{
			return (m_Key == nullptr)
					   ? state::INVALID
					   : std::visit(
							 [this](auto&& container) {
								 using type = typename details::get_container_type<
									 std::remove_const_t<std::remove_cv_t<std::decay_t<decltype(container)>>>>::type;
								 return get<type>().resource_state();
							 },
							 m_Container);
		}

		bool unload(bool force = false)
		{
			assert(m_Key != nullptr);

			auto res = std::visit(
				[this, force](auto&& container) {
					using type = typename details::get_container_type<
						std::remove_const_t<std::remove_cv_t<std::decay_t<decltype(container)>>>>::type;
					return get<type>().unload(force);
				},
				m_Container);
			m_Key = nullptr;
			return res;
		}

	  private:
		resource_key_t m_Key = {nullptr};
		resource::cache* m_Cache;
		psl::UID uid;		   // my actual UID
		psl::UID resource_uid; // the disk based resource I'm based on, this can be the same like my actual uid if I'm a
							   // shared resource, or it can be the same if I'm not a physical file.

		std::variant<std::shared_ptr<details::container<Ts>>...> m_Container;
	};
} // namespace core::resource

// namespace core::resource2
//{
//	/// tag to signify a resource UID
//	/// \details sometimes you want to receive a resource UID, but not an
//	/// actual handle. The tag type gives you _some_ protection when doing so.
//	template <typename T>
//	struct tag final
//	{
//		psl::UID uid;
//	};
//
//
//	enum class handle_type
//	{
//		shared	= 0,
//		strong	= 1,
//		reference = 2
//	};
//
//	class cache;
//
//	template <typename T, handle_type h_type>
//	class handle;
//
//	enum class state
//	{
//		NOT_LOADED = 0,
//		UNLOADING  = 1,
//		LOADING	= 2,
//		FINALIZING = 3,
//		LOADED	 = 4,
//		MISSING	= -1,
//		/// \brief special sentinel state in case something disastrous has happened, like OOM
//		INVALID = -2
//	};
//
//	using resource_key_t = const std::uintptr_t* (*)();
//	namespace details
//	{
//		// added to trick the compiler to not throw away the results at compile time
//		template <typename T>
//		constexpr const std::uintptr_t resource_key_var{0u};
//
//		template <typename T>
//		constexpr const std::uintptr_t* resource_key() noexcept
//		{
//			return &resource_key_var<T>;
//		}
//
//		template <typename T>
//		constexpr resource_key_t key_for()
//		{
//			return resource_key<typename std::decay<T>::type>;
//		};
//
//		using reference_counter = size_t*;
//
//		class container_base
//		{
//		  public:
//			container_base() = delete;
//
//			container_base(resource_key_t key) : m_Key(key){};
//			virtual ~container_base() = default;
//
//			template <typename T>
//			bool is_type() const noexcept
//			{
//				return m_Key == details::key_for<T>();
//			}
//
//			bool is_type(resource_key_t key) const noexcept { return m_Key == key; }
//
//			state state() const noexcept { return m_State; };
//			psl::UID uid() const noexcept { return m_UID; };
//
//			virtual void reset() noexcept = 0;
//
//			reference_counter counter() noexcept { return &m_Counter; }
//
//		  private:
//			resource_key_t m_Key;
//			psl::UID m_UID;
//			size_t m_Counter{0};
//
//		  protected:
//			core::resource2::state m_State;
//		};
//
//		template <typename T>
//		class container final : public container_base
//		{
//			friend class cache;
//
//		  public:
//			container(memory::allocator_base* allocator) : container_base(key_for<T>()), m_Allocator(allocator){};
//			~container()
//			{
//				m_Resource->~T();
//				m_Resource = nullptr;
//				if(m_Segment) m_Allocator->deallocate(m_Segment.value());
//			}
//
//			container(const container& other)	 = delete;
//			container(container&& other) noexcept = delete;
//			container& operator=(const container& other) = delete;
//			container& operator=(container&& other) noexcept = delete;
//
//			template <typename... Args>
//			void load(Args&&... args)
//			{
//				m_State = state::LOADING;
//				if(m_Segment = m_Allocator->allocate(sizeof(T)); !m_Segment)
//					throw std::runtime_error("could not allocate in the region");
//#ifdef DBG_NEW
//#undef new
//#endif
//				m_Resource = new((char*)m_Segment.value().range().begin) T(std::forward<Args>(args)...);
//#ifdef DBG_NEW
//#define new DBG_NEW
//#endif
//				m_State = state::LOADED;
//			}
//
//			void reset() noexcept override
//			{
//				m_State	= state::UNLOADING;
//				m_Resource = nullptr;
//				if(m_Segment) m_Allocator->deallocate(m_Segment.value());
//				m_State = state::NOT_LOADED;
//			}
//			operator T&() { return *m_Resource; }
//
//			T& get() { return *m_Resource; }
//
//		  private:
//			T* m_Resource;
//			memory::allocator_base* m_Allocator;
//			std::optional<memory::segment> m_Segment;
//		};
//	} // namespace details
//
//
//	class cache final
//	{
//		struct data_block
//		{
//			size_t size;
//			size_t alignment;
//			memory::block_allocator* allocator;
//		};
//
//	  public:
//		cache(psl::meta::library&& library, memory::allocator_base* allocator)
//			: m_Library(std::move(library)), m_Allocator(allocator){};
//		~cache() = default;
//
//		cache(const cache& other)	 = default;
//		cache(cache&& other) noexcept = default;
//		cache& operator=(const cache& other) = default;
//		cache& operator=(cache&& other) noexcept = default;
//
//		template <typename T>
//		void create()
//		{
//			auto it = m_Data.find(details::key_for<T>());
//			if(it == std::end(m_Data))
//			{
//				m_Data.emplace(details::key_for<T>(), {});
//				it = m_Data.find(details::key_for<T>());
//			}
//			data_block& block = it->second;
//			auto segment	  = block.allocator->allocate(0);
//		}
//
//		template <typename T, handle_type h_type>
//		handle<T, h_type> get_handle(const T& target) const noexcept
//		{
//			auto it			  = m_Data.find(details::key_for<T>());
//			data_block& block = it->second;
//
//			/*
//			check if pointer falls in the range
// //
//			*/
//		}
//
//	  private:
//		psl::meta::library m_Library;
//		memory::allocator_base* m_Allocator;
//		std::unordered_map<resource_key_t, data_block> m_Data;
//	};
//
//	template <typename T, handle_type h_type = handle_type::shared>
//	class handle final
//	{
//	  public:
//		handle()  = default;
//		~handle() = default;
//
//		handle(const handle& other)		= default;
//		handle(handle&& other) noexcept = default;
//		handle& operator=(const handle& other) = default;
//		handle& operator=(handle&& other) noexcept = default;
//	};
//
//	template <typename T>
//	class handle<T, handle_type::reference> final
//	{
//	  public:
//		handle()  = default;
//		~handle() = default;
//
//		handle(const handle& other)		= default;
//		handle(handle&& other) noexcept = default;
//		handle& operator=(const handle& other) = default;
//		handle& operator=(handle&& other) noexcept = default;
//
//	  private:
//	};
//
//	template <typename T>
//	class handle<T, handle_type::strong> final
//	{
//	  public:
//		handle()  = default;
//		~handle() = default;
//
//		handle(const handle& other)		= delete;
//		handle(handle&& other) noexcept = default;
//		handle& operator=(const handle& other) = delete;
//		handle& operator=(handle&& other) noexcept = default;
//	};
//
//	template <typename T, handle_type h_type>
//	handle<T, h_type> create(cache& cache)
//	{}
//} // namespace core::resource2