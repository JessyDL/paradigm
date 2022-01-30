#pragma once
#include <cppcoro/detail/is_awaiter.hpp>
#include <cppcoro/resume_on.hpp>
#include <cppcoro/schedule_on.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>

#include <functional>
#include <memory>
#include <unordered_map>

#include <psl/library.hpp>
#include <psl/meta.hpp>

#include <psl/template_utils.hpp>

// todo alias handles
// todo args_t to package up cache/resource related data
// todo constructor args forwarding

namespace core::_v1
{
	namespace resource
	{
		class cache_t;

		template <typename T>
		struct resource_traits
		{
			using meta_type									= psl::meta::file;
			static constexpr bool always_send_resource_pack = false;
			using alias_type								= psl::type_pack_t<>;
			/*
			using alias_type = std::tuple<>;
			using resource_data_types = foo;
			*/
		};

		inline namespace details
		{
			struct resource_info_t
			{
				psl::UID uid;
				psl::meta::file* m_MetaData;
			};

			template <typename T>
			concept HasResourceDataTypesLocal = requires
			{
				typename T::resource_data_types;
			};

			template <typename T>
			concept HasResourceDataTypesTrait = requires
			{
				typename resource_traits<T>::resource_data_types;
			};

			template <typename T>
			struct resource_type_deduction : std::false_type
			{
				using type = T;
			};

			template <HasResourceDataTypesLocal T>
			struct resource_type_deduction<T> : std::true_type
			{
				using type = typename T::resource_data_types;
			};

			template <HasResourceDataTypesTrait T>
			struct resource_type_deduction<T> : std::true_type
			{
				using type = typename resource_traits<T>::resource_data_types;
			};

			template <typename T>
			concept HasResourceDataTypes = HasResourceDataTypesLocal<T> || HasResourceDataTypesTrait<T>;

			template <typename T>
			concept HasMetaFileOverride = requires
			{
				typename T::meta_file;
			};

			template <typename T>
			struct meta_file_override
			{
				using type = typename resource_traits<T>::meta_type;
			};

			template <HasMetaFileOverride T>
			struct meta_file_override<T>
			{
				using type = resource_type_deduction<T>::type::meta_file;
			};

			template <typename T>
			concept HasResourcePackOverride = requires
			{
				{
					T::always_send_resource_pack
					} -> std::same_as<bool>;
			};

			template <typename T>
			struct always_send_resource_pack
			{
				static constexpr auto value = resource_traits<T>::always_send_resource_pack;
			};

			template <HasResourcePackOverride T>
			struct always_send_resource_pack<T>
			{
				static constexpr auto value = T::always_send_resource_pack;
			};

			template <typename T>
			static constexpr auto always_send_resource_pack_v = always_send_resource_pack<T>::value;

			template <typename T>
			concept IsCoroutineAwaitable = cppcoro::detail::is_awaiter<T>::value;

			template <typename T>
			concept IsCoroutineScheduler = requires(T t)
			{
				{
					t.schedule()
					} -> IsCoroutineAwaitable;
			};

			/// \brief tests wether the given type has a construct method that accepts the given arguments, and returns
			/// a void coroutine task.
			template <typename T, typename... Args>
			concept IsConstructEnabled = requires(T t, Args... args)
			{
				{
					t.construct(args...)
					} -> std::same_as<cppcoro::task<>>;
			};

			template <template <typename> typename Container>
			class handle_info_base_t
			{
			  protected:
				constexpr handle_info_base_t() noexcept = default;
				handle_info_base_t(const Container<resource_info_t>& info) noexcept : m_Info(info) {}

			  public:
				handle_info_base_t(const handle_info_base_t&)	  = default;
				handle_info_base_t(handle_info_base_t&&) noexcept = default;
				handle_info_base_t& operator=(const handle_info_base_t&) = default;
				handle_info_base_t& operator=(handle_info_base_t&&) noexcept = default;
				~handle_info_base_t()										 = default;

				const auto& uid() const noexcept { return m_Info->uid; }

				bool operator==(const handle_info_base_t& other) const noexcept { return m_Info->uid == other.uid(); }
				bool operator!=(const handle_info_base_t& other) const noexcept { return m_Info->uid != other.uid(); }

				operator const psl::UID&() const noexcept { return m_Info->uid; }
				operator bool() const noexcept { return m_Info != nullptr && m_Info->uid != psl::UID::invalid_uid; }

			  protected:
				Container<resource_info_t> m_Info {};
			};


			template <typename T>
			class handle_resource_base_t
			{
			  protected:
				using value_type	= std::remove_cvref_t<T>;
				using pointer_type	= value_type*;
				using cpointer_type = const value_type*;

				constexpr handle_resource_base_t() noexcept = default;
				constexpr handle_resource_base_t(pointer_type resource) noexcept : m_Resource(resource) {}

			  public:
				auto operator->() noexcept requires !std::is_const_v<T> { return m_Resource; }
				auto operator->() const noexcept { return m_Resource; }

				auto& get() { return *m_Resource; }
				constexpr operator bool() const noexcept { return m_Resource != nullptr; }

			  protected:
				pointer_type m_Resource {nullptr};
			};


			template <typename T>
			class weak_handle_typed_base_t;

			template <typename T>
			class shared_handle_typed_base_t :
				public handle_info_base_t<std::shared_ptr>,
				public handle_resource_base_t<T>
			{
				using info_base_t	  = handle_info_base_t<std::shared_ptr>;
				using resource_base_t = handle_resource_base_t<T>;

			  public:
				shared_handle_typed_base_t() = default;
				shared_handle_typed_base_t(const std::shared_ptr<resource_info_t>& info,
										   typename resource_base_t::pointer_type resource) :
					info_base_t(info),
					resource_base_t(resource)
				{}

				constexpr auto weak_handle() const noexcept -> weak_handle_typed_base_t<T>
				{
					return {info_base_t::m_Info, resource_base_t::m_Resource};
				}

				operator bool() const noexcept
				{
					return info_base_t::operator bool() && resource_base_t::operator bool();
				}
			};

			template <typename T>
			class weak_handle_typed_base_t : public handle_info_base_t<std::weak_ptr>, public handle_resource_base_t<T>
			{
				using info_base_t	  = handle_info_base_t<std::weak_ptr>;
				using resource_base_t = handle_resource_base_t<T>;

			  public:
				weak_handle_typed_base_t() = default;
				weak_handle_typed_base_t(const std::shared_ptr<resource_info_t>& info,
										 typename resource_base_t::pointer_type resource) :
					info_base_t(info),
					resource_base_t(resource)
				{}

				constexpr auto shared_handle() const noexcept -> shared_handle_typed_base_t<T>
				{
					if(auto shared = info_base_t::m_Info.lock())
						return {shared, resource_base_t::m_Resource};
					else
						return {};
				}

				operator bool() const noexcept
				{
					return info_base_t::operator bool() && resource_base_t::operator bool();
				}
			};

			template <typename T>
			struct resource_variant_entry_t
			{
				T* ptr {nullptr};
			};

			template <typename... Ts>
			struct resource_variant_t : private resource_variant_entry_t<Ts>...
			{
				template <typename T>
				resource_variant_t(T* ptr) :
					resource_variant_entry_t<T>::resource_variant_entry_t(ptr), active_index(psl::index_of_v<T, Ts...>)
				{}

				template <typename Fn>
				auto visit(Fn&& fn) const
				{
					return visit_impl<Fn, Ts...>(std::forward<Fn>(fn));
				}

				template <typename Fn>
				auto visit(Fn&& fn)
				{
					return visit_impl<Fn, Ts...>(std::forward<Fn>(fn));
				}

				template <typename Fn, typename Y, typename... Ys>
				auto visit_impl(Fn&& fn) const
				{
					if(psl::index_of_v<Y, Ts...> == active_index)
					{
						return fn(*resource_variant_entry_t<Y>::ptr);
					}
					else
						visit_impl<Fn, Ys...>(std::forward<Fn>(fn));
				}

				template <typename Fn, typename Y, typename... Ys>
				auto visit_impl(Fn&& fn)
				{
					if(psl::index_of_v<Y, Ts...> == active_index)
					{
						return fn(*resource_variant_entry_t<Y>::ptr);
					}
					else
						visit_impl<Fn, Ys...>(std::forward<Fn>(fn));
				}

				size_t active_index {0};
			};

			template <typename... Ts>
			class alias_handle_container_t
			{
			  protected:
				using resource_type = resource_variant_t<handle_resource_base_t<Ts>...>;

			  public:
				template <typename T>
				alias_handle_container_t(T* resource) : m_Resource(resource)
				{}

			  private:
				resource_type m_Resource {};
			};

			template <typename... Ts>
			class shared_alias_handle_typed_base_t :
				public handle_info_base_t<std::shared_ptr>,
				public alias_handle_container_t<Ts...>
			{};


			template <typename... Ts>
			class weak_alias_handle_typed_base_t :
				public handle_info_base_t<std::weak_ptr>,
				public alias_handle_container_t<Ts...>
			{};
		}	 // namespace details

