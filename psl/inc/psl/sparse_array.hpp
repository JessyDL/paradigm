#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/assertions.hpp"
#include "psl/details/buffer_growth_strategy.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/platform_def.hpp"
#include "psl/utility/cast.hpp"

#include <cstring>	  // std::memmove
#include <memory>	  // std::uninitialized_move
#include <span>

namespace psl {

namespace impl {
	template <typename T>
	concept StorageContiguousAccessible = requires(T a) {
		{ a.begin() };
		{ a.end() };
		{ a.size() } -> std::convertible_to<size_t>;
	};

	template <typename T>
	concept StorageDataAccessible = requires(T a) {
		{ a.data() };
	};

	template <typename T, typename Key>
	struct dense_storage_base_t {
		dense_storage_base_t(Key initial_size = 16) : m_Dense(initial_size * sizeof(T)) {
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
				// no-op for trivial types, when available use std::start_lifetime_as<T>(m_End);
			} else {
				new(m_End) T();
			}
			++m_End;
		}

		template <typename U>
		void emplace_back(U&& value) {
			psl_assert(size() < capacity(), "no more space left to create new elements");
			if constexpr(std::is_trivially_copyable_v<T>) {
				std::memcpy(m_End, &value, sizeof(T));
				// when available use std::start_lifetime_as<T>(m_End);
			} else {
				new(m_End) T(value);
			}
			++m_End;
		}

