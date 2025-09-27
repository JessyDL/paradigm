#pragma once
#include "psl/array.hpp"
#include "psl/ecs/details/filter_handler.hpp"
#include "psl/ecs/details/system_information.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/ecs/filtering.hpp"
#include "psl/memory/region.hpp"
#include <memory>
#include <shared_mutex>
#include <tbb/tbb.h>
#include <unordered_map>
#include <unordered_set>

namespace psl::ecs {
class state_t;
}
namespace psl::ecs::details {
class task_performance_observer_t : tbb::task_scheduler_observer {
  public:
	task_performance_observer_t() : tbb::task_scheduler_observer() {
		observe(true);
	}
	~task_performance_observer_t() {
		observe(false);
	}
	void on_scheduler_entry(bool) override {
		m_Start = std::chrono::high_resolution_clock::now();
	}
	void on_scheduler_exit(bool) override {
		m_End = std::chrono::high_resolution_clock::now();
	}
	auto duration() const noexcept -> std::chrono::duration<float> {
		return m_End - m_Start;
	}

  private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Start {};
	std::chrono::time_point<std::chrono::high_resolution_clock> m_End {};
};

class explicit_thread_executor_t {
  public:
	explicit_thread_executor_t() noexcept : m_Tasks({}), m_ThreadId(std::this_thread::get_id()) {}
	template <typename Func>
	void schedule(Func&& func) {
		m_Tasks.push(std::forward<Func>(func));
	}

	void rebind() noexcept {
		m_ThreadId = std::this_thread::get_id();
	}

	bool is_valid_thread() const noexcept {
		return std::this_thread::get_id() == m_ThreadId;
	}

	bool has_work() const noexcept {
		return !m_Tasks.empty();
	}

	void process(std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) const {
		if(!is_valid_thread()) {
			throw std::runtime_error("Main thread executor can only be processed from the main thread");
		}

		auto const start = std::chrono::high_resolution_clock::now();
		while(!m_Tasks.empty()) {
			std::function<bool()> task;
			if(m_Tasks.try_pop(task)) {
				auto res = std::invoke(task);
				if(!res) {
					m_Tasks.push(std::move(task));
				}
			}
			if(timeout != std::chrono::milliseconds::max() &&
			   std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -
																	 start) >= timeout) {
				break;
			}
		}
	}

	void operator()(std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) const {
		process(timeout);
	}

  private:
	mutable tbb::concurrent_queue<std::function<bool()>> m_Tasks;
	std::thread::id m_ThreadId {};
};
struct component_lock_t {
	component_lock_t() : lock(std::make_unique<tbb::rw_mutex>()) {}
	component_lock_t(const component_lock_t&) = delete;
	component_lock_t(component_lock_t&& other) noexcept
		: lock(std::move(other.lock)), readers(other.readers.load()), writers(other.writers.load()),
		  writer_owner(other.writer_owner.load()) {}
	component_lock_t& operator=(const component_lock_t&) = delete;
	component_lock_t& operator=(component_lock_t&& other) noexcept {
		if(this != &other) {
			lock		 = std::move(other.lock);
			readers		 = other.readers.load();
			writers		 = other.writers.load();
			writer_owner = other.writer_owner.load();
		}
		return *this;
	}
	std::unique_ptr<tbb::rw_mutex> lock {};
	std::atomic<size_t> readers {0};
	std::atomic<size_t> writers {0};	// can be multiple writers if they belong to the same system as they will
										// not conflict on
										// data access patterns
	std::atomic<system_token>
	  writer_owner {};	  // in case there's an owner, anyone of the same system can share the mutex
};

struct component_lock_instance_t {
  public:
	component_lock_instance_t(component_lock_t* lock, system_token owner, bool isWrite, bool isDirect) noexcept
		: m_Lock(lock), m_Owner(owner), m_IsWrite(isWrite), m_IsDirect(isDirect) {}
	bool try_lock() noexcept {
		if(m_IsWrite) {
			if(m_Lock->writer_owner.load() == m_Owner) {
				// already owned by the same system, we can share the lock
				m_Lock->writers++;
				return true;
			}
			if(m_Lock->lock->try_lock()) {
				// we acquired the lock, set ourselves as the owner
				m_Lock->writer_owner = m_Owner;
				m_Lock->writers++;
				return true;
			}
			return false;
		} else {
			if(m_Lock->lock->try_lock_shared()) {
				m_Lock->readers++;
				return true;
			}
			return false;
		}
	}