		/// \brief a co-owning, and reference counted handle to a resource
		/// \tparam T type of resource the handle refers to
		template <typename T>
		using handle_t = shared_handle_typed_base_t<T>;

		/// \brief a non-owning handle to a resource
		/// \tparam T type of resource the handle refers to
		/// \warning don't use this to refer to a resource other than within a contained local scope, anyone could
		/// invoke the cache's memory management functionality which could clear this resource. If you want a more
		/// persistent lifetime guarantee, use a handle<T>
		template <typename T>
		using weak_handle_t = weak_handle_typed_base_t<T>;

		template <typename T>
		struct args_t
		{
			cache_t& cache;
			const typename meta_file_override<T>::type& meta;
		};

		template <HasResourceDataTypes T>
		struct args_t<T>
		{
			cache_t& cache;
			const typename meta_file_override<T>::type& meta;
			const typename resource_type_deduction<T>::type& data;
		};

		/// \brief Container class to load and manage resources.
		/// \note Invocations of load*/create*/copy* alter cache_t state on the calling thread, not on the scheduler.
		/// This means that if you wish to access the cache instance over multiple threads, you will have to synchronize
		/// externally when invoking these.
		/// \warning load* methods will always return the "pristine" version off of the `psl::meta::library`, but their
		/// UID will _not_ match what was requested. This is because loaded resources can be mutated and so might differ
		/// from what their "original state" once was for various reasons. To share a variation of a disk based
		/// resource, use the `find()` method instead.
		/// \todo extend the load*/create*/copy* functionality to pre-allocate on the calling thread to fully avoid any
		/// race condition potentials
		class cache_t
		{
			struct immutable_cache_entry_t
			{
				~immutable_cache_entry_t() { deleter(resource); }
				void* resource {nullptr};
				std::function<void(void*)> deleter {};
			};

