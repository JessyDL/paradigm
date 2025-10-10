#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/assertions.hpp"
#include "psl/ecs/component_traits.hpp"
#include "psl/ecs/details/stage_range.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/platform_def.hpp"
#include "psl/sparse_array.hpp"
#include "psl/thread_safety_guard.hpp"
#include "psl/utility/cast.hpp"
#include <cstring>	  // std::memmove
#include <memory>	  // std::uninitialized_move
#include <optional>
#include <span>
#include <type_traits>
#include <utility>	  // std::forward, std::move

namespace psl::ecs::details {
struct untyped_tag_t {};
struct flag_tag_t {};

struct untyped_iterator_t {
	using iterator_concept	= std::contiguous_iterator_tag;
	using iterator_category = std::random_access_iterator_tag;
	using value_type		= std::byte;	// your element type
	using difference_type	= std::ptrdiff_t;
	using pointer			= std::byte*;	 // can be void for output iterators
	using reference			= std::byte&;	 // can be void for output iterators
	using element_type		= std::byte;

	untyped_iterator_t(std::byte* ptr, size_t type_size) : m_Ptr(ptr), m_TypeSize(type_size) {
		psl_assert(m_TypeSize > 0, "type size must be greater than 0");
	}
	untyped_iterator_t() = default;
	untyped_iterator_t(untyped_iterator_t&& other) noexcept : m_Ptr(other.m_Ptr), m_TypeSize(other.m_TypeSize) {
		other.m_Ptr = nullptr;
	}
	untyped_iterator_t(const untyped_iterator_t& other)			   = default;
	untyped_iterator_t& operator=(const untyped_iterator_t& other) = default;
	untyped_iterator_t& operator=(untyped_iterator_t&& other)	   = default;
	~untyped_iterator_t()										   = default;

	reference operator*() const {
		psl_assert(m_Ptr != nullptr, "dereferencing a null pointer");
		return *m_Ptr;
	}
	pointer operator->() const {
		psl_assert(m_Ptr != nullptr, "accessing a null pointer");
		return m_Ptr;
	}

	// For accessing the whole "element"
	pointer data() const {
		return m_Ptr;
	}

	bool operator==(const untyped_iterator_t& other) const noexcept {
		return m_Ptr == other.m_Ptr;
	}
	bool operator!=(const untyped_iterator_t& other) const noexcept {
		return m_Ptr != other.m_Ptr;
	}
	untyped_iterator_t& operator++() noexcept {
		m_Ptr += m_TypeSize;
		return *this;
	}
	untyped_iterator_t operator++(int) noexcept {
		auto temp = *this;
		++(*this);
		return temp;
	}
	untyped_iterator_t& operator--() noexcept {
		m_Ptr -= m_TypeSize;
		return *this;
	}
	untyped_iterator_t operator--(int) noexcept {
		auto temp = *this;
		--(*this);
		return temp;
	}
	untyped_iterator_t operator+(size_t offset) const noexcept {
		return untyped_iterator_t(m_Ptr + (offset * m_TypeSize), m_TypeSize);
	}
	untyped_iterator_t operator-(size_t offset) const noexcept {
		return untyped_iterator_t(m_Ptr - (offset * m_TypeSize), m_TypeSize);
	}
	untyped_iterator_t& operator+=(size_t offset) noexcept {
		m_Ptr += (offset * m_TypeSize);
		return *this;
	}
	untyped_iterator_t& operator-=(size_t offset) noexcept {
		m_Ptr -= (offset * m_TypeSize);
		return *this;
	}
	bool operator<(const untyped_iterator_t& other) const noexcept {
		return m_Ptr < other.m_Ptr;
	}
	bool operator>(const untyped_iterator_t& other) const noexcept {
		return m_Ptr > other.m_Ptr;
	}
	bool operator<=(const untyped_iterator_t& other) const noexcept {
		return m_Ptr <= other.m_Ptr;
	}
	bool operator>=(const untyped_iterator_t& other) const noexcept {
		return m_Ptr >= other.m_Ptr;
	}

	difference_type operator-(const untyped_iterator_t& other) const {
		return (m_Ptr - other.m_Ptr) / m_TypeSize;
	}


	std::byte* m_Ptr {nullptr};
	size_t m_TypeSize {0};
};
namespace impl {
	template <typename T, typename IndexType, typename Pointer, typename DataIterator>
	concept IsForEachFoundInvocable = !std::input_or_output_iterator<T> &&
									  (std::is_invocable_v<T, IndexType, Pointer, DataIterator> ||
									   std::is_invocable_v<T, IndexType, Pointer> || std::is_invocable_v<T, IndexType>);

	template <typename T, typename IndexType>
	concept IsForEachNotFoundInvocable = !std::input_or_output_iterator<T> && std::is_invocable_v<T, IndexType>;

	template <typename T, typename Key>
	struct dense_storage_base_t {
		dense_storage_base_t(Key initial_size) : m_Dense(initial_size * sizeof(T)) {
			recalculate_storage_size();
		}
		~dense_storage_base_t() {
			if constexpr(!std::is_trivially_destructible_v<T>) {
				for(auto it = m_Begin; it < m_End; ++it) {
					it->~T();
				}
			}
		}

		Key capacity() const noexcept {
			return psl::narrow_cast<Key>(std::distance(m_Begin, m_StorageEnd));
		}

		Key size() const noexcept {
			return psl::narrow_cast<Key>(m_End - m_Begin);
		}

		T& operator[](Key index) {
			psl_assert(index < size(), "index out of bounds");
			return m_Begin[index];
		}
		const T& operator[](Key index) const {
			psl_assert(index < size(), "index out of bounds");
			return m_Begin[index];
		}

		template <typename U>
		void set(Key index, U&& value) {
			psl_assert(index < size(), "index out of bounds");

			if constexpr(std::is_same_v<T, std::remove_cvref_t<U>>) {
				if constexpr(std::is_trivially_copyable_v<T>) {
					std::memcpy(m_Begin + index, std::addressof(value), sizeof(T));
				} else {
					m_Begin[index] = std::forward<U>(value);
				}
			} else {
				if constexpr(std::is_trivially_copyable_v<T>) {
					std::memcpy(m_Begin + index, std::addressof(*value), sizeof(T));
				} else {
					psl::ecs::accessor::assign<T>(m_Begin + index, *value);
				}
			}
		}

		void swap(Key first, Key second) {
			psl_assert(first < size() && second < size(), "index out of bounds");
			if constexpr(std::is_trivially_copyable_v<T>) {
				std::swap_ranges(m_Begin + first, m_Begin + first + 1, m_Begin + second);
			} else {
				T tmp;
				psl::ecs::accessor::assign<T>(&tmp, std::move(*(m_Begin + first)));
				psl::ecs::accessor::assign<T>(m_Begin + first, std::move(m_Begin[second]));
				psl::ecs::accessor::assign<T>(m_Begin + second, std::move(tmp));
			}
		}

		void emplace_back() {
			psl_assert(size() < capacity(), "no more space left to create new elements");
			if constexpr(std::is_trivially_constructible_v<T>) {
				// no-op for trivial types, when available use std::start_lifetime_as<T>(m_End);
			} else {
				psl::ecs::accessor::construct_at<T>(m_End);
			}
			++m_End;
		}

		template <typename U>
		void emplace_back(U&& value) {
			psl_assert(size() < capacity(), "no more space left to create new elements");
			if constexpr(std::is_trivially_copyable_v<T>) {
				std::memcpy(m_End, &*value, sizeof(T));
				// when available use std::start_lifetime_as<T>(m_End);
			} else {
				psl::ecs::accessor::construct_at<T>(m_End, *value);
			}
			++m_End;
		}

		void reserve(Key new_size) {
			const auto cap = capacity();
			if(new_size > cap) {
				new_size			 = (std::max)(new_size, cap + (cap / 2));
				auto const old_count = m_End - m_Begin;
				auto const old_size	 = old_count * sizeof(T);

				::memory::raw_region new_dense(new_size * sizeof(T));
				psl_assert(new_dense.size() >= old_size,
						   "new dense size must be greater than or equal to current size");
				if constexpr(std::is_trivially_copyable_v<T>) {
					std::memcpy(new_dense.data(), m_Dense.data(), old_size);
					// when available use std::start_lifetime_as<T>(...);
				} else {
					auto currentPtr		= m_Begin;
					auto newTargetPtr	= (T*)new_dense.data();
					auto new_dense_size = new_dense.size();
					newTargetPtr		= (T*)std::align(alignof(T), sizeof(T), (void*&)newTargetPtr, new_dense_size);
					psl_assert(new_dense_size >= (m_End - m_Begin) * sizeof(T),
							   "new dense size must be greater than or equal to current size");
					for(size_t i = 0; i < size(); ++i) {
						psl::ecs::accessor::construct_at<T>(newTargetPtr, std::move(*currentPtr));
						if constexpr(!std::is_trivially_destructible_v<T>) {
							currentPtr->~T();
						}
						++currentPtr;
						++newTargetPtr;
					}
				}
				m_Dense = std::move(new_dense);
				recalculate_storage_size();
			}
		}

		void insert_space(Key index, Key count) {
			psl_assert(index <= size(), "index out of bounds");
			psl_assert(size() + count <= capacity(), "no more space left to create new elements");
			if constexpr(std::is_trivially_copyable_v<T>) {
				auto dst  = m_Begin + index + count;
				auto src  = m_Begin + index;
				auto size = m_End - src;
				std::memmove(dst, src, size * sizeof(T));
				if constexpr(!std::is_trivially_constructible_v<T>) {
					for(size_t i = 0; i < count; ++i) {
						new(m_Begin + index + i) T();
					}
				}
			} else {
				auto currentPtr = m_End;
				auto targetPtr	= m_End + count;
				while(currentPtr > m_Begin + index) {
					--currentPtr;
					--targetPtr;
					psl::ecs::accessor::construct_at<T>(targetPtr, std::move(*currentPtr));
				}
			}
			m_End += count;
		}

		void truncate(Key new_size) {
			psl_assert(new_size <= size(), "new size must be less than or equal to current size");
			auto const old_size = size();
			if constexpr(!std::is_trivially_destructible_v<T>) {
				for(size_t i = new_size; i < old_size; ++i) {
					(--m_End)->~T();
				}
			} else {
				m_End -= (old_size - new_size);
			}
			std::memset(static_cast<void*>(m_End), 0, (old_size - new_size) * sizeof(T));
		}

