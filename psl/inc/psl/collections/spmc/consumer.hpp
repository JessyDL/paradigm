#pragma once
#include "psl/collections/ring_array.hpp"
#include "psl/math/math.hpp"
#include "psl/view_ptr.hpp"
#include <atomic>
#include <optional>

namespace psl::spmc {
template <typename T>
class producer;

/// \brief provides multithread safe access into a psl::spmc::producer
///
/// \details This is the object to pass to the consumer threads, they have a minimal multithread safe API
/// into the SPMC deque. The consumer is limited to viewing the state of the producer, and popping items from the
/// front (if any are left).
template <typename T>
class consumer final {
	friend class producer<T>;
	consumer(psl::view_ptr<producer<T>> producer) : m_Producer(producer) {};

  public:
	consumer()	= delete;
	~consumer() = default;

	consumer(const consumer& other)				   = default;
	consumer(consumer&& other) noexcept			   = default;
	consumer& operator=(const consumer& other)	   = default;
	consumer& operator=(consumer&& other) noexcept = default;

	/// \brief Tries to pop an element from the front of the producer thread's deque.
	///
	/// \details If any items are left on the producer's deque, then this method will pop an item off of the front.
	/// Otherwise it will return a nullopt;
	auto pop() noexcept -> decltype(std::declval<producer<T>>().steal()) { return m_Producer->steal(); }


	/// \returns the current count of all elements in the producer.
	/// \warning The result here doesn't mean there will be/won't be an item on the deque by the time you invoke
	/// pop.
	auto size() const noexcept -> decltype(std::declval<producer<T>>().size()) { return m_Producer->size(); }

	/// \returns the current count of all elements in the producer.
	/// \warning The result here doesn't mean there will be/won't be an item on the deque by the time you invoke
	/// pop.
	auto ssize() const noexcept -> decltype(std::declval<producer<T>>().ssize()) { return m_Producer->ssize(); }

  private:
	psl::view_ptr<producer<T>> m_Producer;
};

}	 // namespace psl::spmc
