#pragma once
#include "psl/collections/ring_array.hpp"
#include "psl/math/math.hpp"
#include "psl/utility/cast.hpp"
#include "psl/view_ptr.hpp"
#include <atomic>
#include <optional>

namespace psl::spmc {
template <typename T>
class consumer;

/// \brief SPMC based on Chase-Lev deque
///
/// \details This class is based on Chase-Lev's deque implementation of a Single Producer Multi Consumer (SPMC).
/// The producer should only be used on a single thread, and the 'psl::spmc::consumer' variant should be used in
/// the consuming threads. This offers a safe API.
template <typename T>
class producer final {
	friend class consumer<T>;

	using signed_size_t		   = std::conditional_t<sizeof(size_t) == 8, int64_t, int32_t>;
	using signed_atomic_size_t = std::conditional_t<sizeof(size_t) == 8, std::atomic_int64_t, std::atomic_int32_t>;

	/// \brief Wrapper over ring_array<T>
	///
	/// \details This wrapper class over a ring_array keeps track of its internal offset.
	/// That will be used by the producer as it schedules more and more items.
	/// The ring_array can only be resized by invoking the buffer::copy method.
	/// Internally you can keep incrementing your access indices, as long as you never exceed the range
	/// being used (range <= buffer::capacity(), where range == begin to end indices).
	/// When you exceed the capacity, you need to buffer::copy a new one.
	struct buffer {
	  public:
		buffer(size_t capacity) : m_Data(psl::math::next_pow_of(2, std::max<size_t>(32u, capacity))) {};
		~buffer() {}
		void set(signed_size_t index, T&& value) noexcept {
			m_Data[psl::narrow_cast<size_t>((index - static_cast<signed_size_t>(m_Offset)) & (m_Data.ssize() - 1))] =
			  std::forward<T>(value);
		}

		auto at(signed_size_t index) const noexcept {
			return m_Data[psl::narrow_cast<size_t>((index - static_cast<signed_size_t>(m_Offset)) &
												   (m_Data.ssize() - 1))];
		}

		/// \brief Returns a logical continuation buffer based on this buffer
		///
		/// \details Copies the current buffer into a new one of 'at least' the given capacity. It will grow to the
		/// next logical power of 2 that can also contain the begin-end items.
		buffer* copy(size_t begin, size_t end, size_t capacity = 0) {
			capacity = std::max<size_t>(1024, psl::math::next_pow_of(2, std::max(capacity, (end - begin) + 1)));

			buffer* ptr	  = new buffer(capacity);
			ptr->m_Offset = begin;
			for(size_t i = begin; i != end; ++i) {
				ptr->set(i, at(i));
			}
			return ptr;
		}

		/// \returns Max continuous range of items in the buffer.
		size_t capacity() const noexcept { return m_Data.capacity(); };

	  private:
		ring_array<T> m_Data;
		size_t m_Offset {0};
	};

  public:
	producer(signed_size_t capacity = 1024) {
		capacity = psl::math::next_pow_of(2, std::max(capacity, (signed_size_t)1024));
		m_Begin.store(0, std::memory_order_relaxed);
		m_End.store(0, std::memory_order_relaxed);
		auto cont = new buffer(capacity);
		m_Data.store(cont, std::memory_order_relaxed);
	};
	~producer() {
		if(m_Last != nullptr)
			delete(m_Last);
		delete(m_Data.load());
	}

	producer(const producer& other)			   = delete;
	producer(producer&& other)				   = default;
	producer& operator=(const producer& other) = delete;
	producer& operator=(producer&& other)	   = default;

	/// \returns a consumer that is linked to the current producer, to be used in other threads.
	::psl::spmc::consumer<T> consumer() noexcept;

	bool empty() const noexcept {
		auto begin = m_Begin.load(std::memory_order_relaxed);
		auto end   = m_End.load(std::memory_order_relaxed);
		return end <= begin;
	}

	/// \returns the current count of all elements in the producer.
	size_t size() const noexcept { return static_cast<size_t>(ssize()); }

	/// \returns the current count of all elements in the producer.
	signed_size_t ssize() const noexcept {
		auto begin = m_Begin.load(std::memory_order_relaxed);
		auto end   = m_End.load(std::memory_order_relaxed);
		return std::max<decltype(begin)>(end - begin, 0);
	}