		void rotate(Key begin, Key middle, Key last) {
			psl_assert(begin < size() && middle < size() && last <= size(),
					   "index out of bounds, rotate got begin | middle | last: {} {} {} out of size: {}",
					   begin,
					   middle,
					   last,
					   size());
			auto firstPtr = m_Begin + begin;
			auto midPtr	  = m_Begin + middle;
			auto lastPtr  = m_Begin + last;
			if(firstPtr == midPtr || midPtr == lastPtr) {
				return;
			}

			// implement rotate using three reverses
			auto reverse = [](T* start, T* end) {
				while((start < end) && (start != --end)) {
					if constexpr(std::is_trivially_copyable_v<T>) {
						std::swap_ranges(start, start + 1, end);
					} else {
						T tmp;
						psl::ecs::accessor::assign<T>(&tmp, std::move(*start));
						psl::ecs::accessor::assign<T>(start, std::move(*end));
						psl::ecs::accessor::assign<T>(end, std::move(tmp));
					}
					++start;
				}
			};
			reverse(firstPtr, midPtr);
			reverse(midPtr, lastPtr);
			reverse(firstPtr, lastPtr);
		}

		T* begin() noexcept {
			return m_Begin;
		}
		T* end() noexcept {
			return m_End;
		}
		const T* begin() const noexcept {
			return m_Begin;
		}
		const T* end() const noexcept {
			return m_End;
		}

		auto can_merge([[maybe_unused]] const dense_storage_base_t* other) const noexcept -> bool {
			return true;
		}
		template <typename U>
		auto can_merge([[maybe_unused]] const dense_storage_base_t<U, Key>* other) const noexcept -> bool {
			return false;
		}

		void clear(bool release_memory = false) noexcept {
			if constexpr(std::is_trivially_destructible_v<T>) {
				std::memset(m_Begin, 0, size() * sizeof(T));
			} else {
				for(auto it = m_Begin; it < m_End; ++it) {
					it->~T();
				}
			}
			m_End = m_Begin;

			if(release_memory) {
				m_Dense = ::memory::raw_region(16 * sizeof(T));
				recalculate_storage_size();
			}
		}

		T* unsafe_data(Key index) {
			// note we can get one past the end of the array
			psl_assert(index <= size(), "index out of bounds");
			return m_Begin + index;
		}

		T* const unsafe_data(Key index) const noexcept {
			// note we can get one past the end of the array
			psl_assert(index <= size(), "index out of bounds");
			return m_Begin + index;
		}

	  private:
		// invoke after a resize has happened
		void recalculate_storage_size() {
			auto const pre_existing_size = m_End - m_Begin;
			m_Begin						 = (T*)m_Dense.data();
			auto total_size				 = m_Dense.size();
			m_End		 = (T*)std::align(alignof(T), sizeof(T), (void*&)m_Begin, total_size) + pre_existing_size;
			m_StorageEnd = m_Begin + (total_size / sizeof(T));

			psl_assert(m_Dense.end() >= (void*)m_Begin || pre_existing_size == 0,
					   "aligned begin address is greater than or equal to end address");
			if(pre_existing_size > 0) {
				m_Begin = std::launder(m_Begin);
				m_End	= m_Begin + pre_existing_size;
			}
		}

		::memory::raw_region m_Dense;
		T* m_Begin {nullptr};
		T* m_End {nullptr};
		T* m_StorageEnd {nullptr};
	};

	template <typename Key>
	struct dense_storage_base_t<untyped_tag_t, Key> {
		dense_storage_base_t(Key initial_size, Key type_size, Key alignment_size)
			: m_Dense(initial_size * type_size), m_TypeSize(type_size), m_TypeAlignment(alignment_size) {
			psl_assert(m_TypeSize > 0, "type size must be greater than 0");
			recalculate_storage_size();
		}
		~dense_storage_base_t() = default;

		Key capacity() const noexcept {
			return psl::narrow_cast<Key>((m_StorageEnd - m_Begin) / m_TypeSize);
		}
		Key size() const noexcept {
			return psl::narrow_cast<Key>((m_End - m_Begin) / m_TypeSize);
		}

		auto can_merge(const dense_storage_base_t* other) const noexcept -> bool {
			return m_TypeSize == other->m_TypeSize && m_TypeAlignment == other->m_TypeAlignment;
		}

		template <typename U>
		auto can_merge(const dense_storage_base_t<U, Key>* other) const noexcept -> bool {
			return false;
		}

		template <typename U>
		void set(Key index, U&& value) {
			psl_assert(index < size(), "index {} out of bounds {}", index, size());
			if constexpr(std::is_same_v<std::remove_cvref_t<U>, untyped_iterator_t>) {
				psl_assert(value.m_TypeSize == m_TypeSize, "type size mismatch");
			}
			std::memcpy(index_to_memory_offset(index), &*value, m_TypeSize);
		}

		void swap(Key first, Key second) {
			psl_assert(first < size() && second < size(),
					   "index out of bounds, tried to access indices [{}, {}] out of {}",
					   first,
					   second,
					   size());
			std::byte* first_ptr  = index_to_memory_offset(first);
			std::byte* second_ptr = index_to_memory_offset(second);

			std::swap_ranges(first_ptr, first_ptr + m_TypeSize, second_ptr);
		}

		void emplace_back() {
			psl_assert(size() < capacity(), "no more space left to create new elements");
			m_End += m_TypeSize;
		}

		template <typename U>
		void emplace_back(U&& value) {
			psl_assert(size() < capacity(), "no more space left to create new elements");
			if constexpr(std::is_same_v<std::remove_cvref_t<U>, untyped_iterator_t>) {
				psl_assert(value.m_TypeSize == m_TypeSize, "type size mismatch");
			}
			std::memcpy(m_End, &*value, m_TypeSize);

			m_End += m_TypeSize;
		}

		void reserve(Key new_size) {
			if(new_size > capacity()) {
				new_size			= (std::max)(new_size, capacity() + (capacity() / 2));
				auto const old_size = m_End - m_Begin;

				::memory::raw_region new_dense(new_size * m_TypeSize);
				std::memcpy(new_dense.data(), m_Dense.data(), old_size);
				m_Dense = std::move(new_dense);
				recalculate_storage_size();
				psl_assert(new_size <= capacity(),
						   "failed to allocate the requested amount, wanted {}, but got {}",
						   new_size,
						   capacity());
			}
		}
		void insert_space(Key index, Key count) {
			psl_assert(index <= size(), "index out of bounds");
			psl_assert(size() + count <= capacity(), "no more space left to create new elements");
			auto dst  = m_Begin + ((index + count) * m_TypeSize);
			auto src  = m_Begin + (index * m_TypeSize);
			auto size = m_End - src;
			std::memmove(dst, src, size);
			m_End += count * m_TypeSize;
		}

		void truncate(Key new_size) {
			psl_assert(new_size <= size(), "new size must be less than or equal to current size");
			m_End = m_Begin + (new_size * m_TypeSize);
		}

		void rotate(Key begin, Key middle, Key last) {
			psl_assert(begin <= size() && middle <= size() && last <= size(),
					   "out of bounds access, tried to access index [{}, {}, {}] out of the allowed {} elements",
					   begin,
					   middle,
					   last,
					   size() + 1);
			if(begin == middle || middle == last) {
				return;
			}

			Key n = middle - begin;
			Key m = last - middle;

			while(n != 0 && m != 0) {
				if(n <= m) {
					swap_chunks(begin, last - n, n);
					last -= n;
					m -= n;
				} else {
					swap_chunks(begin, middle, m);
					begin += m;
					n -= m;
				}
			}
		}

		void clear(bool release_memory = false) noexcept {
			std::memset(m_Begin, 0, size() * m_TypeSize);
			m_End = m_Begin;

			if(release_memory) {
				m_Dense = ::memory::raw_region(16 * m_TypeSize);
				recalculate_storage_size();
			}
		}


		untyped_iterator_t begin() noexcept {
			return {m_Begin, m_TypeSize};
		}
		untyped_iterator_t end() noexcept {
			return {m_End, m_TypeSize};
		}
		untyped_iterator_t begin() const noexcept {
			return {m_Begin, m_TypeSize};
		}
		untyped_iterator_t end() const noexcept {
			return {m_End, m_TypeSize};
		}

		std::byte* unsafe_data(Key index) {
			psl_assert(index < size() + 1, "Out of bounds: attempted to access index {} out of {}", index, size() + 1);
			return m_Begin + (index * m_TypeSize);
		}
		std::byte* const unsafe_data(Key index) const noexcept {
			psl_assert(index < size() + 1, "Out of bounds: attempted to access index {} out of {}", index, size() + 1);
			return m_Begin + (index * m_TypeSize);
		}

		Key type_size() const noexcept {
			return m_TypeSize;
		}

		Key type_alignment() const noexcept {
			return m_TypeAlignment;
		}

	  private:
		void swap_chunks(Key first_idx, Key second_idx, Key count) {
			std::byte* first  = m_Begin + (first_idx * m_TypeSize);
			std::byte* second = m_Begin + (second_idx * m_TypeSize);

			for(Key i = 0; i < count; ++i) {
				std::swap_ranges(first, first + m_TypeSize, second);
				first += m_TypeSize;
				second += m_TypeSize;
			}
		}

		std::byte* index_to_memory_offset(Key index) const {
			psl_assert(index < capacity(), "index out of bounds");
			return m_Begin + (index * m_TypeSize);
		}

		void recalculate_storage_size() {
			auto const pre_existing_size = m_End - m_Begin;

			m_Begin			= (std::byte*)m_Dense.data();
			auto total_size = m_Dense.size();
			m_End =
			  (std::byte*)std::align(m_TypeAlignment, m_TypeSize, (void*&)m_Begin, total_size) + pre_existing_size;
			m_StorageEnd = m_Begin + total_size;
			psl_assert(m_Dense.end() >= m_Begin || pre_existing_size == 0,
					   "aligned begin address is greater than or equal to end address");
		}

		::memory::raw_region m_Dense;
		std::byte* m_Begin {nullptr};
		std::byte* m_End {nullptr};
		std::byte* m_StorageEnd {nullptr};
		Key m_TypeSize;
		Key m_TypeAlignment;
	};