			struct cache_entry_t : public immutable_cache_entry_t
			{
				std::shared_ptr<resource_info_t> info {std::make_shared<resource_info_t>()};
			};

			template <typename T>
			inline static std::function<void(void*)> deleter_fn = [](void* ptr) { delete((T*)ptr); };

			template <typename T>
			cppcoro::task<T*> coro_load_resource_data(const psl::UID& uid,
													  auto& entry) requires(!HasResourceDataTypes<T>)
			{
				co_return load_resource_data<T>(uid, entry);
			}

			template <typename T>
			T* load_resource_data(const psl::UID& uid, auto& entry) requires(!HasResourceDataTypes<T>)
			{
				if(m_Library->contains(uid))
				{
					auto src_data = m_Library->load(uid);
					psl::serialization::serializer s;
					psl::format::container cont {src_data.value()};

					T* data		   = new T();
					entry.resource = data;
					entry.deleter  = deleter_fn<T>;
					s.deserialize<psl::serialization::decode_from_format, T>(*data, cont);
					return data;
				}
				throw std::runtime_error("could not find the requested UID in the library");
				return (T*)nullptr;
			}

			template <typename T, typename... Args>
			auto
			load_no_resource_fn(auto& entry, auto& metafile, const psl::UID& res_uid, auto& res_entry, Args&&... args)
			  -> cppcoro::task<handle_t<T>>
			{
				using resource_data_t = typename resource_type_deduction<T>::type;
				auto res_data		  = co_await coro_load_resource_data<resource_data_t>(res_uid, res_entry);
				args_t<T> argpack {*this, metafile, *res_data};
				co_return co_await load_fn<T, true>(entry, argpack, std::forward<Args>(args)...);
			}