		void reserve(Key new_size) {
			if(new_size > capacity()) {
				auto const old_size = m_End - m_Begin;

				::memory::raw_region new_dense(new_size * sizeof(T));
				if constexpr(std::is_trivially_copyable_v<T>) {
					std::memcpy(new_dense.data(), m_Dense.data(), old_size * sizeof(T));
					// when available use std::start_lifetime_as<T>(...);
				} else {
					auto currentPtr		= m_Begin;
					auto newTargetPtr	= (T*)new_dense.data();
					auto new_dense_size = new_dense.size();
					newTargetPtr		= (T*)std::align(alignof(T), sizeof(T), (void*&)newTargetPtr, new_dense_size);
					psl_assert(new_dense_size >= (m_End - m_Begin) * sizeof(T),
							   "new dense size must be greater than or equal to current size");
					for(size_t i = 0; i < size(); ++i) {
						new(newTargetPtr) T(std::move(*currentPtr));
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
					new(targetPtr) T(std::move(*currentPtr));
				}
			}
			m_End += count;
		}

		void truncate(Key new_size) {
			psl_assert(new_size <= size(), "new size must be less than or equal to current size");
			auto const old_size = size();
			if constexpr(!std::is_trivially_destructible_v<T>) {
				for(size_t i = new_size; i < old_size; ++i) {
					m_End->~T();
					--m_End;
				}
			} else {
				m_End -= (old_size - new_size);
			}
			std::memset(m_End, 0, (old_size - new_size) * sizeof(T));
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

		T* data() noexcept {
			return m_Begin;
		}
		const T* data() const noexcept {
			return m_Begin;
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

	template <typename T, typename Key>
	struct no_storage_base_t {
		no_storage_base_t([[maybe_unused]] Key initial_size = 16) {};

		Key capacity() const noexcept {
			return std::numeric_limits<Key>::max();
		}

		template <typename U>
		void set([[maybe_unused]] Key index, [[maybe_unused]] U&& value) {}
		void swap([[maybe_unused]] Key first, [[maybe_unused]] Key second) {}
		void emplace_back() {}
		template <typename U>
		void emplace_back([[maybe_unused]] U&& value) {}
		void reserve([[maybe_unused]] Key new_size) {}
		void insert_space([[maybe_unused]] Key index, [[maybe_unused]] Key count) {}
		void truncate([[maybe_unused]] Key new_size) {}
		void clear([[maybe_unused]] bool release_memory = false) noexcept {}
	};

	template <typename T, bool SingleValue = false>
	class iterator_range_wrapper {
	  public:
		iterator_range_wrapper(T first, T last) : m_First(first), m_Last(last), m_Current(first) {}
		~iterator_range_wrapper()										 = default;
		iterator_range_wrapper(const iterator_range_wrapper&)			 = default;
		iterator_range_wrapper(iterator_range_wrapper&&)				 = default;
		iterator_range_wrapper& operator=(const iterator_range_wrapper&) = default;
		iterator_range_wrapper& operator=(iterator_range_wrapper&&)		 = default;

		T next() {
			if constexpr(!SingleValue) {
				psl_assert(m_Current != m_Last, "cannot go past the end of the range");
				++m_Current;
			}
			return m_Current;
		}

		T begin() const noexcept {
			return m_First;
		}

		T begin() noexcept {
			return m_First;
		}

		T end() const noexcept {
			return m_Last;
		}
		T end() noexcept {
			return m_Last;
		}

		auto size() const noexcept -> size_t {
			if constexpr(SingleValue) {
				return 1;
			} else {
				return std::distance(m_First, m_Last);
			}
		}

		void reset() noexcept {
			m_Current = m_First;
		}

		bool has_next() const noexcept {
			if constexpr(SingleValue) {
				return true;
			} else {
				return m_Current != m_Last;
			}
		}

		T current() const noexcept {
			return m_Current;
		}


	  private:
		T m_Current;
		T const m_First;
		T const m_Last;
	};

	template <typename T, typename Value>
	concept IsIteratorLikeType = std::is_same_v<std::remove_cvref_t<decltype(*std::declval<T>())>, Value>;
}	 // namespace impl

template <typename T,
		  typename UserKey				= size_t,
		  typename IndexType			= UserKey,
		  IndexType CHUNKS_SIZE			= 4096,
		  typename BufferGrowthStrategy = details::default_buffer_growth_strategy_t,
		  typename StorageType			= impl::dense_storage_base_t<T, IndexType>>
class sparse_array {
	static_assert(std::is_convertible_v<UserKey, IndexType>, "UserKey must be convertible to IndexType");
	static_assert(std::is_same_v<T, std::remove_cvref_t<T>>,
				  "sparse_array does not support cvref types, please use a non-cvref type");
	static_assert(std::is_pointer_v<T> == false,
				  "sparse_array does not support pointer types, please use a non-pointer type");
	using user_index_type	 = UserKey;
	using index_type		 = IndexType;
	using dense_storage_type = StorageType;
	using chunk_type		 = psl::array<index_type>;
	using chunk_ptr_type	 = std::unique_ptr<chunk_type>;
	using chunk_storage_type = psl::array<chunk_ptr_type>;

	using size_type			= typename psl::array<index_type>::size_type;
	using difference_type	= typename psl::array<index_type>::difference_type;
	using value_type		= std::remove_cvref_t<T>;
	using pointer			= value_type*;
	using const_pointer		= value_type const*;
	using reference			= value_type&;
	using const_reference	= const value_type&;
	using iterator_category = std::random_access_iterator_tag;

	static constexpr auto TOMBSTONE = std::numeric_limits<index_type>::max();
	static constexpr bool IS_CHUNKS_POW_2 {CHUNKS_SIZE && ((CHUNKS_SIZE & (CHUNKS_SIZE - 1)) == 0)};
	static constexpr IndexType MOD_VAL {(IS_CHUNKS_POW_2) ? CHUNKS_SIZE - 1 : CHUNKS_SIZE};

	enum class insertion_mode {
		insert,
		try_insert,
		set,
		assign,
	};

  public:
	sparse_array(index_type initial_size = 16) : m_Data(initial_size) {
		m_Reverse.reserve(initial_size);
	}

	sparse_array(sparse_array const&)			 = default;
	sparse_array(sparse_array&&)				 = default;
	sparse_array& operator=(sparse_array const&) = default;
	sparse_array& operator=(sparse_array&&)		 = default;
	~sparse_array()								 = default;

	constexpr FORCEINLINE auto operator[](user_index_type index) const -> const_reference
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		auto element_index = static_cast<index_type>(index);
		auto chunk		   = userspace_to_internal(element_index);
		psl_assert(chunk != TOMBSTONE, "index not found in sparse array");
		psl_assert(m_Sparse[chunk] != nullptr, "index not found in sparse array");
		auto dense_index = (*m_Sparse[chunk])[element_index];
		psl_assert(dense_index != TOMBSTONE, "index not found in sparse array");
		return m_Data[dense_index];
	}

	constexpr FORCEINLINE auto operator[](user_index_type index) -> reference
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		auto element_index = static_cast<index_type>(index);
		auto chunk		   = userspace_to_internal(element_index);
		auto dense_index   = (*m_Sparse[chunk])[element_index];
		if(dense_index == TOMBSTONE) {
			insert(index);
			dense_index = (*m_Sparse[chunk])[element_index];
		}
		return m_Data[dense_index];
	}

	constexpr FORCEINLINE auto at(user_index_type index) const -> const_reference
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		auto element_index = static_cast<index_type>(index);
		auto chunk		   = std::as_const(*this).userspace_to_internal(element_index);
		psl_assert(chunk != TOMBSTONE, "index not found in sparse array");
		psl_assert(m_Sparse[chunk] != nullptr, "index not found in sparse array");
		auto dense_index = (*m_Sparse[chunk])[element_index];
		return m_Data[dense_index];
	}