	void lock() noexcept {
		if(m_IsWrite) {
			if(m_Lock->writer_owner.load() == m_Owner) {
				// already owned by the same system, we can share the lock
				m_Lock->writers++;
				return;
			}
			m_Lock->lock->lock();
			// we acquired the lock, set ourselves as the owner
			m_Lock->writer_owner = m_Owner;
			m_Lock->writers++;
		} else {
			m_Lock->lock->lock_shared();
			m_Lock->readers++;
		}
	}

	bool unlock() {
		if(m_IsWrite) {
			if(m_Lock->writers == 0 || m_Lock->writer_owner.load() != m_Owner) {
				throw std::runtime_error("Unlocking a component lock that is not owned by the current system");
			}
			m_Lock->writers--;
			if(m_Lock->writers == 0) {
				// last writer, release the lock
				m_Lock->writer_owner = system_token {};
				m_Lock->lock->unlock();
			}
			return true;
		} else {
			if(m_Lock->readers == 0) {
				throw std::runtime_error("Unlocking a component lock that is not owned by any reader");
			}
			m_Lock->readers--;
			m_Lock->lock->unlock_shared();
			return true;
		}
	}

	bool is_locked() const noexcept {
		if(m_IsWrite) {
			return m_Lock->writers > 0 && m_Lock->writer_owner.load() == m_Owner;
		} else {
			return m_Lock->readers > 0;
		}
	}

	bool is_write() const noexcept {
		return m_IsWrite;
	}
	bool is_direct() const noexcept {
		return m_IsDirect;
	}

  private:
	component_lock_t* m_Lock {nullptr};
	system_token m_Owner {};
	bool m_IsWrite {false};
	bool m_IsDirect {false};
};

class component_lock_instance_group_t {
  public:
	component_lock_instance_group_t(std::vector<component_lock_instance_t>& locks, tbb::mutex& mutex)
		: m_Locks(locks), m_Mutex(&mutex) {}
	component_lock_instance_group_t(component_lock_instance_group_t&& other) noexcept
		: m_Locks(std::move(other.m_Locks)), m_Mutex(std::move(other.m_Mutex)) {}
	component_lock_instance_group_t& operator=(component_lock_instance_group_t&& other) noexcept {
		if(this != &other) {
			m_Locks = std::move(other.m_Locks);
			m_Mutex = std::move(other.m_Mutex);
		}
		return *this;
	}

	bool try_lock() {
		auto scoped_lock = std::scoped_lock(*m_Mutex);
		for(auto it = m_Locks.begin(); it != m_Locks.end(); ++it) {
			if(!it->try_lock()) {
				// unlock all previous locks
				for(auto rev = m_Locks.begin(); rev != it; ++rev) {
					rev->unlock();
				}
				return false;
			}
		}
		return true;
	}

	void lock() {
		auto scoped_lock = std::scoped_lock(*m_Mutex);
		for(auto& lock : m_Locks) {
			lock.lock();
		}
	}

	void unlock() {
		auto scoped_lock = std::scoped_lock(*m_Mutex);
		for(auto& lock : m_Locks) {
			lock.unlock();
		}
	}

  private:
	std::vector<component_lock_instance_t> m_Locks {};
	tbb::mutex* m_Mutex;
};

struct filter_task_result_t {
	filter_result const* container {};
	size_t workers {1};
	filter_id_t id {};
};

struct system_task_container_t {
	system_task_container_t(system_token id, system_information* system, size_t pending_filters)
		: id(id), system(system), pending_filters(pending_filters), original_pending(pending_filters) {}
	system_task_container_t(const system_task_container_t& other)
		: id(other.id), system(other.system), filters(other.filters), pending_filters(other.pending_filters.load()),
		  original_pending(other.original_pending) {}
	system_task_container_t(system_task_container_t&& other) noexcept
		: id(other.id), system(other.system), filters(std::move(other.filters)),
		  pending_filters(other.pending_filters.load()), original_pending(other.original_pending) {}
	system_task_container_t& operator=(const system_task_container_t& other) {
		if(this != &other) {
			id				 = other.id;
			system			 = other.system;
			filters			 = other.filters;
			pending_filters	 = other.pending_filters.load();
			original_pending = other.original_pending;
		}
		return *this;
	}
	system_task_container_t& operator=(system_task_container_t&& other) noexcept {
		if(this != &other) {
			id				 = other.id;
			system			 = other.system;
			filters			 = std::move(other.filters);
			pending_filters	 = other.pending_filters.load();
			original_pending = other.original_pending;
		}
		return *this;
	}