			template <typename T, bool IsCoro, typename... Args>
			auto load_fn(auto& entry, args_t<T>& argpack, Args&&... args)
			  -> std::conditional_t<IsCoro, cppcoro::task<handle_t<T>>, handle_t<T>>
			{
				T* value {nullptr};

				if constexpr(IsConstructEnabled<T, args_t<T>, Args...>)
				{
					if constexpr(resource_traits<T>::always_send_resource_pack)
					{
						value = new T(argpack);
					}
					else
					{
						value = new T();
					}
					if constexpr(IsCoro)
					{
						co_await value->construct(argpack, std::forward<Args>(args)...);
					}
					else
					{
						cppcoro::sync_wait(value->construct(argpack, std::forward<Args>(args)...));
					}
				}
				else
				{
					value = new T(argpack, std::forward<Args>(args)...);
				}

				entry.resource = reinterpret_cast<void*>(value);
				entry.deleter  = deleter_fn<T>;
				if constexpr(IsCoro)
				{
					co_return handle_t<T> {entry.info, value};
				}
				else
				{
					return handle_t<T> {entry.info, value};
				}
			}

			template <bool CoroContext = false, typename T, typename... Args>
			auto create_fn(auto& entry, args_t<T>& pack, Args&&... args)
			  -> std::conditional_t<CoroContext, cppcoro::task<handle_t<T>>, handle_t<T>>
			{
				T* value = nullptr;
				if constexpr(IsConstructEnabled<T, args_t<T>, Args...>)
				{
					if constexpr(always_send_resource_pack_v<T>)
						value = new T(pack);
					else
						value = new T();
					if constexpr(CoroContext)
						co_await value->construct(pack, std::forward<Args>(args)...);
					else
						cppcoro::sync_wait(value->construct(pack, std::forward<Args>(args)...));
				}
				else
				{
					value = new T(pack, std::forward<Args>(args)...);
				}
				entry.resource = reinterpret_cast<void*>(value);
				entry.deleter  = deleter_fn<T>;
				if constexpr(CoroContext)
					co_return handle_t<T> {entry.info, value};
				else
					return handle_t<T> {entry.info, value};
			}

			template <typename T>
			cppcoro::task<handle_t<T>> coro_copy_fn(handle_t<T> base,
													auto& entry) requires std::is_copy_constructible_v<T>
			{
				co_return copy_fn(base, entry);
			}

			template <typename T>
			handle_t<T> copy_fn(handle_t<T> base, auto& entry) requires std::is_copy_constructible_v<T>
			{
				auto* resource = new T(base.get());
				entry.resource = resource;
				entry.deleter  = deleter_fn<T>;

				return handle_t<T> {entry.info, resource};
			}

			template <HasResourceDataTypes T>
			[[nodiscard]] auto argpack_for(const psl::UID& uid, auto& meta)
			{
				return args_t<T> {*this, meta, *resource_data_for<T>(uid)};
			}

			template <typename T>
			[[nodiscard]] auto argpack_for([[maybe_unused]] const psl::UID& uid, auto& meta)
			{
				return args_t<T> {*this, meta};
			}