	template <typename Key>
	struct dense_storage_base_t<flag_tag_t, Key> {
		Key capacity() const {
			return 0;
		}
		Key size() const {
			return 0;
		}
		auto can_merge([[maybe_unused]] const dense_storage_base_t* other) const noexcept -> bool {
			return true;
		}
		template <typename U>
		auto can_merge([[maybe_unused]] const dense_storage_base_t<U, Key>* other) const noexcept -> bool {
			return false;
		}
		template <typename U>
		void set(Key index, U&& value) {}
		void swap(Key first, Key second) {}
		void emplace_back() {}
		template <typename U>
		void emplace_back(U&& value) {}
		void reserve(Key new_size) {}
		void insert_space(Key index, Key count) {}
		void truncate(Key new_size) {}
		void rotate(Key begin, Key middle, Key last) {}
		void* begin() noexcept {
			return nullptr;
		}
		void* end() noexcept {
			return nullptr;
		}
		void* begin() const noexcept {
			return nullptr;
		}
		void* end() const noexcept {
			return nullptr;
		}
		void clear(bool release_memory = false) noexcept {}
		void* unsafe_data(Key index) {
			return nullptr;
		}
		void* const unsafe_data(Key index) const noexcept {
			return nullptr;
		}
	};

	template <typename T, typename Value>
	concept IsIteratorLikeType =
	  std::is_pointer_v<T> || std::is_same_v<std::remove_cvref_t<decltype(*std::declval<T>())>, Value>;
}	 // namespace impl

template <typename T,
		  typename Key			= psl::ecs::entity_t,
		  typename IndexType	= psl::ecs::entity_t::size_type,
		  IndexType CHUNKS_SIZE = component_traits_t<T>::storage_chunks_size>
class staged_sparse_array final : private impl::dense_storage_base_t<T, IndexType> {
	static_assert(std::is_standard_layout_v<Key> && sizeof(Key) == sizeof(IndexType),
				  "Key must be a standard layout type and fit within IndexType");
	static_assert(std::is_same_v<T, std::remove_cvref_t<T>>,
				  "staged_sparse_array does not support cvref types, please use a non-cvref type");
	static_assert(std::is_pointer_v<T> == false,
				  "staged_sparse_array does not support pointer types, please use a non-pointer type");
	static constexpr auto IS_TRIVIAL	= std::is_same_v<untyped_tag_t, T>;
	static constexpr auto IS_FLAG		= std::is_same_v<flag_tag_t, T>;
	static constexpr auto IS_COMPLEX	= !IS_TRIVIAL && !IS_FLAG;
	static constexpr auto IS_ASSIGNABLE = !IS_FLAG;
	static constexpr auto TOMBSTONE		= std::numeric_limits<IndexType>::max();

	static constexpr bool IS_CHUNKS_POW_2 {CHUNKS_SIZE && ((CHUNKS_SIZE & (CHUNKS_SIZE - 1)) == 0)};
	static constexpr IndexType MOD_VAL {(IS_CHUNKS_POW_2) ? CHUNKS_SIZE - 1 : CHUNKS_SIZE};

	constexpr FORCEINLINE IndexType& convert_from_user_type(Key& x) const noexcept {
		return *reinterpret_cast<IndexType*>(&x);
	}
	constexpr FORCEINLINE IndexType& convert_from_user_type(Key const& x) const noexcept {
		return const_cast<IndexType&>(*reinterpret_cast<IndexType const*>(&x));
	}

	template <typename Y>
		requires(std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Y>())>, Key>)
	constexpr FORCEINLINE IndexType& convert_from_user_type(Y& x) const noexcept {
		return convert_from_user_type(*x);
	}