	void reset() {
		pending_filters.store(original_pending);
	}

	system_token id;
	system_information* system;
	std::vector<filter_task_result_t const*> filters;
	std::atomic<size_t> pending_filters;

	size_t original_pending;
};

struct filter_task_t {
	filter_handler_t const* handler {};
	psl::array_view<psl::ecs::entity_t> changeset {};
	filter_result* filter {};
	std::vector<system_task_container_t*> systems {};
	filter_task_result_t& result;
};

class system_invocable_task_t {
  public:
	system_invocable_task_t() noexcept = default;
	system_invocable_task_t(system_token id,
							system_information* system,
							std::vector<dependency_pack> const& packs,
							psl::ecs::info_t* info,
							components_cache_t* component_cache) noexcept
		: m_Id(id), m_System(system), m_Packs(packs), m_Info(info), m_ComponentCache(component_cache) {}
	auto operator()(std::vector<std::optional<memory::segment>> const& segments) const -> void {
		prepare(segments);
		execute();
		finalize();
	}

	std::vector<size_t> get_cache_requirements() const;

	system_token id() const noexcept {
		return m_Id;
	}

  private:
	/// \brief Loads all the required data into the cache, and binds the m_Packs.
	void prepare(std::vector<std::optional<memory::segment>> const& segments) const;
	/// \brief Executes the system with the provided info_t and the packs it has prepared.
	void execute() const;
	/// \brief Copy the results back into the state's component caches, and returns the command_buffer_t containing the instructions for the state to process.
	void finalize() const;

	system_token m_Id;
	system_information* m_System;
	std::vector<dependency_pack> m_Packs;
	psl::ecs::info_t* m_Info;
	components_cache_t* m_ComponentCache;
};

class cache_resource_t {
  public:
	cache_resource_t(size_t size, size_t alignment) : m_Region(size, alignment) {}

	std::vector<std::optional<memory::segment>> allocate(std::vector<size_t> sizes) {
		std::lock_guard lock(m_Mutex);
		std::vector<std::optional<memory::segment>> segments;
		segments.reserve(sizes.size());
		for(auto size : sizes) {
			if(size == 0) {
				segments.push_back(std::nullopt);
				continue;
			}
			segments.push_back(m_Region.allocate(size));
			if(!segments.back()) {
				deallocate_internal(segments);
				return {};
			}
		}
		return segments;
	}

	void deallocate(std::vector<std::optional<memory::segment>> const& segments) {
		std::lock_guard lock(m_Mutex);
		deallocate_internal(segments);
	}

  private:
	void deallocate_internal(std::vector<std::optional<memory::segment>> const& segments) {
		for(auto& segment : segments) {
			if(segment) {
				m_Region.deallocate(*const_cast<memory::segment*>(&*segment));
			}
		}
	}
	std::mutex m_Mutex;
	memory::region m_Region;
};

class system_scheduler_t;

class system_invocable_task_group_t {
  public:
	system_invocable_task_group_t(system_token id,
								  system_information* system,
								  component_lock_instance_group_t locks,
								  cache_resource_t* cache) noexcept
		: m_Id(id), m_System(system), m_Tasks(), m_Locks(std::move(locks)), m_Cache(cache) {
		m_PendingTasks.store(m_Tasks.size());
	}

	void set_dependencies(std::vector<system_invocable_task_group_t*> dependencies) {
		std::lock_guard lock(m_Mutex);
		for(auto& dependency : dependencies) {
			dependency->m_DependentTasks.emplace_back(this);
		}
		m_RemainingDependencies.store(dependencies.size());
	}

	void remove_dependency(system_invocable_task_group_t* dependency) {
		std::lock_guard lock(m_Mutex);
		dependency->m_DependentTasks.erase(
		  std::remove(dependency->m_DependentTasks.begin(), dependency->m_DependentTasks.end(), this),
		  dependency->m_DependentTasks.end());
		--m_RemainingDependencies;
	}

