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
					m_Begin[index] = *value;
				}
			}
		}

		void swap(Key first, Key second) {
			psl_assert(first < size() && second < size(), "index out of bounds");
			if constexpr(std::is_trivially_copyable_v<T>) {
				std::swap_ranges(m_Begin + first, m_Begin + first + 1, m_Begin + second);
			} else {
				std::swap(m_Begin[first], m_Begin[second]);
			}
		}

		void emplace_back() {
			psl_assert(size() < capacity(), "no more space left to create new elements");
			if constexpr(std::is_trivially_constructible_v<T>) {
				// no-op for trivial types
			} else {
				new(m_End) T();
			}
			++m_End;
		}

		template <typename U>
		void emplace_back(U&& value) {
			psl_assert(size() < capacity(), "no more space left to create new elements");
			new(m_End) T(*value);
			++m_End;
		}

		void reserve(Key new_size) {
			if(new_size > capacity()) {
				auto const old_size = m_End - m_Begin;

				::memory::raw_region new_dense(new_size * sizeof(T));
				if constexpr(std::is_trivially_copyable_v<T>) {
					std::memcpy(new_dense.data(), m_Dense.data(), old_size);
				} else {
					auto currentPtr		= m_Begin;
					auto newTargetPtr	= (T*)new_dense.data();
					auto new_dense_size = new_dense.size();
					newTargetPtr		= (T*)std::align(alignof(T), sizeof(T), (void*&)newTargetPtr, new_dense_size);
					psl_assert(new_dense_size >= (m_End - m_Begin) * sizeof(T),
							   "new dense size must be greater than or equal to current size");
					for(size_t i = 0; i < size(); ++i) {
						new(newTargetPtr) T(std::move(*currentPtr));
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
					new(targetPtr) T(std::move(*currentPtr));
				}
			}
			m_End += count;
		}

		void truncate(Key new_size) {
			psl_assert(new_size <= size(), "new size must be less than or equal to current size");
			auto const old_size = size();
			for(size_t i = new_size; i < old_size; ++i) {
				m_End->~T();
				--m_End;
			}
			std::memset(m_End, 0, (old_size - new_size) * sizeof(T));
		}

		void rotate(Key begin, Key middle, Key last) {
			psl_assert(begin < size() && middle < size() && last <= size(), "index out of bounds");
			std::rotate(m_Begin + begin, m_Begin + middle, m_Begin + last);
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
			std::rotate(m_Begin + begin * m_TypeSize, m_Begin + middle * m_TypeSize, m_Begin + last * m_TypeSize);
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
		cinvoke_for(
		  it_index_first,
		  it_index_last,
		  [](auto index, chunk_type& chunk, index_type chunk_offset, ItDataFirst dataIt) {
			  psl_assert(chunk[chunk_offset] != TOMBSTONE, "expected valid index in sparse array");
			  *dataIt = chunk[chunk_offset];
		  },
		  it_target_first,
		  it_target_first + std::distance(it_index_first, it_index_last));
	}

	template <typename ItIndexFirst, typename ItIndexLast, typename ItDataFirst>
	FORCEINLINE auto
	copy_dense_into(ItIndexFirst it_index_first, ItIndexLast it_index_last, ItDataFirst it_target_first) const noexcept
		requires(IS_ASSIGNABLE)
	{
		cinvoke_for(
		  it_index_first,
		  it_index_last,
		  [this](auto index, chunk_type& chunk, index_type chunk_offset, ItDataFirst dataIt) {
			  psl_assert(chunk[chunk_offset] != TOMBSTONE, "expected valid index in sparse array");
			  if constexpr(IS_COMPLEX) {
				  *dataIt = *dense_storage_type::unsafe_data(chunk[chunk_offset]);
			  } else {
				  std::memcpy(
					&*dataIt, dense_storage_type::unsafe_data(chunk[chunk_offset]), dense_storage_type::type_size());
			  }
		  },
		  it_target_first,
		  it_target_first + std::distance(it_index_first, it_index_last));
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
		auto& internal_index = userspace_to_internal_guarantee(convert_from_user_type(index));
		if(internal_index == TOMBSTONE) {
			insert(&index, &index + 1);
		}
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

	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst = void*, typename DataItLast = void*>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto insert(IndexItFirst it_index_first,
									  IndexItLast it_index_last,
									  DataItFirst it_data_first = nullptr,
									  DataItLast it_data_last	= nullptr) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		index_type* begin = &convert_from_user_type(it_index_first);
		index_type* end	  = begin + std::distance(it_index_first, it_index_last);
		return insert_impl<insertion_mode::insert>(begin, end, it_data_first, it_data_last);
	}


	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst = void*, typename DataItLast = void*>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto try_insert(IndexItFirst it_index_first,
										  IndexItLast it_index_last,
										  DataItFirst it_data_first = nullptr,
										  DataItLast it_data_last	= nullptr) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		index_type* begin = &convert_from_user_type(it_index_first);
		index_type* end	  = begin + std::distance(it_index_first, it_index_last);
		return insert_impl<insertion_mode::try_insert>(begin, end, it_data_first, it_data_last);
	}

	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst, typename DataItLast = void*>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto set(IndexItFirst it_index_first,
								   IndexItLast it_index_last,
								   DataItFirst it_data_first,
								   DataItLast it_data_last = nullptr) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		index_type* begin = &convert_from_user_type(it_index_first);
		index_type* end	  = begin + std::distance(it_index_first, it_index_last);
		return insert_impl<insertion_mode::set>(begin, end, it_data_first, it_data_last);
	}

	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst, typename DataItLast = void*>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto assign(IndexItFirst it_index_first,
									  IndexItLast it_index_last,
									  DataItFirst it_data_first,
									  DataItLast it_data_last = nullptr) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		index_type* begin = &convert_from_user_type(it_index_first);
		index_type* end	  = begin + std::distance(it_index_first, it_index_last);
		return insert_impl<insertion_mode::assign>(begin, end, it_data_first, it_data_last);
	}

	template <typename IndexItFirst, typename IndexItLast>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto erase(IndexItFirst it_index_first, IndexItLast it_index_last) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		index_type* begin = &convert_from_user_type(it_index_first);
		index_type* end	  = begin + std::distance(it_index_first, it_index_last);
		return erase_impl<false>(begin, end);
	}

	template <typename IndexItFirst, typename IndexItLast>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	constexpr FORCEINLINE auto try_erase(IndexItFirst it_index_first, IndexItLast it_index_last) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		index_type* begin = &convert_from_user_type(it_index_first);
		index_type* end	  = begin + std::distance(it_index_first, it_index_last);
		return erase_impl<true>(begin, end);
	}

	/// \brief Promotes all values to the next `stage_t`. The cycle is as follows: ADDED -> SETTLED -> REMOVED -> deleted.
	constexpr FORCEINLINE auto promote() noexcept -> void {
		std::span<index_type> reverse_view(std::next(m_Reverse.begin(), m_StageStart[2]), m_Reverse.end());

		if(!reverse_view.empty()) {
			invoke_for<false>(reverse_view.begin(),
							  reverse_view.end(),
							  [this](index_type index, chunk_type& chunk, index_type chunk_offset) {
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
			if constexpr(IS_FLAG) {
				added = insert_impl<insertion_mode::try_insert>(other.m_Reverse.data(),
																other.m_Reverse.data() + other.m_Reverse.size());
			} else {
				added += insert_impl<insertion_mode::set>(other.m_Reverse.data(),
														  other.m_Reverse.data() + other.m_Reverse.size(),
														  other.dense_storage_type::begin(),
														  other.dense_storage_type::end());
			}
		}

		if(other.m_StageSize[2] > 0) {
			erase_impl<true>(std::next(other.m_Reverse.data(), other.m_StageStart[2]),
							 other.m_Reverse.data() + other.m_Reverse.size());
		}

		return merge_result {.success = true, .added = added};
	}

	template <typename InvocableFound,
			  typename InvocableNotFound,
			  typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = void*,
			  typename DataItLast  = void*>
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
		index_type* begin = &convert_from_user_type(first);
		index_type* end	  = begin + std::distance(first, last);
		invoke_for<false>(
		  begin,
		  end,
		  [&InvocableOnFound,
		   &InvocableOnNotFound,
		   range_begin = psl::narrow_cast<index_type>(stage_begin(range)),
		   range_end   = psl::narrow_cast<index_type>(stage_end(range))](
			index_type index, chunk_type& chunk, index_type chunk_offset, DataItFirst dataIt = {}) {
			  if(chunk[chunk_offset] < range_begin || chunk[chunk_offset] >= range_end) {
				  InvocableOnNotFound(index);
			  }
			  if constexpr(IS_FLAG) {
				  InvocableOnFound(index);
			  } else if constexpr(std::is_same_v<DataItFirst, void*>) {
				  InvocableOnFound(index, (pointer)dense_storage_type::unsafe_data(chunk[chunk_offset]));
			  } else {
				  InvocableOnFound(index, (pointer)dense_storage_type::unsafe_data(chunk[chunk_offset]), &*dataIt);
			  }
		  },
		  data_first,
		  data_last,
		  InvocableOnNotFound);
	}


	template <typename InvocableFound,
			  typename InvocableNotFound,
			  typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = void*,
			  typename DataItLast  = void*>
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
		index_type* begin = &convert_from_user_type(first);
		index_type* end	  = begin + std::distance(first, last);
		invoke_for<false>(
		  begin,
		  end,
		  [&InvocableOnFound](index_type index, chunk_type& chunk, index_type chunk_offset, DataItFirst dataIt = {}) {
			  if constexpr(IS_FLAG) {
				  InvocableOnFound(index);
			  } else if constexpr(std::is_same_v<DataItFirst, void*>) {
				  InvocableOnFound(index, (pointer)dense_storage_type::unsafe_data(chunk[chunk_offset]));
			  } else {
				  InvocableOnFound(index, (pointer)dense_storage_type::unsafe_data(chunk[chunk_offset]), &*dataIt);
			  }
		  },
		  data_first,
		  data_last,
		  InvocableOnNotFound);
	}

	template <typename Invocable,
			  typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = void*,
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
		index_type* begin = &convert_from_user_type(first);
		index_type* end	  = begin + std::distance(first, last);
		invoke_for<false>(
		  begin,
		  end,
		  [&InvocableOnFound](index_type index, chunk_type& chunk, index_type chunk_offset, DataItFirst dataIt = {}) {
			  if constexpr(IS_FLAG) {
				  InvocableOnFound(index);
			  } else if constexpr(std::is_same_v<DataItFirst, void*>) {
				  InvocableOnFound(index, (pointer)dense_storage_type::unsafe_data(chunk[chunk_offset]));
			  } else {
				  InvocableOnFound(index, (pointer)dense_storage_type::unsafe_data(chunk[chunk_offset]), &*dataIt);
			  }
		  },
		  data_first,
		  data_last);
	}

	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst = void*, typename DataItLast = void*>
		requires(impl::IsIteratorLikeType<IndexItFirst, Key> && impl::IsIteratorLikeType<IndexItLast, Key>)
	FORCEINLINE auto for_each_guarantee(auto&& Invocable,
										IndexItFirst first,
										IndexItLast last,
										DataItFirst data_first = nullptr,
										DataItLast data_last   = nullptr) -> index_type {
		if(first == last) {
			return 0;
		}
		index_type* begin = &convert_from_user_type(first);
		index_type* end	  = begin + std::distance(first, last);

		auto const size = psl::narrow_cast<index_type>(m_Reverse.size());
		dense_storage_type::reserve(size + psl::narrow_cast<index_type>(std::distance(first, last)));
		invoke_for<true>(
		  begin,
		  end,
		  [&Invocable, this](index_type index, chunk_type& chunk, index_type chunk_offset, DataItFirst dataIt = {}) {
			  index_type rev_index {chunk[chunk_offset]};
			  if(rev_index == TOMBSTONE) {
				  chunk[index] = psl::narrow_cast<index_type>(m_Reverse.size()) - m_StageSize[2];
				  rev_index	   = psl::narrow_cast<index_type>(m_Reverse.size());
				  m_Reverse.emplace_back(index);
				  dense_storage_type::emplace_back();
			  }

			  if constexpr(std::is_same_v<DataItFirst, void*>) {
				  Invocable(index, (pointer)dense_storage_type::unsafe_data(rev_index));
			  } else {
				  Invocable(index, (pointer)dense_storage_type::unsafe_data(rev_index), &*dataIt);
			  }
		  },
		  data_first,
		  data_last);

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
				psl_assert(mapping.has(m_Reverse[i]), "mapping didnt have the ID {}", m_Reverse[i]);
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
	/// \brief Invokes the callback for each index in the given range, sorted by chunk.
	template <bool AutoCreate,
			  typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst		   = void*,
			  typename DataItLast		   = void*,
			  typename InvocableNotFoundFn = void*>
	FORCEINLINE auto invoke_for(IndexItFirst it_index_first,
								IndexItLast it_index_last,
								auto&& Cb,
								DataItFirst it_data_first		 = nullptr,
								DataItLast it_data_last			 = nullptr,
								InvocableNotFoundFn&& CbNotFound = nullptr) {
		using view_type = std::span<index_type>;
		auto const size = psl::narrow_cast<index_type>(std::distance(it_index_first, it_index_last));
		if(size == 0) {
			return;
		}
		view_type view =
		  std::span<index_type>((index_type*)&*it_index_first, std::distance(it_index_first, it_index_last));
		if constexpr(std::is_same_v<DataItLast, void*>) {
			if(std::is_sorted(view.begin(), view.end())) {
				invoke_for_impl<AutoCreate, true>(view, Cb, it_data_first, CbNotFound);
			} else {
				invoke_for_impl<AutoCreate, false>(view, Cb, it_data_first, CbNotFound);
			}
		} else {
			if(std::is_sorted(it_index_first, it_index_last)) {
				invoke_for_impl<AutoCreate, true>(view, Cb, it_data_first, it_data_last, CbNotFound);
			} else {
				invoke_for_impl<AutoCreate, false>(view, Cb, it_data_first, it_data_last, CbNotFound);
			}
		}
	}

	template <bool AutoCreate,
			  bool PreSorted,
			  typename View,
			  typename DataItFirst		   = void*,
			  typename DataItLast		   = void*,
			  typename InvocableNotFoundFn = void*>
	FORCEINLINE auto invoke_for_impl(View& view,
									 auto&& Cb,
									 DataItFirst it_data_first		= nullptr,
									 DataItLast it_data_last		= nullptr,
									 InvocableNotFoundFn CbNotFound = nullptr) {
		static_assert(std::is_same_v<InvocableNotFoundFn, void*> || !AutoCreate,
					  "Cannot autocreate with a not-found-callback");

		if constexpr(AutoCreate && PreSorted) {
			sparse_guarantee_for_userspace(*(std::prev(view.end())));
		} else if constexpr(AutoCreate && !PreSorted) {
			sparse_guarantee_for_userspace(*std::max_element(view.begin(), view.end()));
		}


		auto it = view.begin();
		do {
			auto const first_index = *it;
			index_type chunk_index {};
			index_type element_index {};
			chunk_info_for(first_index, element_index, chunk_index);
			size_t const prev_treshold {(chunk_index)*CHUNKS_SIZE};
			size_t const next_treshold {prev_treshold + CHUNKS_SIZE};
			if constexpr(!std::is_same_v<InvocableNotFoundFn, void*>) {
				if(chunk_index >= m_Sparse.size() || !m_Sparse[chunk_index]) {
					for(;;) {
						if(it == view.end()) {
							return;
						}
						auto const next_index = *it;
						if constexpr(PreSorted) {
							if(chunk_index < m_Sparse.size() && next_index >= next_treshold) {
								break;
							}
						} else {
							if(next_index >= next_treshold || next_index < prev_treshold) {
								break;
							}
						}
						CbNotFound(next_index);
						++it;
					}
					continue;
				}
			} else {
				psl_assert(chunk_index < m_Sparse.size(), "Chunk index out of bounds");
			}
			auto& chunkPtr = m_Sparse[chunk_index];
			if constexpr(AutoCreate) {
				if(!chunkPtr) {
					chunkPtr = std::make_unique<chunk_type>(CHUNKS_SIZE, TOMBSTONE);
				}
			} else {
				psl_assert(chunkPtr, "Chunk pointer cannot be null");
			}

			auto& chunk = *chunkPtr;
			for(;;) {
				if(it == view.end()) {
					break;
				}
				auto const next_index = *it;
				if constexpr(PreSorted) {
					if(next_index >= next_treshold) {
						break;
					}
				} else {
					if(next_index >= next_treshold || next_index < prev_treshold) {
						break;
					}
				}

				auto const next_element_index = next_index - static_cast<index_type>(prev_treshold);
				if constexpr(!std::is_same_v<InvocableNotFoundFn, void*>) {
					if(chunk[next_element_index] == TOMBSTONE) {
						CbNotFound(next_index);
						++it;
						continue;
					}
				}

				if constexpr(std::is_same_v<DataItFirst, void*>) {
					Cb(next_index, chunk, next_element_index);
				} else if constexpr(std::is_same_v<DataItLast, void*>) {
					Cb(next_index, chunk, next_element_index, it_data_first);
				} else if constexpr(!std::is_same_v<View, std::span<index_type>>) {
					Cb(next_index, chunk, next_element_index, it->second);
				} else if constexpr(std::is_same_v<View, std::span<index_type>>) {
					Cb(next_index, chunk, next_element_index, it_data_first);
					++it_data_first;
				} else {
					psl_assert(false, "unreachable");
				}
				++it;
			}
		} while(it != view.end());
	}

	template <bool PreSorted, typename View, typename DataItFirst = void*, typename DataItLast = void*>
	FORCEINLINE auto cinvoke_for_impl(View& view,
									  auto&& Cb,
									  DataItFirst it_data_first = nullptr,
									  DataItLast it_data_last	= nullptr) const {
		auto it = view.begin();
		do {
			auto const first_index = *it;
			index_type chunk_index {};
			index_type element_index {};
			chunk_info_for(first_index, element_index, chunk_index);
			psl_assert(chunk_index < m_Sparse.size(), "Chunk index out of bounds");
			auto& chunkPtr = m_Sparse[chunk_index];
			psl_assert(chunkPtr, "Chunk pointer cannot be null");
			auto& chunk = *chunkPtr;
			size_t const prev_treshold {(chunk_index)*CHUNKS_SIZE};
			size_t const next_treshold {prev_treshold + CHUNKS_SIZE};

			for(;;) {
				if(it == view.end()) {
					break;
				}
				auto const next_index = *it;
				if constexpr(PreSorted) {
					if(next_index >= next_treshold) {
						break;
					}
				} else {
					if(next_index >= next_treshold || next_index < prev_treshold) {
						break;
					}
				}
				auto const next_element_index = next_index - static_cast<index_type>(prev_treshold);
				if constexpr(std::is_same_v<DataItFirst, void*>) {
					Cb(next_index, chunk, next_element_index);
				} else if constexpr(std::is_same_v<DataItLast, void*>) {
					Cb(next_index, chunk, next_element_index, it_data_first);
				} else if constexpr(!std::is_same_v<View, std::span<index_type>>) {
					Cb(next_index, chunk, next_element_index, it->second);
				} else if constexpr(std::is_same_v<View, std::span<index_type>>) {
					Cb(next_index, chunk, next_element_index, it_data_first);
					++it_data_first;
				} else {
					psl_assert(false, "unreachable hit");
				}
				++it;
			}
		} while(it != view.end());
	}

	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst = void*, typename DataItLast = void*>
	FORCEINLINE auto cinvoke_for(IndexItFirst it_index_first,
								 IndexItLast it_index_last,
								 auto&& Cb,
								 DataItFirst it_data_first = nullptr,
								 DataItLast it_data_last   = nullptr) const {
		using view_type = std::span<index_type>;
		auto const size = psl::narrow_cast<index_type>(std::distance(it_index_first, it_index_last));
		if(size == 0) {
			return;
		}
		view_type view =
		  std::span<index_type>((index_type*)&*it_index_first, std::distance(it_index_first, it_index_last));
		if constexpr(std::is_same_v<DataItLast, void*>) {
			if(std::is_sorted(view.begin(), view.end())) {
				cinvoke_for_impl<true>(view, Cb, it_data_first);
			} else {
				cinvoke_for_impl<false>(view, Cb, it_data_first);
			}
		} else {
			if(std::is_sorted(it_index_first, it_index_last)) {
				cinvoke_for_impl<true>(view, Cb, it_data_first, it_data_last);
			} else {
				cinvoke_for_impl<false>(view, Cb, it_data_first, it_data_last);
			}
		}
	}

	/// \brief Invokes the callback for each index in the given range, sorted by chunk, in reverse order.
	template <bool AutoCreate, bool SkipMissing, bool PreSorted = false, typename IndexItFirst, typename IndexItLast>
	FORCEINLINE auto reverse_invoke_for(IndexItFirst it_index_first, IndexItLast it_index_last, auto&& Cb) {
		if(it_index_first == it_index_last) {
			return;
		}
		std::span<index_type> view((index_type*)&*it_index_first, std::distance(it_index_first, it_index_last));
		if constexpr(!PreSorted) {
			if(std::is_sorted(view.begin(), view.end())) {
				reverse_invoke_for<AutoCreate, SkipMissing, true>(it_index_first, it_index_last, Cb);
				return;
			}
		}
		auto it = view.rbegin();

		if constexpr(AutoCreate && PreSorted) {
			sparse_guarantee_for_userspace(*std::prev(view.end()));
		} else if constexpr(AutoCreate && !PreSorted) {
			sparse_guarantee_for_userspace(*std::max_element(view.begin(), view.end()));
		}

		if constexpr(SkipMissing && !AutoCreate && PreSorted) {
			auto const first_index = *std::begin(view);
			index_type chunk_index {};
			index_type element_index {};
			chunk_info_for(first_index, element_index, chunk_index);
			if(chunk_index >= m_Sparse.size()) {
				return;
			}
		}

		do {
			auto const first_index = *it;
			index_type chunk_index {};
			index_type element_index {};
			chunk_info_for(first_index, element_index, chunk_index);
			size_t next_treshold {chunk_index * CHUNKS_SIZE};
			size_t prev_treshold {next_treshold + CHUNKS_SIZE};

			if constexpr(SkipMissing && !AutoCreate) {
				if(chunk_index >= m_Sparse.size() || !m_Sparse[chunk_index]) {
					for(;;) {
						if(it == view.rend()) {
							break;
						}
						auto const next_index = *it;
						if constexpr(PreSorted) {
							if(next_index < next_treshold) {
								break;
							}
						} else {
							if(next_index < next_treshold || next_index >= prev_treshold) {
								break;
							}
						}
						++it;
					}
					continue;
				}
			}
			auto& chunkPtr = m_Sparse[chunk_index];
			if constexpr(AutoCreate) {
				if(!chunkPtr) {
					chunkPtr = std::make_unique<chunk_type>(CHUNKS_SIZE, TOMBSTONE);
				}
			} else {
				psl_assert(chunkPtr, "Chunk pointer cannot be null");
			}
			auto& chunk = *chunkPtr;

			for(;;) {
				if(it == view.rend()) {
					break;
				}
				auto const next_index = *it;
				if constexpr(PreSorted) {
					if(next_index < next_treshold) {
						break;
					}
				} else {
					if(next_index < next_treshold || next_index >= prev_treshold) {
						break;
					}
				}
				auto const next_element_index = next_index - static_cast<index_type>(next_treshold);
				Cb(next_index, chunk, next_element_index);
				++it;
			}
		} while(it != view.rend());
	}

	template <bool TryErase, typename IndexItFirst, typename IndexItLast>
	constexpr FORCEINLINE auto erase_impl(IndexItFirst it_index_first, IndexItLast it_index_last) -> index_type {
		if(it_index_first == it_index_last) {
			return 0;
		}
		if(m_StageStart[2] == 0) {
			return 0;
		}
		if constexpr(!TryErase) {
			psl_assert(!std::empty(m_Reverse), "cannot erase from an empty staged_sparse_array");
		}

		auto const original_deleted = m_StageSize[2];

		reverse_invoke_for<false, TryErase>(
		  it_index_first, it_index_last, [this](index_type user_index, chunk_type& chunk, index_type chunk_offset) {
			  if constexpr(TryErase) {
				  if(chunk[chunk_offset] == TOMBSTONE) {
					  return;
				  }
			  } else {
				  psl_assert(chunk[chunk_offset] != TOMBSTONE);
			  }
			  auto reverse_index = chunk[chunk_offset];
			  auto what_stage	 = (reverse_index < m_StageStart[1]) ? 0 : (reverse_index < m_StageStart[2]) ? 1 : 2;
			  if(what_stage == 2) {
				  return;
			  }

			  // swap with the back of my range, afterwards
			  for(auto i = what_stage; i < 2; ++i) {
				  auto const original_index = m_StageStart[i + 1] - 1;
				  auto const original_value = m_Reverse[original_index];
				  if(original_index != reverse_index) {
					  auto const sparse_index  = &chunk - m_Sparse.begin()->get();
					  auto const next_treshold = sparse_index * CHUNKS_SIZE;
					  /*if(original_value >= next_treshold && original_value < next_treshold + CHUNKS_SIZE) {
						  auto const distance =
							original_value > index ? original_value - index : index - original_value;
						  chunk[chunk_offset + distance] = reverse_index;
					  } else {*/
					  auto original_sparse_offset = original_value;
					  auto original_sparse		  = userspace_to_internal(original_sparse_offset);
					  (*m_Sparse[original_sparse])[original_sparse_offset] = reverse_index;
					  //}

					  std::iter_swap(std::next(std::begin(m_Reverse), reverse_index),
									 std::next(std::begin(m_Reverse), original_index));

					  dense_storage_type::swap(original_index, reverse_index);

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

	enum class insertion_mode {
		insert,
		try_insert,
		set,
		assign,
	};

	template <insertion_mode InsertMode, typename DataItFirst = void*, typename DataItLast = void*>
	auto insert_impl(index_type const* it_index_first,
					 index_type const* it_index_last,
					 DataItFirst it_data_first = nullptr,
					 DataItLast it_data_last   = nullptr) -> index_type {
		// this function behaves a bit different depending on the mode (normal or try-insert), and if data is provided.
		// insert mode /w data:
		// - does all the normal insertions as if no data is present
		// - at the end will make space and insert. If there's a last-data iterator it will advance the data-iterator
		// otherwise it will consume the first one continiously
		// data iterator towards the next if an end iterator is present.
		// - afterwards it will rotate the dense data and swap it with the ones in the removed state.
		// insert mode /w no data:
		// - normal insertion operations for all indices
		// - afterwards it will inject space for the new indices in the dense storage before the removed indices stage
		// try-insert mode /w data:
		// - reserves all the required size for the data upfront in the dense storage
		// - during insertions it will emplace the data at the back of the dense storage. It will always advance the
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

		static_assert(std::is_same_v<DataItFirst, void*> || IS_ASSIGNABLE, "Only assignable types can have data");
		static_assert((InsertMode != insertion_mode::assign && InsertMode != insertion_mode::set) ||
						!std::is_same_v<DataItFirst, void*>,
					  "set/assign without data makes no sense, use (try-)insert");
		const auto size = psl::narrow_cast<index_type>(std::distance(it_index_first, it_index_last));
		if(size == 0) {
			return 0;
		}

		psl_assert(m_Reverse.empty() || it_index_first < m_Reverse.data() ||
					 it_index_first > m_Reverse.data() + m_Reverse.size(),
				   "Cannot self-assign");

		if constexpr(!std::is_same_v<DataItFirst, void*> && !std::is_same_v<DataItLast, void*>) {
			psl_assert(std::distance(it_data_first, it_data_last) == size,
					   "Data iterator range must match the index iterator range in size");
		}

		m_Reverse.reserve(m_StageStart[3] + size);

		// as we'll insert as we go along, we need to reserve space upfront
		// try_insert can have holes and set can add new elements.
		if constexpr(IS_ASSIGNABLE && (InsertMode == insertion_mode::try_insert || InsertMode == insertion_mode::set) &&
					 !std::is_same_v<DataItFirst, void*>) {
			dense_storage_type::reserve(m_StageStart[3] + size);
		}
		index_type count = 0;
		invoke_for<true>(
		  it_index_first,
		  it_index_last,
		  [this, &count](index_type index, chunk_type& chunk, index_type chunk_offset, DataItFirst dataIt = {}) {
			  auto& reverse_index = chunk[chunk_offset];
			  // try_insert requires the element to be empty
			  if constexpr(InsertMode == insertion_mode::try_insert) {
				  if(reverse_index != TOMBSTONE) {
					  return;
				  }
			  }
			  // if we're inserting or assigning, we need to ensure that the chunk is empty
			  else if constexpr(InsertMode == insertion_mode::insert) {
				  psl_assert(reverse_index == TOMBSTONE);
			  }
			  // both insert and try_insert always add to the end
			  if constexpr(InsertMode == insertion_mode::try_insert || InsertMode == insertion_mode::insert) {
				  reverse_index = m_StageStart[2] + count;
				  m_Reverse.emplace_back(index);
				  if constexpr(InsertMode == insertion_mode::try_insert && !std::is_same_v<DataItFirst, void*>) {
					  dense_storage_type::emplace_back(dataIt);
				  }
				  ++count;
			  }
			  // set might add a new element, or it will overwrite an existing one
			  else if constexpr(InsertMode == insertion_mode::set) {
				  if(reverse_index != TOMBSTONE) {
					  dense_storage_type::set(reverse_index, dataIt);
				  } else {
					  reverse_index = m_StageStart[2] + count;
					  m_Reverse.emplace_back(index);
					  dense_storage_type::emplace_back(dataIt);
					  ++count;
				  }
			  }
			  // assign will always overwrite an existing element, and skip missing ones
			  else {
				  if(reverse_index != TOMBSTONE) {
					  dense_storage_type::set(reverse_index, dataIt);
				  }
			  }
		  },
		  it_data_first,
		  it_data_last);

		if(count > 0) {
			if constexpr(IS_ASSIGNABLE && InsertMode == insertion_mode::try_insert &&
						 std::is_same_v<DataItFirst, void*>) {
				dense_storage_type::reserve(m_StageStart[3] + count);
				dense_storage_type::insert_space(m_StageStart[2], count);
			}
		}

		if(m_StageSize[2] > 0 && count > 0) {
			for(auto it = m_Reverse.begin() + m_StageStart[2], end = m_Reverse.begin() + m_StageStart[3]; it != end;
				++it) {
				index_type element_index, chunk_index;
				chunk_info_for(*it, element_index, chunk_index);
				(*m_Sparse[chunk_index])[element_index] += count;
			}
			std::rotate(m_Reverse.begin() + m_StageStart[2],
						m_Reverse.begin() + m_StageStart[3],
						m_Reverse.begin() + m_StageStart[3] + count);

			// both try_insert and set can add new data elements at the end of the dense storage, so we'll rotate those
			if constexpr(IS_ASSIGNABLE &&
						 (InsertMode == insertion_mode::try_insert || InsertMode == insertion_mode::set) &&
						 !std::is_same_v<DataItFirst, void*>) {
				dense_storage_type::rotate(m_StageStart[2], m_StageStart[3], m_StageStart[3] + count);
			}
		}

		// when inserting we only need to reserve space and insert at the end as we can be certain these
		// are all new elements.
		if constexpr(IS_ASSIGNABLE && InsertMode == insertion_mode::insert) {
			dense_storage_type::reserve(m_StageStart[3] + count);
			if constexpr(!std::is_same_v<DataItFirst, void*>) {
				auto dataIt = it_data_first;
				for(size_t i = 0; i < count; ++i) {
					dense_storage_type::emplace_back(dataIt);
					if constexpr(!std::is_same_v<DataItLast, void*>) {
						++dataIt;
					}
				}
				dense_storage_type::rotate(m_StageStart[2], m_StageStart[3], m_StageStart[3] + count);
			} else {
				dense_storage_type::insert_space(m_StageStart[2], count);
			}
		}

		m_StageStart[2] += count;
		m_StageStart[3] += count;
		m_StageSize[1] += count;
		return count;
	}

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

	/// \brief Same as `userspace_to_internal`, but creates the chunk if it doesn't exist.
	constexpr FORCEINLINE auto userspace_to_internal_guarantee(index_type index) noexcept -> index_type& {
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
		auto& chunk = *chunkPtr;

		return chunk[element_index];
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

	psl::array<index_type> m_Reverse {};
	chunk_storage_type m_Sparse {};

	psl::static_array<index_type, 4> m_StageStart {0, 0, 0, 0};
	psl::static_array<index_type, 3> m_StageSize {0, 0, 0};
};

}	 // namespace psl::ecs::details