	template <typename Y>
		requires(std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Y>())>, Key>)
	constexpr FORCEINLINE IndexType& convert_from_user_type(Y const& x) const noexcept {
		return convert_from_user_type(*x);
	}

	constexpr FORCEINLINE Key& convert_to_user_type(IndexType& x) const noexcept {
		return *reinterpret_cast<Key*>(&x);
	}

	constexpr FORCEINLINE Key& convert_to_user_type(IndexType const& x) const noexcept {
		return const_cast<Key&>(*reinterpret_cast<Key const*>(&x));
	}
	template <typename Y>
		requires(std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Y>())>, IndexType>)
	constexpr FORCEINLINE Key& convert_to_user_type(Y& x) const noexcept {
		return convert_to_user_type(*x);
	}
	template <typename Y>
		requires(std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Y>())>, IndexType>)
	constexpr FORCEINLINE Key& convert_to_user_type(Y const& x) const noexcept {
		return convert_to_user_type(*x);
	}


	using this_type			 = staged_sparse_array<T, Key, IndexType, CHUNKS_SIZE>;
	using user_index_type	 = Key;
	using index_type		 = IndexType;
	using dense_storage_type = impl::dense_storage_base_t<T, IndexType>;
	using chunk_type		 = psl::array<index_type>;
	using chunk_ptr_type	 = std::unique_ptr<chunk_type>;
	using chunk_storage_type = psl::array<chunk_ptr_type>;

	using size_type			= typename psl::array<index_type>::size_type;
	using difference_type	= typename psl::array<index_type>::difference_type;
	using value_type		= std::conditional_t<IS_COMPLEX, T, std::byte>;
	using pointer			= value_type*;
	using const_pointer		= value_type* const;
	using reference			= value_type&;
	using const_reference	= const value_type&;
	using iterator_category = std::random_access_iterator_tag;

  public:
	staged_sparse_array(index_type initial_size = 16)
		requires(IS_FLAG)
		: dense_storage_type() {
		m_Reverse.reserve(initial_size);
	}
	staged_sparse_array(index_type initial_size = 16)
		requires(IS_COMPLEX)
		: dense_storage_type(initial_size) {
		m_Reverse.reserve(initial_size);
	}
	staged_sparse_array(index_type type_size, index_type alignment_size, index_type initial_size = 16)
		requires(IS_TRIVIAL)
		: dense_storage_type(initial_size, type_size, alignment_size) {
		m_Reverse.reserve(initial_size);
	}

	staged_sparse_array(staged_sparse_array const&)			   = delete;
	staged_sparse_array(staged_sparse_array&&)				   = delete;
	staged_sparse_array& operator=(staged_sparse_array const&) = delete;
	staged_sparse_array& operator=(staged_sparse_array&&)	   = delete;

	~staged_sparse_array() = default;

	/// \brief Get a view of the underlying data for the given `stage_range_t`
	/// \tparam T Component type to interpret the data as
	/// \param stage `stage_range_t` to limit what data is returned
	/// \return A view of the underlying data as the requested type
	template <typename U = T>
	FORCEINLINE auto
	dense(stage_range_t stage = stage_range_t::ALIVE) const noexcept -> psl::array_view<value_type const>
		requires(IS_COMPLEX)
	{
		static_assert(std::is_same_v<value_type, U>, "dense_storage_type type does not match requested type");
		return psl::array_view<T const> {dense_storage_type::unsafe_data(m_StageStart[stage_begin(stage)]),
										 dense_storage_type::unsafe_data(m_StageStart[stage_end(stage)])};
	}

	template <typename U = T>
	FORCEINLINE auto dense(stage_range_t stage = stage_range_t::ALIVE) noexcept -> psl::array_view<value_type>
		requires(IS_COMPLEX)
	{
		static_assert(std::is_same_v<value_type, U>, "dense_storage_type type does not match requested type");
		return psl::array_view<T> {dense_storage_type::unsafe_data(m_StageStart[stage_begin(stage)]),
								   dense_storage_type::unsafe_data(m_StageStart[stage_end(stage)])};
	}

	/// \brief Get a view of the underlying data for the given `stage_range_t`
	/// \tparam T Component type to interpret the data as
	/// \param stage `stage_range_t` to limit what data is returned
	/// \return A view of the underlying data as the requested type
	template <typename U>
	FORCEINLINE auto dense(stage_range_t stage = stage_range_t::ALIVE) const noexcept -> psl::array_view<U const>
		requires(IS_TRIVIAL)
	{
		psl_assert(dense_storage_type::type_size() == sizeof(U),
				   "dense_storage_type type size does not match requested type size");
		psl_assert(dense_storage_type::type_alignment() == alignof(U),
				   "dense_storage_type type alignment does not match requested type alignment");
		return psl::array_view<U const> {(U*)dense_storage_type::unsafe_data(m_StageStart[stage_begin(stage)]),
										 (U*)dense_storage_type::unsafe_data(m_StageStart[stage_end(stage)])};
	}

	template <typename U>
	FORCEINLINE auto dense(stage_range_t stage = stage_range_t::ALIVE) noexcept -> psl::array_view<U>
		requires(IS_TRIVIAL)
	{
		psl_assert(dense_storage_type::type_size() == sizeof(U),
				   "dense_storage_type type size does not match requested type size");
		psl_assert(dense_storage_type::type_alignment() == alignof(U),
				   "dense_storage_type type alignment does not match requested type alignment");
		return psl::array_view<U> {(U*)dense_storage_type::unsafe_data(m_StageStart[stage_begin(stage)]),
								   (U*)dense_storage_type::unsafe_data(m_StageStart[stage_end(stage)])};
	}

	/// \brief Get the data pointer for the given stage (where the data begins)
	/// \param stage `stage_t` to retrieve
	/// \return A pointer to the head of the dense data
	FORCEINLINE auto data(stage_t stage = stage_t::SETTLED) noexcept -> pointer
		requires(IS_ASSIGNABLE)
	{
		return dense_storage_type::unsafe_data(m_StageStart[to_underlying(stage)]);
	}

	/// \brief Get the data pointer for the given stage (where the data begins)
	/// \param stage `stage_t` to retrieve
	/// \return A pointer to the head of the dense data
	FORCEINLINE auto cdata(stage_t stage = stage_t::SETTLED) const noexcept -> const_pointer
		requires(IS_ASSIGNABLE)
	{
		return dense_storage_type::unsafe_data(m_StageStart[to_underlying(stage)]);
	}

	FORCEINLINE auto data(stage_t stage = stage_t::SETTLED) const noexcept -> const_pointer
		requires(IS_ASSIGNABLE)
	{
		return cdata(stage);
	}

	/// \brief Get a reference of the requested type at the index
	/// \tparam T Type we want to interpret the data as
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return Given memory address as a const ref
	/// \note When assertions are enabled, this function can assert
	FORCEINLINE auto at(user_index_type index,
						stage_range_t stage = stage_range_t::ALIVE) const noexcept -> value_type const&
		requires(IS_COMPLEX)
	{
		return *addressof(index, stage);
	}


	/// \brief Get a reference of the requested type at the index
	/// \tparam T Type we want to interpret the data as
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return Given memory address as a const ref
	/// \note When assertions are enabled, this function can assert
	FORCEINLINE auto at(user_index_type index, stage_range_t stage = stage_range_t::ALIVE) noexcept -> value_type&
		requires(IS_COMPLEX)
	{
		return *addressof(index, stage);
	}


	template <typename U>
	FORCEINLINE auto at(user_index_type index, stage_range_t stage = stage_range_t::ALIVE) const noexcept -> U const&
		requires(IS_COMPLEX || IS_TRIVIAL)
	{
		if constexpr(IS_TRIVIAL) {
			psl_assert(dense_storage_type::type_size() == sizeof(U),
					   "dense_storage_type type size does not match requested type size");
			psl_assert(dense_storage_type::type_alignment() == alignof(U),
					   "dense_storage_type type alignment does not match requested type alignment");
		} else {
			static_assert(std::is_same_v<value_type, U>, "dense_storage_type type does not match requested type");
		}
		return *reinterpret_cast<U* const>(addressof(index, stage));
	}

	template <typename U>
	FORCEINLINE auto at(user_index_type index, stage_range_t stage = stage_range_t::ALIVE) noexcept -> U&
		requires(IS_COMPLEX || IS_TRIVIAL)
	{
		if constexpr(IS_TRIVIAL) {
			psl_assert(dense_storage_type::type_size() == sizeof(U),
					   "dense_storage_type type size does not match requested type size");
			psl_assert(dense_storage_type::type_alignment() == alignof(U),
					   "dense_storage_type type alignment does not match requested type alignment");
		} else {
			static_assert(std::is_same_v<value_type, U>, "dense_storage_type type does not match requested type");
		}
		return *reinterpret_cast<U*>(addressof(index, stage));
	}

	/// \brief Get a pointer of the data at the index
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return memory address
	/// \note When assertions are enabled, this function can assert
	FORCEINLINE auto addressof(user_index_type index,
							   stage_range_t range = stage_range_t::ALIVE) const noexcept -> const_pointer
		requires(IS_ASSIGNABLE)
	{
		auto element_index = convert_from_user_type(index);
		auto chunk		   = userspace_to_internal(element_index);
		psl_assert(chunk != TOMBSTONE && m_Sparse[chunk] != nullptr, "Could not find the data entry");
		auto dense_index = (*m_Sparse[chunk])[element_index];
		psl_assert(
		  ((stage_range_for_index(dense_index) & range) == stage_range_for_index(dense_index)),
		  "Data was not in the expected stage_range_t. User index {}'s dense index {} was expected between {} and {}",
		  convert_from_user_type(index),
		  dense_index,
		  m_StageStart[stage_begin(range)],
		  m_StageStart[stage_end(range)]);

		return dense_storage_type::unsafe_data(dense_index);
	}

	/// \brief Get a pointer of the data at the index
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return memory address
	/// \note When assertions are enabled, this function can assert
	FORCEINLINE auto addressof(user_index_type index, stage_range_t range = stage_range_t::ALIVE) noexcept -> pointer
		requires(IS_ASSIGNABLE)
	{
		auto element_index = convert_from_user_type(index);
		auto chunk		   = userspace_to_internal(element_index);
		psl_assert(chunk != TOMBSTONE && m_Sparse[chunk] != nullptr, "Could not find the data entry");
		auto dense_index = (*m_Sparse[chunk])[element_index];
		psl_assert(
		  ((stage_range_for_index(dense_index) & range) == stage_range_for_index(dense_index)),
		  "Data was not in the expected stage_range_t. User index {}'s dense index {} was expected between {} and {}",
		  convert_from_user_type(index),
		  dense_index,
		  m_StageStart[stage_begin(range)],
		  m_StageStart[stage_end(range)]);
		return dense_storage_type::unsafe_data(dense_index);
	}

	/// \brief Get a pointer of the data at the index, or nullptr when not found
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return memory address or nullptr
	FORCEINLINE auto addressof_if(user_index_type index,
								  stage_range_t range = stage_range_t::ALIVE) const noexcept -> const_pointer
		requires(IS_ASSIGNABLE)
	{
		auto chunk = userspace_to_internal(convert_from_user_type(index));
		if(chunk == TOMBSTONE || !m_Sparse[chunk]) {
			return nullptr;
		}
		auto dense_index = (*m_Sparse[chunk])[convert_from_user_type(index)];
		auto const stage = stage_range_for_index(dense_index);
		if(dense_index == TOMBSTONE || (stage & range) != stage) {
			return nullptr;
		}
		return dense_storage_type::unsafe_data(dense_index);
	}

	/// \brief Get a pointer of the data at the index, or nullptr when not found
	/// \param index Where to look
	/// \param stage Used to limit the stages we wish to look in
	/// \return memory address or nullptr
	FORCEINLINE auto addressof_if(user_index_type index, stage_range_t range = stage_range_t::ALIVE) noexcept -> pointer
		requires(IS_ASSIGNABLE)
	{
		auto chunk = userspace_to_internal(convert_from_user_type(index));
		if(chunk == TOMBSTONE || !m_Sparse[chunk]) {
			return nullptr;
		}
		auto dense_index = (*m_Sparse[chunk])[convert_from_user_type(index)];
		auto const stage = stage_range_for_index(dense_index);
		if(dense_index == TOMBSTONE || (stage & range) != stage) {
			return nullptr;
		}
		return dense_storage_type::unsafe_data(dense_index);
	}

	template <typename ItIndexFirst, typename ItIndexLast, typename ItDataFirst>
	FORCEINLINE auto write_dense_indices(ItIndexFirst it_index_first,
										 ItIndexLast it_index_last,
										 ItDataFirst it_target_first) const noexcept
		requires(IS_ASSIGNABLE)
	{
		auto index_span = psl::impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= psl::impl::to_span_wrapper(it_target_first, it_target_first + index_span.size());
		std::as_const(*this).template invoke_for_l0<false, false>(
		  index_span,
		  [](auto index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset, auto dataIt) {
			  psl_assert(chunk[chunk_offset] != TOMBSTONE, "expected valid index in sparse array");
			  *dataIt = chunk[chunk_offset];
		  },
		  data_span);
	}

	template <typename ItIndexFirst, typename ItIndexLast, typename ItDataFirst>
	FORCEINLINE auto
	copy_dense_into(ItIndexFirst it_index_first, ItIndexLast it_index_last, ItDataFirst it_target_first) const noexcept
		requires(IS_ASSIGNABLE)
	{
		auto index_span = psl::impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= psl::impl::to_span_wrapper(it_target_first, it_target_first + index_span.size());
		std::as_const(*this).template invoke_for_l0<false, false>(
		  index_span,
		  [this](auto index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset, auto dataIt) {
			  psl_assert(chunk[chunk_offset] != TOMBSTONE, "expected valid index in sparse array");
			  if constexpr(IS_COMPLEX) {
				  if constexpr(std::is_trivially_copyable_v<value_type>) {
					  std::memcpy(&*dataIt, dense_storage_type::unsafe_data(chunk[chunk_offset]), sizeof(value_type));
				  } else {
					  psl::ecs::accessor::construct_at<value_type>(
						dataIt, *dense_storage_type::unsafe_data(chunk[chunk_offset]));
				  }
			  } else {
				  std::memcpy(
					&*dataIt, dense_storage_type::unsafe_data(chunk[chunk_offset]), dense_storage_type::type_size());
			  }
		  },
		  data_span);
	}

	constexpr FORCEINLINE auto size(stage_range_t stage = stage_range_t::ALIVE) const noexcept -> index_type {
		if(stage == stage_range_t::NOT_ADDED) {
			return m_StageSize[0] + m_StageSize[2];
		}
		return m_StageStart[stage_end(stage)] - m_StageStart[stage_begin(stage)];
	}

	FORCEINLINE constexpr auto empty() const noexcept -> bool {
		return m_StageStart[3] == 0;
	};

	constexpr FORCEINLINE auto capacity() const noexcept -> index_type {
		return dense_storage_type::capacity();
	}

	constexpr FORCEINLINE void reserve(index_type count) noexcept {
		m_Reverse.reserve(count);
		if(IS_ASSIGNABLE) {
			dense_storage_type::reserve(count);
		}
	}

	/// \brief Get a view of the indices for the given `stage_range_t`
	/// \param stage `stage_range_t` to limit what indices are returned
	/// \return A view of the indices
	FORCEINLINE auto
	indices(stage_range_t range = stage_range_t::ALIVE) const noexcept -> psl::array_view<user_index_type> {
		if(m_Reverse.empty()) {
			return {};
		}
		return psl::array_view<user_index_type>(
		  &convert_to_user_type(*m_Reverse.data()) + m_StageStart[stage_begin(range)],
		  &convert_to_user_type(*m_Reverse.data()) + m_StageStart[stage_end(range)]);
	}

	/// \brief Checks if the given index exist in the requested range
	/// \param index Index to check
	/// \param stage Stage that will be searched in
	/// \return Boolean containing true if found
	FORCEINLINE auto has(user_index_type index, stage_range_t range = stage_range_t::ALIVE) const noexcept -> bool {
		auto chunk_index = userspace_to_internal(convert_from_user_type(index));
		if(chunk_index == TOMBSTONE || !m_Sparse[chunk_index]) {
			return false;
		}
		auto reverse	 = (*m_Sparse[chunk_index])[convert_from_user_type(index)];
		auto const stage = stage_range_for_index(reverse);
		return reverse != TOMBSTONE && ((stage & range) == stage);
	}

	template <bool PreSorted = false, typename IndexItFirst, typename IndexItLast>
	FORCEINLINE auto for_each_generator(IndexItFirst it_index_first, IndexItLast it_index_last) {
		psl_assert(std::is_sorted(it_index_first, it_index_last), "This method requires sorted indices");

		if constexpr(!PreSorted) {
			if(std::is_sorted(it_index_first, it_index_last)) {
				return for_each_generator<true>(it_index_first, it_index_last);
			}
		}

		return [this,
				end			  = it_index_last,
				it			  = it_index_first,
				prev_treshold = size_t {0},
				next_treshold = size_t {0},
				first_index	  = index_type {0},
				element_index = index_type {0},
				chunkPtr	  = (chunk_type*)nullptr]() mutable {
			if(it == end) {
				return std::make_pair(std::numeric_limits<index_type>::max(), details::stage_t {4});
			}
			auto next_index = index_type {0};
			auto rev_index	= index_type {0};
			for(;;) {
				if(it == end) {
					break;
				}

				for(;;) {
					first_index = *it;
					if constexpr(PreSorted) {
						if(chunkPtr != nullptr && first_index >= next_treshold) {
							break;
						}
					} else {
						if(chunkPtr != nullptr && (first_index < prev_treshold || first_index >= next_treshold)) {
							break;
						}
					}
					index_type chunk_index {};
					chunk_info_for(first_index, element_index, chunk_index);
					prev_treshold = chunk_index * CHUNKS_SIZE;
					next_treshold = prev_treshold + CHUNKS_SIZE;
					if(chunk_index > m_Sparse.size()) {
						it = end;
						return std::make_pair(std::numeric_limits<index_type>::max(), details::stage_t {4});
					}
					chunkPtr = m_Sparse[chunk_index].get();
					if(!chunkPtr) {
						for(;;) {
							if(it == end) {
								return std::make_pair(std::numeric_limits<index_type>::max(), details::stage_t {4});
							}
							auto const next_index = *it;

							if constexpr(PreSorted) {
								if(next_index >= next_treshold) {
									first_index = next_index;
									break;
								}
							} else {
								if(next_index < prev_treshold || next_index >= next_treshold) {
									first_index = next_index;
									break;
								}
							}
							++it;
						}
					}
				}

				next_index					  = *it;
				auto const next_element_index = next_index - static_cast<index_type>(prev_treshold);
				++it;
				rev_index = (*chunkPtr)[next_element_index];
				if(rev_index != TOMBSTONE) {
					break;
				}
			}

			if(rev_index != TOMBSTONE) {
				auto const what_stage = rev_index < m_StageStart[1]	  ? stage_t::SETTLED
										: rev_index < m_StageStart[2] ? stage_t::ADDED
																	  : stage_t::REMOVED;

				return std::make_pair(next_index, what_stage);
			}
			return std::make_pair(std::numeric_limits<index_type>::max(), details::stage_t {4});
		};
	}

	/// \brief Iterates through the given range, removing the elements that do not exist in the given stage range
	template <typename IndexItFirst, typename IndexItLast>
	FORCEINLINE auto remove_if_has(IndexItFirst it_index_first,
								   IndexItLast it_index_last,
								   stage_range_t range = stage_range_t::ALL) const {
		return remove_if_has_impl<true>(it_index_first, it_index_last, range);
	}

	/// \brief Iterates through the given range, removing the elements that do not exist in the given stage range
	template <typename IndexItFirst, typename IndexItLast>
	FORCEINLINE auto remove_if_has_not(IndexItFirst it_index_first,
									   IndexItLast it_index_last,
									   stage_range_t range = stage_range_t::ALL) const {
		return remove_if_has_impl<false>(it_index_first, it_index_last, range);
	}

	/// \brief Iterates through the given range, removing the elements that do not exist in the given stage range
	template <bool Operation = false, bool PreSorted = false, typename IndexItFirst, typename IndexItLast>
	FORCEINLINE auto remove_if_has_impl(IndexItFirst it_index_first,
										IndexItLast it_index_last,
										stage_range_t range = stage_range_t::ALL) const {
		if(it_index_first == it_index_last) {
			return it_index_last;
		}

		if constexpr(!PreSorted) {
			if(std::is_sorted(it_index_first, it_index_last)) {
				return remove_if_has_impl<Operation, true>(it_index_first, it_index_last, range);
			}
		}

		auto current	= it_index_first;
		auto const last = it_index_last;
		auto valid		= current;

		do {
			auto const first_index = convert_from_user_type(*current);
			index_type chunk_index {};
			index_type element_index {};
			chunk_info_for(first_index, element_index, chunk_index);
			size_t prev_treshold {(chunk_index)*CHUNKS_SIZE};
			size_t next_treshold {prev_treshold + CHUNKS_SIZE};
			if constexpr(PreSorted) {
				// if the returned chunk_index is larger than the amount we've stored
				// we can safely terminate the loop due to the source being sorted.
				if(chunk_index >= m_Sparse.size()) {
					return valid;
				}
			} else {
				for(;;) {
					if(current == last) {
						return valid;
					}
					auto const next_index = convert_from_user_type(*current);
					if(next_index < prev_treshold || next_index >= next_treshold) {
						break;
					}
					++current;
				}
				continue;
			}
			auto& chunkPtr = m_Sparse[chunk_index];
			// skip the chunk in case there's nothing in it
			if(!chunkPtr) {
				for(;;) {
					if(current == last) {
						return valid;
					}
					auto const next_index = convert_from_user_type(*current);
					if constexpr(PreSorted) {
						if(next_index >= next_treshold) {
							break;
						}
					} else {
						if(next_index < prev_treshold || next_index >= next_treshold) {
							break;
						}
					}
					if constexpr(Operation) {
						*valid = *current;
						++valid;
					}
					++current;
				}
				continue;
			}
			auto& chunk = *chunkPtr;

			for(;;) {
				if(current == last) {
					return valid;
				}
				auto const next_index = convert_from_user_type(*current);
				if constexpr(PreSorted) {
					if(next_index >= next_treshold) {
						break;
					}
				} else {
					if(next_index < prev_treshold || next_index >= next_treshold) {
						break;
					}
				}

				auto const next_element_index = next_index - static_cast<index_type>(prev_treshold);
				auto const val				  = chunk[next_element_index];
				auto const stage			  = stage_range_for_index(val);
				if constexpr(Operation) {
					if(val == TOMBSTONE || ((stage & range) != stage)) {
						*valid = *current;
						++valid;
					}
				} else {
					if(val != TOMBSTONE && ((stage & range) == stage)) {
						*valid = *current;
						++valid;
					}
				}
				++current;
			}
		} while(current != last);

		return valid;
	}

	constexpr FORCEINLINE auto operator[](user_index_type index) -> value_type&
		requires(IS_COMPLEX)
	{
		auto element_index = convert_from_user_type(index);
		auto chunk_index   = userspace_to_internal(element_index);
		if(element_index == TOMBSTONE) {
			insert(&index, &index + 1);
			// we know the index must be the last in the added stage start (meaning removed stage - 1) as it has just
			// been inserted
			return dense_storage_type::operator[](m_StageStart[2] - 1);
		}
		auto internal_index = m_Sparse[chunk_index]->at(element_index);
		return dense_storage_type::operator[](internal_index);
	}

	constexpr FORCEINLINE auto operator[](user_index_type index) const -> value_type const&
		requires(IS_COMPLEX)
	{
		auto internal_index = userspace_to_internal(convert_from_user_type(index));
		if(internal_index == TOMBSTONE || !m_Sparse[internal_index]) {
			throw std::out_of_range("Index does not exist in staged_sparse_array");
		}
		return dense_storage_type::operator[]((*m_Sparse[internal_index])[convert_from_user_type(index)]);
	}

	template <typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = std::nullptr_t,
			  typename DataItLast  = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto insert(IndexItFirst it_index_first,
									  IndexItLast it_index_last,
									  DataItFirst it_data_first = nullptr,
									  DataItLast it_data_last	= nullptr) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		auto index_span = psl::impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= psl::impl::to_span_wrapper(it_data_first, it_data_last);
		auto lock		= m_Guard.scoped_guard();
		return insert_impl<insertion_mode::insert>(index_span, data_span);
	}


	template <typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = std::nullptr_t,
			  typename DataItLast  = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto try_insert(IndexItFirst it_index_first,
										  IndexItLast it_index_last,
										  DataItFirst it_data_first = nullptr,
										  DataItLast it_data_last	= nullptr) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		auto index_span = psl::impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= psl::impl::to_span_wrapper(it_data_first, it_data_last);
		auto lock		= m_Guard.scoped_guard();
		return insert_impl<insertion_mode::try_insert>(index_span, data_span);
	}

	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst, typename DataItLast = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto set(IndexItFirst it_index_first,
								   IndexItLast it_index_last,
								   DataItFirst it_data_first,
								   DataItLast it_data_last = nullptr) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		auto lock		= m_Guard.scoped_guard();
		auto index_span = psl::impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= psl::impl::to_span_wrapper(it_data_first, it_data_last);
		return insert_impl<insertion_mode::set>(index_span, data_span);
	}

	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst, typename DataItLast = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto assign(IndexItFirst it_index_first,
									  IndexItLast it_index_last,
									  DataItFirst it_data_first,
									  DataItLast it_data_last = nullptr) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		auto index_span = psl::impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= psl::impl::to_span_wrapper(it_data_first, it_data_last);
		return insert_impl<insertion_mode::assign>(index_span, data_span);
	}

	/// \brief Erases the values at the provided indices from the sparse array.
	/// \tparam IndexItFirst The type of the first iterator for the indices.
	/// \tparam IndexItLast The type of the last iterator for the indices.
	///
	/// \details This function will erase the provided indices from the sparse array.
	/// All indices are expected to be valid and present in the sparse array, otherwise
	/// it is considered a user error and will assert in debug builds. Use try_erase to skip
	/// indices that are not present.
	///
	/// \return The number of indices that were actually erased.
	template <typename IndexItFirst, typename IndexItLast>
		requires(impl::IsIteratorLikeType<IndexItFirst, user_index_type> &&
				 impl::IsIteratorLikeType<IndexItLast, user_index_type>)
	constexpr FORCEINLINE auto erase(IndexItFirst begin, IndexItLast end) -> index_type {
		if(begin == end) {
			return index_type {0};
		}
		auto index_span =
		  psl::impl::to_span_wrapper(std::make_reverse_iterator(end), std::make_reverse_iterator(begin));
		auto lock = m_Guard.scoped_guard();
		return erase_impl<false>(index_span);
	}

	/// \brief Erases the value at the provided index from the sparse array.
	/// \see erase
	constexpr FORCEINLINE auto erase(user_index_type index) -> bool {
		return erase(&index, &index + 1) != 0;
	}

	/// \brief Attempts to erase the values at the provided indices from the sparse array.
	/// \tparam IndexItFirst The type of the first iterator for the indices.
	/// \tparam IndexItLast The type of the last iterator for the indices.
	///
	/// \details This function will attempt to erase the provided indices from the sparse array.
	/// If an index is not present in the sparse array, it will be skipped instead of being considered an error.
	///
	/// \return The number of indices that were actually erased.
	template <typename IndexItFirst, typename IndexItLast>
		requires(impl::IsIteratorLikeType<IndexItFirst, user_index_type> &&
				 impl::IsIteratorLikeType<IndexItLast, user_index_type>)
	constexpr FORCEINLINE auto try_erase(IndexItFirst begin, IndexItLast end) -> index_type {
		if(begin == end) {
			return index_type {0};
		}
		auto index_span =
		  psl::impl::to_span_wrapper(std::make_reverse_iterator(end), std::make_reverse_iterator(begin));
		auto lock = m_Guard.scoped_guard();
		return erase_impl<true>(index_span);
	}

	/// \brief Attempts to erase the value at the provided index from the sparse array.
	/// \see try_erase
	constexpr FORCEINLINE auto try_erase(user_index_type index) -> bool {
		return try_erase(&index, &index + 1) != 0;
	}

	/// \brief Promotes all values to the next `stage_t`. The cycle is as follows: ADDED -> SETTLED -> REMOVED -> deleted.
	constexpr FORCEINLINE auto promote() noexcept -> void {
		std::span<index_type> reverse_view(std::next(m_Reverse.begin(), m_StageStart[2]), m_Reverse.end());

		if(!reverse_view.empty()) {
			auto index_span = psl::impl::to_span_wrapper(reverse_view.begin(), reverse_view.end());
			invoke_for_l0<false, false, index_type>(
			  index_span, [this](index_type index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset) {
				  psl_assert(chunk[chunk_offset] != TOMBSTONE);
				  chunk[chunk_offset] = TOMBSTONE;
			  });

			dense_storage_type::truncate(m_StageStart[to_underlying(stage_t::REMOVED)]);
			m_Reverse.erase(std::next(std::begin(m_Reverse), m_StageStart[to_underlying(stage_t::REMOVED)]),
							std::end(m_Reverse));
		}

		m_StageSize[to_underlying(stage_t::SETTLED)] += m_StageSize[to_underlying(stage_t::ADDED)];
		m_StageStart[to_underlying(stage_t::ADDED)]	 = m_StageSize[to_underlying(stage_t::SETTLED)];
		m_StageStart[3]								 = m_StageStart[to_underlying(stage_t::REMOVED)];
		m_StageSize[to_underlying(stage_t::ADDED)]	 = 0;
		m_StageSize[to_underlying(stage_t::REMOVED)] = 0;
	}

	constexpr FORCEINLINE void clear(bool release_memory = false) noexcept {
		m_Reverse.clear();
		m_Sparse.clear();
		m_StageStart = {0, 0, 0, 0};
		m_StageSize	 = {0, 0, 0};
		dense_storage_type::clear(release_memory);
	}

	constexpr FORCEINLINE auto begin() noexcept -> T* requires(IS_COMPLEX) { return dense_storage_type::begin(); }

	constexpr FORCEINLINE auto end() noexcept -> T* requires(IS_COMPLEX) { return dense_storage_type::end(); }

	constexpr FORCEINLINE
	  auto begin() const noexcept -> T const* requires(IS_COMPLEX) { return dense_storage_type::begin(); }

	constexpr FORCEINLINE
	  auto end() const noexcept -> T const* requires(IS_COMPLEX) { return dense_storage_type::end(); }

	struct merge_result {
		bool success {false};
		size_t added {0};
	};

	/// \brief Merges 2 staged_sparse_memory_region_t together (into the current one) replacing pre-existing entries.
	/// \param other the staged_sparse_memory_region_t to merge into this one
	/// \returns object that contains { .success /* bool */, .total /* total items processed, inserted + replaced */, .added /* amount that was inserted into the current container */ }
	FORCEINLINE auto merge(const this_type& other) noexcept -> merge_result {
		if(!dense_storage_type::can_merge(&other) || this == &other) {
			return merge_result {false};
		}
		size_t added {0};
		if(other.m_Reverse.size() > 0) {
			auto index_span = psl::impl::to_span_wrapper(other.m_Reverse.begin(), other.m_Reverse.end());
			if constexpr(IS_FLAG) {
				added = insert_impl<insertion_mode::try_insert, index_type>(index_span, nullptr);
			} else {
				auto data_span =
				  psl::impl::to_span_wrapper(other.dense_storage_type::begin(), other.dense_storage_type::end());
				added += insert_impl<insertion_mode::set, index_type>(index_span, data_span);
			}
		}

		if(other.m_StageSize[2] > 0) {
			auto index_span = psl::impl::to_span_wrapper(std::next(other.m_Reverse.begin(), other.m_StageStart[2]),
														 std::next(other.m_Reverse.begin(), other.m_Reverse.size()));
			erase_impl<true, index_type>(index_span);
		}

		return merge_result {.success = true, .added = added};
	}

	template <typename InvocableFound,
			  typename InvocableNotFound,
			  typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = std::nullptr_t,
			  typename DataItLast  = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	FORCEINLINE auto for_each(stage_range_t range,
							  InvocableFound&& InvocableOnFound,
							  InvocableNotFound&& InvocableOnNotFound,
							  IndexItFirst first,
							  IndexItLast last,
							  DataItFirst data_first = nullptr,
							  DataItLast data_last	 = nullptr)
		requires(impl::IsForEachFoundInvocable<InvocableFound, index_type, pointer, DataItFirst> &&
				 impl::IsForEachNotFoundInvocable<InvocableNotFound, index_type> &&
				 (std::input_or_output_iterator<IndexItFirst> || std::is_pointer_v<IndexItFirst>))
	{
		if(first == last) {
			return;
		}
		auto index_span = psl::impl::to_span_wrapper(first, last);
		auto data_span	= psl::impl::to_span_wrapper(data_first, data_last);
		invoke_for_l0<false, false>(
		  index_span,
		  [&InvocableOnFound,
		   &InvocableOnNotFound,
		   range_begin = psl::narrow_cast<index_type>(stage_begin(range)),
		   range_end   = psl::narrow_cast<index_type>(stage_end(range))](
			index_type index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset, auto... dataIt) {
			  if(chunk[chunk_offset] < range_begin || chunk[chunk_offset] >= range_end) {
				  InvocableOnNotFound(index);
			  }
			  if constexpr(IS_FLAG) {
				  InvocableOnFound(index);
			  } else {
				  InvocableOnFound(index, (pointer)dense_storage_type::unsafe_data(chunk[chunk_offset]), &*dataIt...);
			  }
		  },
		  data_span,
		  InvocableOnNotFound);
	}


	template <typename InvocableFound,
			  typename InvocableNotFound,
			  typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = std::nullptr_t,
			  typename DataItLast  = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	FORCEINLINE auto for_each(InvocableFound&& InvocableOnFound,
							  InvocableNotFound&& InvocableOnNotFound,
							  IndexItFirst first,
							  IndexItLast last,
							  DataItFirst data_first = nullptr,
							  DataItLast data_last	 = nullptr)
		requires(impl::IsForEachFoundInvocable<InvocableFound, index_type, pointer, DataItFirst> &&
				 impl::IsForEachNotFoundInvocable<InvocableNotFound, index_type> &&
				 (std::input_or_output_iterator<IndexItFirst> || std::is_pointer_v<IndexItFirst>))
	{
		if(first == last) {
			return;
		}
		auto index_span = psl::impl::to_span_wrapper(first, last);
		auto data_span	= psl::impl::to_span_wrapper(data_first, data_last);
		invoke_for_l0<false, false>(
		  index_span,
		  [&InvocableOnFound](
			index_type index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset, auto... dataIt) {
			  if constexpr(IS_FLAG) {
				  InvocableOnFound(index);
			  } else {
				  InvocableOnFound(index, (pointer)dense_storage_type::unsafe_data(chunk[chunk_offset]), &*dataIt...);
			  }
		  },
		  data_span,
		  InvocableOnNotFound);
	}

	template <typename Invocable,
			  typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = std::nullptr_t,
			  typename DataItLast  = void*>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	FORCEINLINE auto for_each(Invocable&& InvocableOnFound,
							  IndexItFirst first,
							  IndexItLast last,
							  DataItFirst data_first = nullptr,
							  DataItLast data_last	 = nullptr)
		requires(impl::IsForEachFoundInvocable<Invocable, index_type, pointer, DataItFirst> &&
				 (std::input_or_output_iterator<IndexItFirst> || std::is_pointer_v<IndexItFirst>))
	{
		if(first == last) {
			return;
		}
		auto index_span = psl::impl::to_span_wrapper(first, last);
		auto data_span	= psl::impl::to_span_wrapper(data_first, data_last);
		invoke_for_l0<false, false>(
		  index_span,
		  [&InvocableOnFound](
			index_type index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset, auto... dataIt) {
			  if constexpr(IS_FLAG) {
				  InvocableOnFound(index);
			  } else {
				  InvocableOnFound(index, (pointer)dense_storage_type::unsafe_data(chunk[chunk_offset]), &*dataIt...);
			  }
		  },
		  data_span);
	}

	template <typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = std::nullptr_t,
			  typename DataItLast  = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	FORCEINLINE auto for_each_guarantee(auto&& Invocable,
										IndexItFirst first,
										IndexItLast last,
										DataItFirst data_first = nullptr,
										DataItLast data_last   = nullptr) -> index_type {
		if(first == last) {
			return 0;
		}
		auto index_span = psl::impl::to_span_wrapper(first, last);
		auto data_span	= psl::impl::to_span_wrapper(data_first, data_last);

		auto const size = psl::narrow_cast<index_type>(m_Reverse.size());
		dense_storage_type::reserve(size + psl::narrow_cast<index_type>(index_span.size()));
		invoke_for_l0<true, false>(
		  index_span,
		  [&Invocable,
		   this](index_type index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset, auto dataIt) {
			  index_type rev_index {chunk[chunk_offset]};
			  if(rev_index == TOMBSTONE) {
				  chunk[index] = psl::narrow_cast<index_type>(m_Reverse.size()) - m_StageSize[2];
				  rev_index	   = psl::narrow_cast<index_type>(m_Reverse.size());
				  m_Reverse.emplace_back(index);
				  dense_storage_type::emplace_back();
			  }

			  if constexpr(std::is_same_v<DataItFirst, std::nullptr_t>) {
				  Invocable(index, (pointer)dense_storage_type::unsafe_data(rev_index));
			  } else {
				  Invocable(index, (pointer)dense_storage_type::unsafe_data(rev_index), &*dataIt);
			  }
		  },
		  data_span);

		auto count = psl::narrow_cast<index_type>(m_Reverse.size()) - size;
		if(count > 0) {
			for(auto it = m_Reverse.begin() + m_StageStart[2], end = m_Reverse.begin() + m_StageStart[3]; it != end;
				++it) {
				index_type element_index, chunk_index;
				chunk_info_for(*it, element_index, chunk_index);
				(*m_Sparse[chunk_index])[element_index] += count;
			}
			std::rotate(m_Reverse.begin() + m_StageStart[2],
						m_Reverse.begin() + m_StageStart[3],
						m_Reverse.begin() + m_StageStart[3] + count);


			dense_storage_type::rotate(m_StageStart[2], m_StageStart[3], m_StageStart[3] + count);

			m_StageStart[2] += count;
			m_StageStart[3] += count;
			m_StageSize[1] += count;
		}

		return count;
	}

	template <typename Fn>
	FORCEINLINE auto remap(const psl::sparse_array<index_type, index_type>& mapping, Fn&& predicate) -> void {
		psl_assert(m_Reverse.size() >= mapping.size(), "expected {} >= {}", m_Reverse.size(), mapping.size());
		m_Sparse.clear();
		for(index_type i = 0, count = psl::narrow_cast<index_type>(m_Reverse.size()); i < count; ++i) {
			if(predicate(convert_to_user_type(m_Reverse[i]))) {
				psl_assert(mapping.contains(m_Reverse[i]), "mapping didnt have the ID {}", m_Reverse[i]);
				auto new_index = mapping.at(m_Reverse[i]);
				auto offset	   = new_index;
				auto& chunk	   = chunk_for_guarantee(offset);
				chunk[offset]  = i;
				m_Reverse[i]   = new_index;
			} else {
				auto offset	  = m_Reverse[i];
				auto& chunk	  = chunk_for_guarantee(offset);
				chunk[offset] = i;
			}
		}
	}

  private:
	enum class insertion_mode {
		insert,
		try_insert,
		set,
		assign,
	};


	constexpr FORCEINLINE auto
	chunk_info_for(index_type index, index_type& element_index, index_type& chunk_index) const noexcept -> void {
		if constexpr(IS_CHUNKS_POW_2) {
			element_index = index & MOD_VAL;
			chunk_index	  = (index - element_index) / CHUNKS_SIZE;
		} else {
			element_index = index % MOD_VAL;
			chunk_index	  = (index - (index % MOD_VAL)) / CHUNKS_SIZE;
		}
	}

	/// \brief Converts a user-space index to an internal index, returning the chunk index, and modifying the index
	/// parameter to be the element index within the chunk.
	constexpr FORCEINLINE auto userspace_to_internal(index_type& index) const noexcept -> index_type {
		index_type chunk_index;
		index_type element_index;
		chunk_info_for(index, element_index, chunk_index);
		if(chunk_index >= m_Sparse.size() || !m_Sparse[chunk_index]) {
			return TOMBSTONE;
		}
		auto& chunk = *m_Sparse[chunk_index];
		/*if(element_index >= chunk.size()) {
			return TOMBSTONE;
		}*/
		index = element_index;
		return chunk_index;
	}

	constexpr FORCEINLINE auto userspace_to_internal(index_type& index) noexcept -> index_type {
		index_type chunk_index;
		index_type element_index;
		chunk_info_for(index, element_index, chunk_index);
		if(m_Sparse.size() <= chunk_index) {
			m_Sparse.resize(chunk_index + 1);
		}
		auto& chunkPtr = m_Sparse[chunk_index];
		if(!chunkPtr) {
			chunkPtr = std::make_unique<chunk_type>(CHUNKS_SIZE, TOMBSTONE);
		}
		index = element_index;
		return chunk_index;
	}

	/// \brief Ensures that the sparse array has enough space for the given index.
	constexpr FORCEINLINE void sparse_guarantee_for_userspace(index_type index) {
		index_type chunk_index;
		if constexpr(IS_CHUNKS_POW_2) {
			chunk_index = (index - (index & MOD_VAL)) / CHUNKS_SIZE;
		} else {
			chunk_index = (index - (index % MOD_VAL)) / CHUNKS_SIZE;
		}
		if(m_Sparse.size() <= chunk_index) {
			m_Sparse.resize(chunk_index + 1);
		}
	}

	constexpr FORCEINLINE auto stage_for_index(index_type index) const noexcept -> stage_t {
		return (index < m_StageStart[1])   ? stage_t::SETTLED
			   : (index < m_StageStart[2]) ? stage_t::ADDED
										   : stage_t::REMOVED;
	}

	constexpr FORCEINLINE auto stage_range_for_index(index_type index) const noexcept -> stage_range_t {
		return (index < m_StageStart[1])   ? stage_range_t::SETTLED
			   : (index < m_StageStart[2]) ? stage_range_t::ADDED
										   : stage_range_t::REMOVED;
	}

	constexpr FORCEINLINE auto chunk_for(index_type& index) const noexcept -> chunk_type& {
		auto chunk_index = userspace_to_internal(index);
		psl_assert(chunk_index != TOMBSTONE, "chunk for user index {} does not exist in staged_sparse_array", index);
		return *m_Sparse[chunk_index];
	}

	constexpr FORCEINLINE auto chunk_for_guarantee(index_type& index) noexcept -> chunk_type& {
		auto chunk_index = userspace_to_internal(index);
		psl_assert(chunk_index != TOMBSTONE, "chunk for user index {} does not exist in staged_sparse_array", index);
		return *m_Sparse[chunk_index].get();
	}

	/// \brief Entrypoint for all operations that modify the sparse array's underlying data, or add new indices.
	/// \tparam InsertMode The mode of insertion to perform. Can be insert, try_insert, set, or assign.
	/// \param index_span A span wrapper containing the indices to operate on.
	/// \param data_span A span wrapper containing the data to operate on. Can be nullptr if no data is to be used.
	/// \return The number of indices that were actually modified/inserted. Depending on the operation, this may be less than the number of indices provided.
	template <insertion_mode InsertMode, typename local_index_type = user_index_type>
	constexpr FORCEINLINE auto insert_impl(auto&& index_span, auto&& data_span) -> index_type {
		// this function behaves a bit different depending on the mode (normal or try-insert), and if data is provided.
		// insert mode /w data:
		// - does all the normal insertions as if no data is present
		// - at the end will make space and insert all the data at once
		// - afterwards it will rotate the dense data and swap it with the ones in the removed state.
		// insert mode /w no data:
		// - normal insertion operations for all indices
		// - afterwards it will inject space for the new indices in the dense storage before the removed indices stage
		// try-insert mode /w data:
		// - reserves all the required size for the data upfront in the dense storage
		// - during insertions it will emplace the data at the back of the dense storage
		// try-insert mode /w no data:
		// - behaves exactly like the normal mode /w no data.
		// set mode /w data:
		// - when the element is already present it will overwrite the data in the dense storage
		// - otherwise it will insert the data at the end of the dense storage (and rotate at the end)
		// set mode /w no data:
		// - compile error
		// assign mode /w data:
		// - will overwrite the data in the dense storage or assert when the element is not present
		// assign mode /w no data:
		// - compile error
		const auto size = psl::narrow_cast<index_type>(index_span.size());

		if constexpr(!std::is_same_v<std::remove_cvref_t<decltype(data_span)>, std::nullptr_t>) {
			psl_assert(data_span.size() == size || data_span.size() == 1,
					   "Data span size must match index span size (or be 1), got for data_span {} and expected {} or 1",
					   data_span.size(),
					   size);
		}

		// assign is thread safe as it only modifies existing elements
		if constexpr(InsertMode != insertion_mode::assign) {
			m_Reverse.reserve(m_StageStart[3] + size);
			dense_storage_type::reserve(m_StageStart[3] + size);
		}
		index_type count = 0;

		invoke_for_l0<true, false, local_index_type>(
		  index_span,
		  [&, this](
			index_type index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset, auto&&... dataIt) {
			  static_assert(sizeof...(dataIt) <= 1, "dataIt can only be empty, or contain a single element");
			  index_type rev_index {chunk[chunk_offset]};
			  // try_insert requires the element to be empty
			  if constexpr(InsertMode == insertion_mode::try_insert) {
				  if(rev_index != TOMBSTONE) {
					  return;
				  }
			  }
			  // if we're inserting or assigning, we need to ensure that the chunk is empty
			  else if constexpr(InsertMode == insertion_mode::insert) {
				  psl_assert(rev_index == TOMBSTONE);
			  }
			  // both insert and try_insert always add to the end
			  if constexpr(InsertMode == insertion_mode::try_insert || InsertMode == insertion_mode::insert) {
				  rev_index = m_StageStart[2] + count;
				  m_Reverse.emplace_back(index);
				  if constexpr(InsertMode == insertion_mode::try_insert && sizeof...(dataIt) > 0) {
					  dense_storage_type::emplace_back(&*dataIt...);
				  }
				  chunk[chunk_offset] = rev_index;
				  ++count;
			  }
			  // set might add a new element, or it will overwrite an existing one
			  else if constexpr(InsertMode == insertion_mode::set) {
				  if(rev_index != TOMBSTONE) {
					  if constexpr(sizeof...(dataIt) == 0) {
						  psl_assert(false, "set without data makes no sense, use (try-)insert");
					  } else {
						  dense_storage_type::set(rev_index, &*dataIt...);
					  }
				  } else {
					  rev_index = m_StageStart[2] + count;
					  m_Reverse.emplace_back(index);
					  if constexpr(sizeof...(dataIt) == 0) {
						  psl_assert(false, "set without data makes no sense, use (try-)insert");
					  } else {
						  dense_storage_type::emplace_back(&*dataIt...);
					  }
					  chunk[chunk_offset] = rev_index;
					  ++count;
				  }
			  }
			  // assign will always overwrite an existing element, and skip missing ones
			  else {
				  if(rev_index != TOMBSTONE) {
					  if constexpr(sizeof...(dataIt) == 0) {
						  psl_assert(false, "assign without data makes no sense, use (try-)insert");
					  } else {
						  dense_storage_type::set(rev_index, &*dataIt...);
						  ++count;	  // count successful assignments
					  }
				  }
			  }
		  },
		  data_span);

		// we silence the next scope if this is an assign as we have already completed our work
		if constexpr(InsertMode == insertion_mode::assign) {
		} else if(count > 0) {
			if constexpr(IS_ASSIGNABLE && InsertMode == insertion_mode::try_insert &&
						 std::is_same_v<std::remove_cvref_t<decltype(data_span)>, std::nullptr_t>) {
				dense_storage_type::insert_space(m_StageStart[2], count);
			}

			if(m_StageSize[2] > 0) {
				for(auto it = m_Reverse.begin() + m_StageStart[2], end = m_Reverse.begin() + m_StageStart[3]; it != end;
					++it) {
					index_type element_index, chunk_index;
					chunk_info_for(*it, element_index, chunk_index);
					(*m_Sparse[chunk_index])[element_index] += count;
				}
				std::rotate(m_Reverse.begin() + m_StageStart[2],
							m_Reverse.begin() + m_StageStart[3],
							m_Reverse.begin() + m_StageStart[3] + count);

				// both try_insert and set can add new data elements at the end of the dense storage, so we'll rotate
				// those
				if constexpr(IS_ASSIGNABLE &&
							 (InsertMode == insertion_mode::try_insert || InsertMode == insertion_mode::set) &&
							 !std::is_same_v<std::remove_cvref_t<decltype(data_span)>, std::nullptr_t>) {
					dense_storage_type::rotate(m_StageStart[2], m_StageStart[3], m_StageStart[3] + count);
				}
			}

			// when inserting we only need to reserve space and insert at the end as we can be certain these
			// are all new elements.
			if constexpr(IS_ASSIGNABLE && InsertMode == insertion_mode::insert) {
				if constexpr(!std::is_same_v<std::remove_cvref_t<decltype(data_span)>, std::nullptr_t>) {
					for(size_t i = 0; i < count; ++i) {
						dense_storage_type::emplace_back(&*data_span.current());
						data_span.next();
					}
					dense_storage_type::rotate(m_StageStart[2], m_StageStart[3], m_StageStart[3] + count);
				} else {
					dense_storage_type::insert_space(m_StageStart[2], count);
				}
			}

			m_StageStart[2] += count;
			m_StageStart[3] += count;
			m_StageSize[1] += count;
		}
		return count;
	}

	/// \brief Entrypoint for all erase operations on the sparse array.
	/// \tparam TryErase If true, will skip indices that are not present in the sparse array. If false, will assert if an index is not present.
	/// \param index_span A span wrapper containing the indices to erase.
	/// \return The number of indices that were actually erased.
	template <bool TryErase, typename local_index_type = user_index_type>
	constexpr FORCEINLINE auto erase_impl(auto&& range) -> index_type {
		if constexpr(!TryErase) {
			psl_assert(!std::empty(m_Reverse), "cannot erase from an empty staged_sparse_array");
		}

		auto const original_deleted = m_StageSize[2];

		invoke_for_l0<false, TryErase, local_index_type, std::greater<local_index_type>>(
		  range, [this](index_type user_index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset) {
			  auto reverse_index = chunk[chunk_offset];
			  psl_assert(reverse_index != TOMBSTONE);
			  auto const what_stage = (reverse_index < m_StageStart[1]) ? 0 : (reverse_index < m_StageStart[2]) ? 1 : 2;
			  // It's already set to be removed
			  if(what_stage == 2) {
				  return;
			  }

			  for(auto i = what_stage; i < 2; ++i) {
				  // last index in the current stage
				  auto const original_index = m_StageStart[i + 1] - 1;
				  auto const original_value = m_Reverse[original_index];

				  // if the original index is the same as the reverse index, we don't need to do anything
				  if(original_index != reverse_index) {
					  if(original_value >= chunk_index * CHUNKS_SIZE &&
						 original_value < (chunk_index + 1) * CHUNKS_SIZE) {
						  chunk[original_value - (chunk_index * CHUNKS_SIZE)] = reverse_index;
					  } else {
						  auto original_sparse_offset = original_value;
						  auto original_sparse		  = userspace_to_internal(original_sparse_offset);
						  (*m_Sparse[original_sparse])[original_sparse_offset] = reverse_index;
					  }

					  dense_storage_type::swap(reverse_index, original_index);
					  std::iter_swap(std::next(std::begin(m_Reverse), reverse_index),
									 std::next(std::begin(m_Reverse), original_index));

					  chunk[chunk_offset] = original_index;
					  reverse_index		  = original_index;
				  }

				  m_StageSize[i] -= 1;
				  m_StageStart[i + 1] -= 1;
				  m_StageSize[i + 1] += 1;
			  }
		  });

		return m_StageSize[2] - original_deleted;
	}

	/// \brief Helper intermediate that will invoke all indices in the provided span, creating chunks as needed if AutoCreate is true.
	/// It will determine if the span is sorted or not, and call the appropriate invoke_for_l1 function.
	/// \tparam AutoCreate If true, will create chunks as needed. If false, will assert if a chunk is missing.
	/// \tparam Cmp The comparator to use for determining if the span is sorted. Defaults to std::less.
	/// \tparam DataSpan The type of the data span wrapper. Can be nullptr if no data is to be used.
	template <bool AutoCreate,
			  bool SkipMissing,
			  typename local_index_type = user_index_type,
			  typename Cmp				= std::less<local_index_type>,
			  typename DataSpan			= std::nullptr_t,
			  typename CbNotFound		= std::nullptr_t>
	constexpr FORCEINLINE auto invoke_for_l0(this auto&& self,
											 auto index_span,
											 auto&& CallbackFound,
											 DataSpan data_span			   = nullptr,
											 CbNotFound&& CallbackNotFound = nullptr) {
		constexpr auto IsConst = std::is_const_v<std::remove_reference_t<decltype(self)>>;
		static_assert((!AutoCreate && IsConst) || !IsConst, "Cannot mutate when this is a const invoke");
		if(index_span.size() == 0) {
			return;
		}
		if(std::is_sorted(index_span.begin(), index_span.end(), Cmp {})) {
			if constexpr(AutoCreate) {
				self.sparse_guarantee_for_userspace(static_cast<index_type>(*(std::prev(index_span.end()))));
			}
			self.template invoke_for_l1<AutoCreate,
										true,
										std::is_same_v<Cmp, std::greater<local_index_type>>,
										SkipMissing>(index_span, CallbackFound, data_span, CallbackNotFound);
		} else {
			if constexpr(AutoCreate) {
				self.sparse_guarantee_for_userspace(
				  static_cast<index_type>(*std::max_element(index_span.begin(), index_span.end())));
			}
			self.template invoke_for_l1<AutoCreate,
										false,
										std::is_same_v<Cmp, std::greater<local_index_type>>,
										SkipMissing>(index_span, CallbackFound, data_span, CallbackNotFound);
		}
	}

	/// \brief Core implementation that will invoke all indices in the provided span, creating chunks as needed if AutoCreate is true.
	/// \tparam AutoCreate If true, will create chunks as needed. If false,
	/// will assert if a chunk is missing.
	/// \tparam PreSorted If true, will assume the indices are sorted in ascending order.
	///
	template <bool AutoCreate,
			  bool PreSorted,
			  bool IsReverse	  = false,
			  bool SkipMissing	  = false,
			  typename DataSpan	  = std::nullptr_t,
			  typename CbNotFound = std::nullptr_t>
	constexpr FORCEINLINE auto invoke_for_l1(this auto&& self,
											 auto&& index_span,
											 auto&& CallbackFound,
											 DataSpan&& data_span		   = nullptr,
											 CbNotFound&& CallbackNotFound = nullptr) {
		constexpr auto HasNotFoundCb = !std::is_same_v<std::remove_cvref_t<CbNotFound>, std::nullptr_t>;
		constexpr auto HasDataSpan	 = !std::is_same_v<std::remove_cvref_t<DataSpan>, std::nullptr_t>;
		constexpr auto IsConst		 = std::is_const_v<std::remove_reference_t<decltype(self)>>;

		static_assert((!AutoCreate && IsConst) || !IsConst, "Cannot mutate when this is a const invoke");

		do {
			auto const first_index = static_cast<index_type>(*index_span.current());
			index_type chunk_index {};
			index_type element_index {};
			self.chunk_info_for(first_index, element_index, chunk_index);
			auto const prev_treshold {chunk_index * CHUNKS_SIZE};
			auto const next_treshold {prev_treshold + CHUNKS_SIZE};

			if constexpr(SkipMissing || HasNotFoundCb) {
				if(chunk_index >= self.m_Sparse.size() || !self.m_Sparse[chunk_index]) {
					for(;;) {
						if(!index_span.has_next()) {
							return;
						}

						auto const next_index = static_cast<index_type>(*index_span.current());

						if constexpr(PreSorted && IsReverse) {
							if(chunk_index < self.m_Sparse.size() && next_index < prev_treshold) {
								break;
							}
						} else if constexpr(PreSorted) {
							if(chunk_index < self.m_Sparse.size() && next_index >= next_treshold) {
								break;
							}
						} else {
							if(next_index >= next_treshold || next_index < prev_treshold) {
								break;
							}
						}

						if constexpr(HasNotFoundCb) {
							CallbackNotFound(next_index);
						}
						index_span.next();
						if constexpr(HasDataSpan) {
							data_span.next();
						}
					}
					continue;
				}
			}

			psl_assert(chunk_index < self.m_Sparse.size(), "Chunk index out of bounds");

			auto& chunkPtr = self.m_Sparse[chunk_index];
			if constexpr(AutoCreate) {
				if(!chunkPtr) {
					chunkPtr = std::make_unique<chunk_type>(CHUNKS_SIZE, TOMBSTONE);
				}
			} else {
				psl_assert(chunkPtr, "Chunk pointer cannot be null");
			}

			auto& chunk = *chunkPtr;
			for(;;) {
				if(!index_span.has_next()) {
					break;
				}
				auto const next_index = static_cast<index_type>(*index_span.current());
				if constexpr(PreSorted) {
					if constexpr(IsReverse) {
						if(next_index < prev_treshold) {
							break;
						}
					} else {
						if(next_index >= next_treshold) {
							break;
						}
					}
				} else {
					if(next_index >= next_treshold || next_index < prev_treshold) {
						break;
					}
				}

				index_type const next_element_index = next_index - prev_treshold;
				if constexpr(SkipMissing || HasNotFoundCb) {
					if(chunk[next_element_index] == TOMBSTONE) {
						if constexpr(HasNotFoundCb) {
							CallbackNotFound(next_index);
						}
						index_span.next();

						if constexpr(HasDataSpan) {
							data_span.next();
						}
						continue;
					}
				}

				if constexpr(!HasDataSpan) {
					CallbackFound(next_index, chunk, chunk_index, next_element_index);
				} else {
					auto data_it = data_span.current();
					CallbackFound(next_index, chunk, chunk_index, next_element_index, data_it);
					data_span.next();
				}
				index_span.next();
			}
		} while(index_span.has_next());
	}

	psl::array<index_type> m_Reverse {};
	chunk_storage_type m_Sparse {};

	psl::static_array<index_type, 4> m_StageStart {0, 0, 0, 0};
	psl::static_array<index_type, 3> m_StageSize {0, 0, 0};

	psl::dbg_thread_safety_guard_t m_Guard {};
};

}	 // namespace psl::ecs::details