	void set_tasks(std::vector<system_invocable_task_t> tasks) {
		std::lock_guard lock(m_Mutex);
		m_Tasks		= std::move(tasks);
		m_Current	= m_Tasks.begin();
		m_Startable = true;
	}

	system_token id() const noexcept {
		return m_Id;
	}

	bool is_ready() const noexcept {
		return m_Startable && m_RemainingDependencies.load() == 0;
	}

	bool is_finished() const noexcept {
		return m_Finished.load();
	}

	void reset() noexcept {
		m_Tasks.clear();
		m_Current = m_Tasks.begin();
		m_PendingTasks.store(m_Tasks.size());
		m_Finished.store(false);
		m_Startable.store(false);
		m_RemainingDependencies.store(0);
		m_DependentTasks.clear();
	}

	/// \brief Either schedules tasks to be executed on the provided task_group, or executes them on the calling thread.
	/// \note This function is thread-safe and can be called from any thread. Additionally the
	/// calling thread _might_ be used to execute the tasks if possible.
	/// \return True if all tasks have been executed, false otherwise.
	bool operator()(system_scheduler_t* handler,
					std::optional<std::reference_wrapper<tbb::task_group>> group = std::nullopt);

  private:
	/// \brief Notifies the task group that a dependency has been completed.
	/// If this was the last dependency, it will return true marking it as 'is_ready' to be scheduled.
	void dependency_completed() noexcept {
		--m_RemainingDependencies;
	}

	system_token m_Id;
	system_information* m_System;
	std::vector<system_invocable_task_t> m_Tasks;
	component_lock_instance_group_t m_Locks;
	cache_resource_t* m_Cache {nullptr};
	std::vector<system_invocable_task_t>::iterator m_Current {m_Tasks.begin()};
	std::atomic<size_t> m_PendingTasks {0};
	tbb::mutex m_Mutex;
	std::atomic<bool> m_Finished {false};
	std::atomic<bool> m_Startable {false};
	std::atomic<size_t> m_RemainingDependencies {0};
	std::vector<system_invocable_task_group_t*> m_DependentTasks {};
};

/// \brief Manages the scheduling of systems which are ready to be executed.
/// This scheduler is responsible for ensuring that systems are executed.
/// Internally it will make sure that the system has enough cache to be scheduled,
/// and that the components it has need for are not being used by other systems.
/// If that's the case it will immediately run the system on the calling thread.
/// Otherwise it will store the system as a pending task, and once resources are
/// freed up the scheduler will check the pending tasks and try to execute them.
class system_scheduler_t {
	friend class system_handler_t;

  public:
	system_scheduler_t(tbb::task_group& group, size_t cache_size, size_t cache_alignment) noexcept
		: m_ExecutionGroup(group) {}
	/// \brief Schedules a system for execution, if it cannot be executed immediately it will be stored as a pending task.
	/// \note This function is thread-safe and can be called from any thread. Additionally the calling thread _might_ be used to execute the system if possible.
	void execute(system_token id, system_information* system, std::vector<filter_task_result_t*> filters = {});

	void
	prepare(psl::ecs::state_t& state,
			std::chrono::duration<float> dTime,
			std::chrono::duration<float> rTime,
			size_t tick,
			std::unordered_map<system_token, std::unique_ptr<system_invocable_task_group_t>>* running_systems) noexcept;

	bool is_done() const noexcept {
		return m_PendingTaskGroups.empty() && !m_MainThreadExecutor.has_work();
	}
	void schedule(system_information* system, system_invocable_task_group_t& group);

  private:
	std::vector<system_invocable_task_t>
	make_tasks(system_token id, system_information* system, std::vector<filter_task_result_t*> filters);

	void reschedule(system_invocable_task_group_t& group);

	void try_execute_pending();
	tbb::concurrent_queue<system_invocable_task_group_t*> m_PendingTaskGroups {};
	tbb::concurrent_vector<std::unique_ptr<psl::ecs::info_t>> m_CommandBuffers {};
	explicit_thread_executor_t m_MainThreadExecutor {};
	tbb::task_group& m_ExecutionGroup;
	std::unique_ptr<psl::ecs::info_t>
	  m_SharedCommandBuffer {};	   // used for const systems, or those who do not read the info_t
	components_cache_t* m_ComponentCache {nullptr};
	std::unordered_map<system_token, std::unique_ptr<system_invocable_task_group_t>>* m_RunningSystems {nullptr};
	std::atomic<size_t> m_RemainingTasks {};
};