	/// \brief Resizes the internal buffer to the given size
	///
	/// \details Tries to resize to the given size, it will automatically align itself to the next power of 2 if
	/// the value isn't a power of 2 already. The minimum size will be 'at least' equal to, or bigger than, the
	/// current size (not capacity) of the internal buffer.
	void resize(size_t size) {
		auto cont = m_Data.load(std::memory_order_relaxed);
		size	  = psl::math::next_pow_of(2, size);
		if(size == (signed_size_t)cont->capacity())
			return;

		auto begin	 = m_Begin.load(std::memory_order_relaxed);
		auto end	 = m_End.load(std::memory_order_relaxed);
		auto newCont = cont->copy(begin, end);
		std::swap(newCont, cont);
		m_Data.store(cont, std::memory_order_relaxed);

		if(m_Last != nullptr)
			delete(m_Last);
		m_Last = newCont;
	}

	/// \brief Push the given element onto the end of the deque.
	///
	/// \details Will push the given element onto the deque if enough space is present.
	/// If not enough space is present it will construct a new internal buffer that contains at least enough space.
	/// If a previous (unused) buffer is present it will now clean that buffer up.
	/// \warning Callable only on the owning thread, do not call from multiple threads.
	/// \todo Implement the backing storage as an atomic<shared_ptr<buffer>> for more logical cleanup flow.
	void push(T&& value) {
		signed_size_t end	= m_End.load(std::memory_order_relaxed);
		signed_size_t begin = m_Begin.load(std::memory_order_acquire);
		auto cont			= m_Data.load(std::memory_order_relaxed);

		if(static_cast<signed_size_t>(cont->capacity()) < (end - begin) + 1) {
			auto newCont = cont->copy(begin, end);
			std::swap(newCont, cont);
			m_Data.store(cont, std::memory_order_relaxed);

			if(m_Last != nullptr)
				delete(m_Last);
			m_Last = newCont;
		}

		cont->set(end, std::forward<T>(value));
		std::atomic_thread_fence(std::memory_order_release);
		m_End.store(end + 1, std::memory_order_relaxed);
	}

	/// \brief Pops an element (if any are left) off the deque from the end.
	///
	/// \details Tries to pop an element off the end of the deque.
	/// \warning Only callable from the owning thread, otherwise the results will be undefined.
	std::optional<T> pop() noexcept {
		signed_size_t end = m_End.load(std::memory_order_relaxed) - 1;
		auto cont		  = m_Data.load(std::memory_order_relaxed);
		m_End.store(end, std::memory_order_relaxed);
		std::atomic_thread_fence(std::memory_order_seq_cst);
		signed_size_t begin = m_Begin.load(std::memory_order_relaxed);

		std::optional<T> res {std::nullopt};

		if(begin <= end) {
			res = cont->at(end);
			if(begin == end) {
				if(!m_Begin.compare_exchange_strong(
					 begin, begin + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
					res = std::nullopt;
				}
				m_End.store(end + 1, std::memory_order_relaxed);
			}
		} else {
			m_End.store(end + 1, std::memory_order_relaxed);
		}
		return res;
	}

	void clear() noexcept {
		auto end = m_End.load(std::memory_order_relaxed);
		m_Begin.store(end, std::memory_order_seq_cst);
	}

  private:
	/// \brief Pops an element (if any are left) off the deque from the front.
	///
	/// \details To be used by consumer threads, this gives a thread safe way of stealing items from the deque.
	/// It can be called by any thread, but is only exposed to the consumer class.
	std::optional<T> steal() noexcept {
		signed_size_t begin = m_Begin.load(std::memory_order_acquire);
		std::atomic_thread_fence(std::memory_order_seq_cst);
		signed_size_t end = m_End.load(std::memory_order_acquire);

		std::optional<T> res {std::nullopt};

		if(begin < end) {
			auto data = m_Data.load(std::memory_order_consume);
			res		  = data->at(begin);

			if(!m_Begin.compare_exchange_strong(
				 begin, begin + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
				return std::nullopt;
			}
		}

		return res;
	}

	signed_atomic_size_t m_Begin;
	signed_atomic_size_t m_End;
	std::atomic<buffer*> m_Data;
	buffer* m_Last {nullptr};
};

template <typename T>
::psl::spmc::consumer<T> producer<T>::consumer() noexcept {
	return ::psl::spmc::consumer<T>(psl::view_ptr<producer<T>> {this});
};
}	 // namespace psl::spmc