			template <HasResourceDataTypes T, bool IsCoroContext = false>
			[[nodiscard]] auto resource_data_for(const psl::UID& uid)
			  -> std::conditional_t<IsCoroContext,
									cppcoro::task<typename resource_type_deduction<T>::type*>,
									typename resource_type_deduction<T>::type*>
			{
				using resource_data_t = typename resource_type_deduction<T>::type;
				resource_data_t* data {nullptr};

				if(auto it = m_ImmutableCache.find(uid); it != std::end(m_ImmutableCache))
				{
					data = reinterpret_cast<resource_data_t*>(it->second.resource);
				}
				else
				{
					if constexpr(IsCoroContext)
					{
						auto& res_entry = m_ImmutableCache[uid];
						data			= co_await coro_load_resource_data<resource_data_t>(uid, m_ImmutableCache[uid]);
					}
					else
					{
						data = load_resource_data<resource_data_t>(uid, m_ImmutableCache[uid]);
					}
				}
				if constexpr(IsCoroContext)
					co_return data;
				else
					return data;
			}

		  public:
			cache_t(std::shared_ptr<psl::meta::library> library,
					std::shared_ptr<cppcoro::static_thread_pool> threadpool = nullptr) :
				m_Library(library),
				m_Workers((threadpool == nullptr) ? std::make_shared<cppcoro::static_thread_pool>(1) : threadpool),
				m_Cache()
			{}

			~cache_t() noexcept(false)
			{
				bool has_leak = std::any_of(std::begin(m_Cache), std::end(m_Cache), [](const auto& pair) {
					return pair.second.info.use_count() > 1;
				});

				if(has_leak) throw std::runtime_error("resource leak detected");
			}

			/// \brief Load a resource from the `library` using a coroutine
			/// \details This method will always load a "pristine" version of the requested item from the disk. It will
			/// cache as much as it can to skip the deserialization step if the same type/uid is requested a second
			/// time.
			/// `find()` will always return the last time the UID was loaded
			///
			/// \tparam T Type to load
			/// \param uid UID of item in the `psl::meta::library`
			/// \return cppcoro::task<handle_t<T>> awaitable task that contains a handle to the requested type
			/// \note If you wish to have access to a shared version of the same type, f.e. multiple materials
			/// sharing the same shader, use `find()` first to see if the asset is already loaded.
			template <typename T, typename... Args>
			[[nodiscard]] cppcoro::task<handle_t<T>> load(const psl::UID& uid, Args&&... args)
			{
				return load_on<T>(*m_Workers, uid, std::forward<Args>(args)...);
			}

			/// \brief Load a resource from the `library` using a coroutine, and schedule it on a specific Scheduler
			/// \details This method will always load a "pristine" version of the requested item from the disk. It will
			/// cache as much as it can to skip the deserialization step if the same type/uid is requested a second
			/// time.
			/// `find()` will always return the last time the UID was loaded
			///
			/// \tparam T Type to load
			/// \tparam Scheduler type that satisfies `IsCoroutineScheduler`
			/// \param scheduler instance of a scheduler
			/// \param uid UID of item in the `psl::meta::library`
			/// \return cppcoro::task<handle_t<T>> awaitable task that contains a handle to the requested type
			/// \note If you wish to have access to a shared version of the same type, f.e. multiple materials
			/// sharing the same shader, use `find()` first to see if the asset is already loaded.
			template <typename T, IsCoroutineScheduler Scheduler, typename... Args>
			[[nodiscard]] cppcoro::task<handle_t<T>> load_on(Scheduler& scheduler, const psl::UID& uid, Args&&... args)
			{
				auto newUID			= psl::UID::generate();
				auto pair			= m_Library->create<typename meta_file_override<T>::type>(newUID);
				m_RenamedCache[uid] = newUID;
				auto& entry			= m_Cache[newUID];
				entry.info->uid		= newUID;
				if constexpr(HasResourceDataTypes<T>)
				{
					using resource_data_t = typename resource_type_deduction<T>::type;
					resource_data_t* data {nullptr};

					if(auto it = m_ImmutableCache.find(uid); it != std::end(m_ImmutableCache))
					{
						data = reinterpret_cast<resource_data_t*>(it->second.resource);
						args_t<T> argpack {*this, pair.second, *data};
						co_return co_await cppcoro::schedule_on(
						  scheduler, load_fn<T, true>(entry, argpack, std::forward<Args>(args)...));
					}
					else
					{
						auto& res_entry = m_ImmutableCache[uid];
						co_return co_await cppcoro::schedule_on(
						  scheduler,
						  load_no_resource_fn<T>(entry, pair.second, uid, res_entry, std::forward<Args>(args)...));
					}
				}
				else
				{
					args_t<T> argpack {*this, pair.second};
					co_return co_await cppcoro::schedule_on(
					  scheduler, load_fn<T, true>(entry, argpack, std::forward<Args>(args)...));
				}
				co_return handle_t<T> {};
			}