class system_handler_t {
	struct filter_shared_state_t {
		filter_handler_t* handler;
		psl::array_view<psl::ecs::entity_t> modified;
		psl::array_view<psl::ecs::entity_t> mutated;
	};

	class internal_graph_t {
	  public:
		struct system_info_t {
			system_information* system;
			std::vector<filter_id_t> filters;
			std::vector<filter_task_result_t*> filters_task_results;
			std::vector<system_token> dependencies;
			system_task_container_t* task {};
		};

		struct filter_info_t {
			std::vector<std::pair<system_token, system_info_t*>> systems;
			filter_task_result_t* result;
		};

		bool has(system_token id) const noexcept {
			return m_ExistingSystems.find(id) != m_ExistingSystems.end();
		}

		void prepare(filter_shared_state_t const& state, cache_resource_t& cache);
		void apply_changes(psl::array<system_information*>& systems, psl::array<filter_result>& all_filters);
		void add(system_information* system, psl::array<filter_result>& all_filters);
		void remove(system_token system);
		// void rebuild(psl::array<system_information*> systems, psl::array<filter_result>& all_filters);
		void schedule(tbb::task_group& group, system_scheduler_t& system_scheduler);

		std::unordered_map<system_token, std::unique_ptr<system_invocable_task_group_t>>& running_systems() {
			return m_RunningSystems;
		}

	  private:
		// happens when a system is removed from the state which was the last owner of a filter
		void remove(filter_id_t filter);
		std::unordered_set<system_token> m_ExistingSystems;
		std::unordered_map<system_token, system_info_t> m_SystemMap {};
		std::unordered_map<filter_id_t, filter_info_t> m_FilterMap {};
		std::unordered_map<component_key_t, component_lock_t> m_ComponentLocks {};
		std::unordered_map<system_token, std::unique_ptr<system_invocable_task_group_t>> m_RunningSystems {};
		std::vector<std::unique_ptr<system_task_container_t>> m_SystemContainers {};
		std::vector<std::unique_ptr<filter_task_result_t>> m_FilterResults {};
		filter_shared_state_t m_FilterState;

		// normally we schedule all the filters, which will kick off the systems that depend on them
		// but there are system which have no filters, these need special handling.
		std::unordered_set<system_token> m_FilterlessSystems {};
		tbb::mutex m_ComponentLockMutex {};
		cache_resource_t* m_Cache {nullptr};
	};

  public:
	struct system_task_t {
		system_token id;
		std::unordered_set<filter_id_t> filters;
		system_information* system;
	};

	struct state_info_t {
		psl::array<psl::ecs::entity_t> modified_entities;
		psl::array<psl::ecs::entity_t> mutated_entities;
		psl::ecs::state_t* state;
		filter_handler_t* filter_handler;
	};

	system_handler_t(size_t cache_size = 64 * 1024 * 1024);
	~system_handler_t();
	system_handler_t(const system_handler_t&)			 = delete;
	system_handler_t(system_handler_t&&)				 = delete;
	system_handler_t& operator=(const system_handler_t&) = delete;
	system_handler_t& operator=(system_handler_t&&)		 = delete;

	auto execute(state_info_t info,
				 psl::array<system_information*> systems,
				 std::chrono::duration<float> dTime,
				 std::chrono::duration<float> rTime,
				 size_t tick) -> psl::array<std::unique_ptr<psl::ecs::info_t>>;

  private:
	system_scheduler_t m_SystemScheduler;
	// std::vector<system_task_container_t> m_SystemContainers {};
	// std::vector<filter_task_result_t> m_FilterResults {};
	// std::unordered_map<component_key_t, component_lock_t> m_ComponentLocks {};
	// std::unordered_map<system_token, std::unique_ptr<system_invocable_task_group_t>> m_RunningSystems {};
	tbb::task_arena m_Arena;
	tbb::task_group m_FilteringTasks;
	// filter_shared_state_t m_FilterState;
	cache_resource_t m_Cache;
	// tbb::mutex m_ComponentLockMutex {};

	internal_graph_t m_Graph {};
};
}	 // namespace psl::ecs::details