	constexpr FORCEINLINE auto at(user_index_type index) -> reference
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		auto element_index = static_cast<index_type>(index);
		auto chunk		   = std::as_const(*this).userspace_to_internal(element_index);
		psl_assert(chunk != TOMBSTONE, "index not found in sparse array");
		psl_assert(m_Sparse[chunk] != nullptr, "index not found in sparse array");
		auto dense_index = (*m_Sparse[chunk])[element_index];
		return m_Data[dense_index];
	}

	constexpr FORCEINLINE auto try_get(user_index_type index) const -> const_pointer
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		auto element_index = static_cast<index_type>(index);
		auto chunk		   = std::as_const(*this).userspace_to_internal(element_index);
		if(chunk == TOMBSTONE || m_Sparse[chunk] == nullptr) {
			return nullptr;
		}
		auto dense_index = (*m_Sparse[chunk])[element_index];
		if(dense_index == TOMBSTONE) {
			return nullptr;
		}
		auto& data = m_Data[dense_index];
		return std::addressof(data);
	}

	constexpr FORCEINLINE auto try_get(user_index_type index) -> pointer
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		auto element_index = static_cast<index_type>(index);
		auto chunk		   = std::as_const(*this).userspace_to_internal(element_index);
		if(chunk == TOMBSTONE || m_Sparse[chunk] == nullptr) {
			return nullptr;
		}
		auto dense_index = (*m_Sparse[chunk])[element_index];
		if(dense_index == TOMBSTONE) {
			return nullptr;
		}
		auto& data = m_Data[dense_index];
		return std::addressof(data);
	}

	constexpr FORCEINLINE auto size() const noexcept -> index_type {
		return psl::narrow_cast<index_type>(m_Reverse.size());
	}
	constexpr FORCEINLINE auto capacity() const noexcept -> index_type {
		return m_Data.capacity();
	}
	constexpr FORCEINLINE auto empty() const noexcept -> bool {
		return size() == 0;
	}
	constexpr FORCEINLINE void reserve(index_type count) noexcept {
		if(count <= capacity())
			return;
		count = psl::narrow_cast<index_type>(BufferGrowthStrategy {}.growth(m_Reverse.capacity(), count));
		m_Reverse.reserve(count);
		m_Data.reserve(count);
	}

	constexpr FORCEINLINE auto begin() noexcept -> pointer
		requires(impl::StorageContiguousAccessible<dense_storage_type>)
	{
		return m_Data.begin();
	}

	constexpr FORCEINLINE auto begin() const noexcept -> const_pointer
		requires(impl::StorageContiguousAccessible<dense_storage_type>)
	{
		return m_Data.begin();
	}

	constexpr FORCEINLINE auto end() noexcept -> pointer
		requires(impl::StorageContiguousAccessible<dense_storage_type>)
	{
		return m_Data.end();
	}

	constexpr FORCEINLINE auto end() const noexcept -> const_pointer
		requires(impl::StorageContiguousAccessible<dense_storage_type>)
	{
		return m_Data.end();
	}

	constexpr FORCEINLINE auto data() const noexcept -> const_pointer
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return reinterpret_cast<const_pointer>(m_Data.data());
	}

	constexpr FORCEINLINE auto data() noexcept -> pointer
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return reinterpret_cast<pointer>(m_Data.data());
	}

	FORCEINLINE auto indices() const noexcept -> std::span<user_index_type const> {
		if(m_Reverse.empty()) {
			return {};
		}
		return std::span<user_index_type const>(
		  reinterpret_cast<user_index_type const*>(m_Reverse.data()),
		  reinterpret_cast<user_index_type const*>(m_Reverse.data() + m_Reverse.size()));
	}


	constexpr FORCEINLINE void clear(bool release_memory = false) noexcept {
		m_Reverse.clear();
		m_Sparse.clear();
		m_Data.clear(release_memory);
	}

	template <typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = std::nullptr_t,
			  typename DataItLast  = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, user_index_type> &&
				 impl::IsIteratorLikeType<IndexItLast, user_index_type>)
				constexpr FORCEINLINE auto insert(IndexItFirst it_index_first,
												  IndexItLast it_index_last,
												  DataItFirst it_data_first = {},
												  DataItLast it_data_last	= {}) -> index_type
					requires(impl::StorageDataAccessible<dense_storage_type> ||
							 std::is_same_v<DataItFirst, std::nullptr_t>)
	{
		if(it_index_first == it_index_last) {
			return index_type {0};
		}

		auto [index_span, data_span] = to_span_wrapper(it_index_first, it_index_last, it_data_first, it_data_last);
		return insert_impl<insertion_mode::insert>(index_span, data_span);
	}

	constexpr FORCEINLINE auto insert(user_index_type index, const_reference value) -> bool
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return insert(&index, &index + 1, &value) != 0;
	}

	constexpr FORCEINLINE auto insert(user_index_type index) -> bool {
		return insert(&index, &index + 1) != 0;
	}

	template <typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = std::nullptr_t,
			  typename DataItLast  = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, user_index_type> &&
				 impl::IsIteratorLikeType<IndexItLast, user_index_type>)
				constexpr FORCEINLINE auto try_insert(IndexItFirst it_index_first,
													  IndexItLast it_index_last,
													  DataItFirst it_data_first = {},
													  DataItLast it_data_last	= {}) -> index_type
					requires(impl::StorageDataAccessible<dense_storage_type> ||
							 std::is_same_v<DataItFirst, std::nullptr_t>)
	{
		if(it_index_first == it_index_last) {
			return index_type {0};
		}

		auto [index_span, data_span] = to_span_wrapper(it_index_first, it_index_last, it_data_first, it_data_last);
		return insert_impl<insertion_mode::try_insert>(index_span, data_span);
	}

	constexpr FORCEINLINE auto try_insert(user_index_type index, const_reference value) -> bool
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return try_insert(&index, &index + 1, &value) != 0;
	}

	constexpr FORCEINLINE auto try_insert(user_index_type index) -> bool {
		return try_insert(&index, &index + 1) != 0;
	}

	constexpr FORCEINLINE auto contains(user_index_type index) const noexcept -> bool {
		auto element_index = static_cast<index_type>(index);
		auto chunk		   = userspace_to_internal(element_index);
		if(chunk == TOMBSTONE) {
			return false;
		}
		if(chunk >= m_Sparse.size()) {
			return false;
		}
		if(m_Sparse[chunk] == nullptr) {
			return false;
		}
		auto dense_index = (*m_Sparse[chunk])[element_index];
		return dense_index != TOMBSTONE;
	}

	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst, typename DataItLast = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, user_index_type> &&
				 impl::IsIteratorLikeType<IndexItLast, user_index_type>)
				constexpr FORCEINLINE auto set(IndexItFirst it_index_first,
											   IndexItLast it_index_last,
											   DataItFirst it_data_first,
											   DataItLast it_data_last = {}) -> index_type
					requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		if(it_index_first == it_index_last) {
			return index_type {0};
		}

		auto [index_span, data_span] = to_span_wrapper(it_index_first, it_index_last, it_data_first, it_data_last);
		return insert_impl<insertion_mode::set>(index_span, data_span);
	}

	constexpr FORCEINLINE auto set(user_index_type index, const_reference value) -> bool
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return set(&index, &index + 1, &value) != 0;
	}


	template <typename IndexItFirst, typename IndexItLast, typename DataItFirst, typename DataItLast = std::nullptr_t>
		requires(impl::IsIteratorLikeType<IndexItFirst, user_index_type> &&
				 impl::IsIteratorLikeType<IndexItLast, user_index_type>)
				constexpr FORCEINLINE auto assign(IndexItFirst it_index_first,
												  IndexItLast it_index_last,
												  DataItFirst it_data_first,
												  DataItLast it_data_last = {}) -> index_type
					requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		if(it_index_first == it_index_last) {
			return index_type {0};
		}

		auto [index_span, data_span] = to_span_wrapper(it_index_first, it_index_last, it_data_first, it_data_last);
		return insert_impl<insertion_mode::assign>(index_span, data_span);
	}

	constexpr FORCEINLINE auto assign(user_index_type index, const_reference value) -> bool
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return assign(&index, &index + 1, &value) != 0;
	}

	template <typename IndexItFirst, typename IndexItLast>
		requires(impl::IsIteratorLikeType<IndexItFirst, user_index_type> &&
				 impl::IsIteratorLikeType<IndexItLast, user_index_type>)
	constexpr FORCEINLINE auto erase(IndexItFirst begin, IndexItLast end) -> index_type {
		if(begin == end) {
			return index_type {0};
		}
		auto [index_span, _] = to_span_wrapper(std::make_reverse_iterator(end), std::make_reverse_iterator(begin));
		return erase_impl<false>(index_span);
	}

	constexpr FORCEINLINE auto erase(user_index_type index) -> bool {
		return erase(&index, &index + 1) != 0;
	}

	template <typename IndexItFirst, typename IndexItLast>
		requires(impl::IsIteratorLikeType<IndexItFirst, user_index_type> &&
				 impl::IsIteratorLikeType<IndexItLast, user_index_type>)
	constexpr FORCEINLINE auto try_erase(IndexItFirst begin, IndexItLast end) -> index_type {
		if(begin == end) {
			return index_type {0};
		}
		auto [index_span, _] = to_span_wrapper(std::make_reverse_iterator(end), std::make_reverse_iterator(begin));
		return erase_impl<true>(index_span);
	}

	constexpr FORCEINLINE auto try_erase(user_index_type index) -> bool {
		return try_erase(&index, &index + 1) != 0;
	}

  private:
	template <typename IndexItFirst,
			  typename IndexItLast,
			  typename DataItFirst = std::nullptr_t,
			  typename DataItLast  = std::nullptr_t>
	auto to_span_wrapper(IndexItFirst it_index_first,
						 IndexItLast it_index_last,
						 DataItFirst it_data_first = std::nullptr_t {},
						 DataItLast it_data_last   = std::nullptr_t {}) const noexcept {
		if constexpr(std::is_same_v<std::remove_cvref_t<DataItFirst>, std::nullptr_t>) {
			return std::pair {
			  impl::iterator_range_wrapper<std::remove_cvref_t<IndexItFirst>, false>(it_index_first, it_index_last),
			  std::nullptr_t {}};
		} else if constexpr(std::is_same_v<std::remove_cvref_t<DataItLast>, std::nullptr_t>) {
			return std::pair {
			  impl::iterator_range_wrapper<std::remove_cvref_t<IndexItFirst>, false>(it_index_first, it_index_last),
			  impl::iterator_range_wrapper<std::remove_cvref_t<DataItFirst>, true>(it_data_first, it_data_first + 1)};
		} else {
			psl_assert(std::distance(it_index_first, it_index_last) == std::distance(it_data_first, it_data_last),
					   "index and data iterators must have the same distance");
			return std::pair {
			  impl::iterator_range_wrapper<std::remove_cvref_t<IndexItFirst>, false>(it_index_first, it_index_last),
			  impl::iterator_range_wrapper<std::remove_cvref_t<DataItFirst>, false>(it_data_first, it_data_last)};
		}
	}

	template <insertion_mode InsertMode>
	constexpr FORCEINLINE auto insert_impl(auto&& index_span, auto&& data_span) -> index_type {
		const auto size = psl::narrow_cast<index_type>(index_span.size());
		m_Reverse.reserve(m_Reverse.size() + size);
		m_Data.reserve(psl::narrow_cast<index_type>(m_Reverse.size()) + size);
		index_type count = 0;

		invoke_for_l0<true>(
		  index_span,
		  [&, this](index_type index, chunk_type& chunk, index_type chunk_offset, auto&&... dataIt) {
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
				  rev_index = psl::narrow_cast<index_type>(m_Reverse.size());
				  m_Reverse.emplace_back(index);
				  if constexpr(InsertMode == insertion_mode::try_insert && sizeof...(dataIt) > 0) {
					  m_Data.emplace_back(*dataIt...);
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
						  m_Data.set(rev_index, *dataIt...);
					  }
				  } else {
					  rev_index = psl::narrow_cast<index_type>(m_Reverse.size());
					  m_Reverse.emplace_back(index);
					  if constexpr(sizeof...(dataIt) == 0) {
						  psl_assert(false, "set without data makes no sense, use (try-)insert");
					  } else {
						  m_Data.emplace_back(*dataIt...);
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
						  m_Data.set(rev_index, *dataIt...);
						  ++count;	  // count successful assignments
					  }
				  }
			  }
		  },
		  data_span);

		if constexpr(InsertMode == insertion_mode::try_insert || InsertMode == insertion_mode::insert) {
			if(count == 0) {
				return count;
			}
			m_Data.reserve(psl::narrow_cast<index_type>(m_Reverse.size()));
			if constexpr(InsertMode == insertion_mode::insert &&
						 !std::is_same_v<std::remove_cvref_t<decltype(data_span)>, std::nullptr_t>) {
				for(size_t i = 0; i < count; ++i) {
					m_Data.emplace_back(*data_span.current());
					data_span.next();
				}
			} else {
				m_Data.insert_space(psl::narrow_cast<index_type>(m_Reverse.size()) - count, count);
			}
		}

		return count;
	}

	template <bool TryErase>
	constexpr FORCEINLINE auto erase_impl(auto&& range) -> index_type {
		if constexpr(!TryErase) {
			psl_assert(!std::empty(m_Reverse), "cannot erase from an empty staged_sparse_array");
		}

		index_type start_size = psl::narrow_cast<index_type>(m_Reverse.size());

		invoke_for_l0<false>(range, [this](index_type user_index, chunk_type& chunk, index_type chunk_offset) {
			if constexpr(TryErase) {
				if(chunk[chunk_offset] == TOMBSTONE) {
					return;
				}
			} else {
				psl_assert(chunk[chunk_offset] != TOMBSTONE);
			}
			auto reverse_index = chunk[chunk_offset];
			auto last_index	   = m_Reverse.back();
			// if we're not removing the last element, we need to swap the last element into the removed element's place
			// otherwise we can just pop the last element
			if(last_index != user_index) {
				auto const chunk_index = &chunk - m_Sparse.front().get();
				if(last_index >= chunk_index * CHUNKS_SIZE && last_index < (chunk_index + 1) * CHUNKS_SIZE) {
					chunk[last_index - (chunk_index * CHUNKS_SIZE)] = reverse_index;
				} else {
					auto original_sparse_offset = last_index;
					auto original_sparse		= userspace_to_internal(original_sparse_offset);
					(*m_Sparse[original_sparse])[original_sparse_offset] = reverse_index;
				}

				std::iter_swap(std::next(std::begin(m_Reverse), reverse_index), std::prev(std::end(m_Reverse)));
				m_Data.swap(reverse_index, psl::narrow_cast<index_type>(m_Reverse.size()) - 1);
			}

			m_Reverse.pop_back();
			m_Data.truncate(psl::narrow_cast<index_type>(m_Reverse.size()) - 1);
			chunk[chunk_offset] = TOMBSTONE;
		});

		return start_size - psl::narrow_cast<index_type>(m_Reverse.size());
	}

	template <bool AutoCreate, typename DataSpan = std::nullptr_t, typename CbNotFound = std::nullptr_t>
	constexpr FORCEINLINE auto invoke_for_l0(auto index_span,
											 auto&& CallbackFound,
											 DataSpan data_span			   = nullptr,
											 CbNotFound&& CallbackNotFound = nullptr) {
		if(index_span.size() == 0) {
			return;
		}
		if(std::is_sorted(index_span.begin(), index_span.end())) {
			if constexpr(AutoCreate) {
				sparse_guarantee_for_userspace(*(std::prev(index_span.end())));
			}
			invoke_for_l1<AutoCreate, true>(index_span, CallbackFound, data_span, CallbackNotFound);
		} else {
			if constexpr(AutoCreate) {
				sparse_guarantee_for_userspace(*std::max_element(index_span.begin(), index_span.end()));
			}
			invoke_for_l1<AutoCreate, false>(index_span, CallbackFound, data_span, CallbackNotFound);
		}
	}

	template <bool AutoCreate, bool PreSorted, typename DataSpan = std::nullptr_t, typename CbNotFound = std::nullptr_t>
	constexpr FORCEINLINE auto invoke_for_l1(auto&& index_span,
											 auto&& CallbackFound,
											 DataSpan&& data_span		   = nullptr,
											 CbNotFound&& CallbackNotFound = nullptr) {
		constexpr auto HasNotFoundCb = !std::is_same_v<std::remove_cvref_t<CbNotFound>, std::nullptr_t>;
		constexpr auto HasDataSpan	 = !std::is_same_v<std::remove_cvref_t<DataSpan>, std::nullptr_t>;

		do {
			auto const first_index = static_cast<index_type>(*index_span.current());
			index_type chunk_index {};
			index_type element_index {};
			chunk_info_for(first_index, element_index, chunk_index);
			size_t const prev_treshold {(chunk_index)*CHUNKS_SIZE};
			size_t const next_treshold {prev_treshold + CHUNKS_SIZE};

			if constexpr(HasNotFoundCb) {
				if(chunk_index >= m_Sparse.size() || !m_Sparse[chunk_index]) {
					for(;;) {
						if(!index_span.has_next()) {
							return;
						}
						auto const next_index = static_cast<index_type>(*index_span.current());
						if constexpr(PreSorted) {
							if(chunk_index < m_Sparse.size() && next_index >= next_treshold) {
								break;
							}
						} else {
							if(next_index >= next_treshold || next_index < prev_treshold) {
								break;
							}
						}
						CallbackNotFound(next_index);
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
				if(!index_span.has_next()) {
					break;
				}
				auto const next_index = static_cast<index_type>(*index_span.current());
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
				if constexpr(HasNotFoundCb) {
					if(chunk[next_element_index] == TOMBSTONE) {
						CallbackNotFound(next_index);
						index_span.next();
						continue;
					}
				}

				if constexpr(!HasDataSpan) {
					CallbackFound(next_index, chunk, next_element_index);
				} else {
					auto data_it = data_span.current();
					CallbackFound(next_index, chunk, next_element_index, data_it);
					data_span.next();
				}
				index_span.next();
			}
		} while(index_span.has_next());
	}

	// utilities

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

	constexpr FORCEINLINE auto userspace_to_internal(index_type& index) const noexcept -> index_type {
		index_type chunk_index;
		index_type element_index;
		chunk_info_for(index, element_index, chunk_index);
		if(m_Sparse.size() <= chunk_index) {
			index = TOMBSTONE;
			return TOMBSTONE;
		}
		auto& chunkPtr = m_Sparse[chunk_index];
		if(!chunkPtr) {
			index = TOMBSTONE;
			return TOMBSTONE;
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

	dense_storage_type m_Data {};
	psl::array<index_type> m_Reverse {};
	chunk_storage_type m_Sparse {};
};


/// \brief A sparse array that only stores the indices of the elements that are present.
/// The value type is std::byte which doesn't matter but it needs something, and the storage type does not actually
/// store any data
/// \tparam UserKey The type used by the user to index the sparse array.
/// \tparam IndexType The internal type used to index the sparse array. These is the internal type used, typically this
/// should be the same as UserKey, but for certain opaque types this can be different.
/// \tparam CHUNKS_SIZE The size of each chunk in the sparse array. This should be a power of two for optimal performance.
/// \tparam BufferGrowthStrategy The strategy used to grow the internal buffers.
/// \see psl::sparse_array for more information.
template <typename UserKey				= size_t,
		  typename IndexType			= UserKey,
		  IndexType CHUNKS_SIZE			= 4096,
		  typename BufferGrowthStrategy = details::default_buffer_growth_strategy_t>
using sparse_indice_array = psl::sparse_array<std::byte,
											  UserKey,
											  IndexType,
											  CHUNKS_SIZE,
											  BufferGrowthStrategy,
											  impl::no_storage_base_t<std::byte, IndexType>>;
}	 // namespace psl