			/// \brief Load a resource from the `library` on the calling thread
			/// \details This method will always load a "pristine" version of the requested item from the disk. It will
			/// cache as much as it can to skip the deserialization step if the same type/uid is requested a second
			/// time.
			/// `find()` will always return the last time the UID was loaded
			///
			/// \tparam T Type to load
			/// \param uid UID of item in the `psl::meta::library`
			/// \return handle_t<T> a handle to the requested type
			/// \note If you wish to have access to a shared version of the same type, f.e. multiple materials
			/// sharing the same shader, use `find()` first to see if the asset is already loaded.
			template <typename T, typename... Args>
			[[nodiscard]] handle_t<T> load_now(const psl::UID& uid, Args&&... args)
			{
				auto newUID			= psl::UID::generate();
				auto pair			= m_Library->create<typename meta_file_override<T>::type>(newUID);
				m_RenamedCache[uid] = newUID;
				auto& entry			= m_Cache[newUID];
				entry.info->uid		= newUID;
				args_t<T> argpack(argpack_for<T>(uid, pair.second));
				return load_fn<T, false>(entry, argpack, std::forward<Args>(args)...);
			}

			/// \brief Creates a deep copy of the given handle, and schedule the operation on the specified
			/// Scheduler-type
			///
			/// \tparam T Type contained in the handle
			/// \tparam Scheduler type that satisfies `IsCoroutineScheduler`
			/// \param scheduler instance of a scheduler
			/// \param base Instance of the handle
			/// \return cppcoro::task<handle_t<T>> awaitable task with the results of the copy operation
			template <typename T, IsCoroutineScheduler Scheduler>
			[[nodiscard]] cppcoro::task<handle_t<T>> copy_on(Scheduler& scheduler,
															 handle_t<T> base) requires std::is_copy_constructible_v<T>
			{
				auto it = m_Cache.find(base.uid());
				if(it == std::end(m_Cache))
				{
					throw std::runtime_error(
					  "The target handle was not found in the cache. If this happens the handle is either invalid, or "
					  "was generated from another cache");
				}
				psl::UID newUID = psl::UID::generate();
				auto& entry		= m_Cache[newUID];
				entry.info->uid = newUID;
				co_return co_await cppcoro::schedule_on(scheduler, coro_copy_fn<T>(base, entry));
			}

			/// \brief Creates a deep copy of the given handle
			///
			/// \tparam T Type contained in the handle
			/// \param base Instance of the handle
			/// \return cppcoro::task<handle_t<T>> awaitable task with the results of the copy operation
			template <typename T>
			[[nodiscard]] cppcoro::task<handle_t<T>> copy(handle_t<T> base) requires std::is_copy_constructible_v<T>
			{
				return copy_on<T>(*m_Workers, base);
			}

			/// \brief Creates a deep copy of the given handle on the calling thread
			///
			/// \tparam T Type contained in the handle
			/// \param base Instance of the handle
			/// \return handle_t<T> a handle with the resulting deep copy
			template <typename T>
			[[nodiscard]] handle_t<T> copy_now(handle_t<T> base) requires std::is_copy_constructible_v<T>
			{
				auto it = m_Cache.find(base.uid());
				if(it == std::end(m_Cache))
				{
					throw std::runtime_error(
					  "The target handle was not found in the cache. If this happens the handle is either invalid, or "
					  "was generated from another cache");
				}
				psl::UID newUID = psl::UID::generate();
				auto& entry		= m_Cache[newUID];
				entry.info->uid = newUID;
				return copy_fn<T>(base, entry);
			}

			/// \brief Checks the cache to see if it has a loaded instance of the item
			///
			/// \param uid UID of the resource to find
			/// \return true/false based on if the cache has the resource or not
			[[nodiscard]] bool has(psl::UID uid) const noexcept
			{
				if(auto it = m_RenamedCache.find(uid); it != std::end(m_RenamedCache))
				{
					uid = it->second;
				}
				return m_Cache.contains(uid);
			}

			/// \brief Try to find the resource in the cahce
			///
			/// \tparam T Type of the resource
			/// \param uid UID of the resource to find
			/// \return A valid handle_t<T> if it was found, otherwise it will return an invalid one
			template <typename T>
			[[nodiscard]] handle_t<T> find(psl::UID uid) const noexcept
			{
				if(auto it = m_RenamedCache.find(uid); it != std::end(m_RenamedCache))
				{
					uid = it->second;
				}
				if(auto it = m_Cache.find(uid); it != std::end(m_Cache))
				{
					return handle_t<T> {it->second.info, reinterpret_cast<T*>(it->second.resource)};
				}
				return handle_t<T> {};
			}

			/// \brief Create a resource directly from code in a coroutine
			///
			/// \tparam T Type contained in the handle
			/// \tparam Args Construction argument types
			/// \param args These will be forwarded to the constructor
			/// \return cppcoro::task<handle_t<T>> an awaitable task that contains the results
			template <typename T, typename... Args>
			[[nodiscard]] cppcoro::task<handle_t<T>> create(Args&&... args)
			{
				return create_on<T>(*m_Workers, std::forward<Args>(args)...);
			}

			/// \brief Create a resource directly from code directly in the calling thread
			///
			/// \tparam T Type contained in the handle
			/// \tparam Args Construction argument types
			/// \param args These will be forwarded to the constructor
			/// \return handle_t<T> resulting handle to the constructed resource
			template <typename T, typename... Args>
			[[nodiscard]] handle_t<T> create_now(Args&&... args)
			{
				auto uid		= psl::UID::generate();
				auto pair		= m_Library->create<typename meta_file_override<T>::type>(uid);
				auto& entry		= m_Cache[uid];
				entry.info->uid = uid;
				args_t<T> argpack(argpack_for<T>(uid, pair.second));

				return create_fn<false, T>(entry, argpack, std::forward<Args>(args)...);
			}

			/// \brief Create a resource directly from code in a coroutine, and schedule it on a Schedule-like objecy
			///
			/// \tparam T Type contained in the handle
			/// \tparam Scheduler type that satisfies `IsCoroutineScheduler`
			/// \tparam Args Construction argument types
			/// \param scheduler instance of a scheduler
			/// \param args These will be forwarded to the constructor
			/// \return cppcoro::task<handle_t<T>> an awaitable task that contains the results
			template <typename T, IsCoroutineScheduler Scheduler, typename... Args>
			[[nodiscard]] cppcoro::task<handle_t<T>> create_on(Scheduler& scheduler, Args&&... args)
			{
				auto uid		= psl::UID::generate();
				auto pair		= m_Library->create<typename meta_file_override<T>::type>(uid);
				auto& entry		= m_Cache[uid];
				entry.info->uid = uid;
				args_t<T> argpack(argpack_for<T>(uid, pair.second));

				co_return co_await cppcoro::schedule_on(
				  scheduler, create_fn<true, T>(entry, argpack, std::forward<Args>(args)...));
			}

			/// \brief Tries to clear all memory that is no longer in use
			/// \details The memory is cleared by checking all resources that are no longer referenced outside of the
			/// cache, and by dropping the cached versions of the resources loaded using the load* method group.
			/// \returns The amount of cycles it took to clean up.
			/// \note Directly after running a clear you can query `is_empty()` to see if there are still held resources
			/// out there, if there are it could be used as an indicator of a resource leak.
			/// \note Any `handle` held outside of the cache is a reference the cache will respect. If you wish to
			/// neglect held references then use the `unsafe_cleanup()` method.
			size_t cleanup()
			{
				m_ImmutableCache = {};
				size_t cycles {1};

				// repeat N times as resources can (and should) hold references to other resources they need/depend on
				while(std::erase_if(m_Cache, [](const auto& pair) { return pair.second.info.use_count() <= 1; }) != 0)
				{
					++cycles;
				}
				return cycles;
			}

#if defined(RESOURCE_ALLOW_UNSAFE)
			/// \brief unsafe variation of the cleanup method
			/// \details Avoid this method at all costs, it exists solely for scenario's you can guarantee the current
			/// state of the cache. Due to the nature of this method it has a large chance of segfaulting.
			/// Internally this method will try to cleanup as much as it can safely, until it runs into the scenario
			/// where a resource is still being referenced and cannot be safely deleted. In that moment it will select
			/// "any" of the resources left and erase one, in the hopes of resolving the "clog", and then try to do a
			/// safe cleanup. It should be evident this can lead to a segfault from this description. If any of the
			/// remaining resources still references this and try to access the resource after its deletion, there is no
			/// guarantee for safety remaining.
			/// \return Amount of cycles needed to fully clear the cache.
			/// \warning Avoid this method at all costs, prefer the safer `cleanup()` method unless you know the risks,
			/// and know what you're doing.
			size_t unsafe_clear()
			{
				size_t cycles = 1;
				while(!is_empty())
				{
					cycles += cleanup();

					// if it's still not empty, snipe the first cache entry, see if it resolves the "clog"
					// for obvious reasons this is a "no-no" method, and has a large chance of segfaulting
					if(!is_empty()) m_Cache.erase(std::begin(m_Cache));
				}
				return cycles;
			}
#endif
			template <typename T>
			bool try_destroy(handle_t<T>& handle)
			{
				if(!handle) return false;

				if(auto it = m_Cache.find(handle.uid()); it != std::end(m_Cache))
				{
					if(it->second.info.use_count() > 2) return false;
					handle = {};
					m_Cache.erase(it);
					return true;
				}
				return false;
			}

#if defined(RESOURCE_ALLOW_UNSAFE)
			/// \brief Unsafe variation of `try_destroy`, in contrast this version ignores held references and destroys
			/// the resource regardless. \tparam T \param handle \returns True if the resource was found and destroyed,
			/// otherwise false. \warning Same as `unsafe_clear()` this method has a large chance of creating a segfault
			/// further down your application.
			template <typename T>
			bool unsafe_destroy(handle_t<T>& handle)
			{
				if(!handle) return false;

				if(auto it = m_Cache.find(handle.uid()); it != std::end(m_Cache))
				{
					handle = {};
					m_Cache.erase(it);
					return true;
				}
				return false;
			}
#endif

			/// \brief Check if the cache is empty or not
			/// \returns True if the cache is empty
			bool is_empty() const noexcept { return m_Cache.empty(); }

			/// \brief Size of the cache
			/// \returns the amount of loaded resources
			auto size() const noexcept { return m_Cache.size(); }

		  private:
			std::shared_ptr<psl::meta::library> m_Library;
			std::shared_ptr<cppcoro::static_thread_pool> m_Workers;
			std::unordered_map<psl::UID, cache_entry_t> m_Cache;
			std::unordered_map<psl::UID, psl::UID> m_RenamedCache;

			// special variation of the cache, base types are loaded into this and "preserved"
			// when memory pressure is applied they are the first to go.
			std::unordered_map<psl::UID, immutable_cache_entry_t> m_ImmutableCache;
		};
	}	 // namespace resource
}	 // namespace core::_v1

namespace core::resource
{
	using cache_t = core::_v1::resource::cache_t;

	template <typename T>
	using handle_t = core::_v1::resource::handle_t<T>;

	template <typename T>
	using weak_handle_t = core::_v1::resource::weak_handle_t<T>;

	template <typename T>
	using args_t = core::_v1::resource::args_t<T>;
}	 // namespace core::resource