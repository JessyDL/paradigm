#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/assertions.hpp"
#include "psl/details/buffer_growth_strategy.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/platform_def.hpp"
#include "psl/thread_safety_guard.hpp"
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

	struct no_data_t {};

	template <typename T>
	concept IsNoDataSpecialization = std::is_same_v<T, no_data_t>;

	/// \brief A storage type that uses a memory::raw_region as its backing storage. Typically this is a virtual page allocator
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
					(--m_End)->~T();
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

	/// \brief A storage type that does not actually store anything, but provides the same interface as dense_storage_base_t
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

	/// \brief A simple iterator range wrapper that provides begin/end/size functionality as well as next/has_next/current functionality
	/// \details This is primarily used to provide a uniform interface for iterating over ranges of iterators or single values reducing code
	/// bloat slightly in the sparse_array implementation.
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

	/// \brief A helper function to create an iterator_range_wrapper from either a single value or a range of iterators.
	/// \details If only a single value is provided, a SingleValue iterator_range_wrapper is
	/// created. If a range of iterators is provided, a normal iterator_range_wrapper is created.
	/// If a nullptr is provided, a nullptr_t is returned.
	/// \note This function is used internally by the sparse_array to provide a uniform interface for iterating over ranges of iterators or single values.
	template <typename ItFirst, typename ItLast = std::nullptr_t>
	constexpr FORCEINLINE auto to_span_wrapper(ItFirst first, ItLast last = nullptr) noexcept {
		if constexpr(std::is_same_v<std::remove_cvref_t<ItFirst>, std::nullptr_t>) {
			return std::nullptr_t {};
		} else if constexpr(std::is_same_v<std::remove_cvref_t<ItLast>, std::nullptr_t>) {
			return impl::iterator_range_wrapper<std::remove_cvref_t<ItFirst>, true>(first, first + 1);
		} else {
			return impl::iterator_range_wrapper<std::remove_cvref_t<ItFirst>, false>(first, last);
		}
	}
}	 // namespace impl

/// \brief A sparse array implementation that provides a mapping between user provided keys and a dense storage of values.
/// \tparam T The type of value to store in the sparse array
/// \tparam UserKey The type of key provided by the user to access values in the sparse array. Must be convertible to IndexType.
/// \tparam IndexType The internal index type used by the sparse array. Must be an integral type.
/// \tparam CHUNKS_SIZE The size of each chunk in the sparse array. Should be a power of 2 for optimal performance.
/// \tparam BufferGrowthStrategy The strategy used to grow the internal storage of the sparse array. Defaults to doubling the size.
/// \tparam StorageType The type of dense storage to use for the values in the sparse array. Must provide a similar interface to impl::dense_storage_base_t.
///
/// \details The sparse_array is a data structure that provides a mapping between user provided keys and a dense storage of values.
/// Unlike a traditional array, the sparse_array does not require contiguous storage for all possible keys. Instead the
/// keys are indirections that map to the dense storage of values. The sparse_array is implemented using a chunked
/// approach, where it is divided into chunks of a fixed size (CHUNKS_SIZE). Each chunk contains an array of indices
/// that map to the dense storage of values. This allows for efficient insertion, deletion, and access of values in the
/// sparse array.
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
	template <typename... Args>
	sparse_array(index_type initial_size = 16, Args&&... args) : m_Data(initial_size, std::forward<Args>(args)...) {
		m_Reverse.reserve(initial_size);
	}

	sparse_array(sparse_array const&)			 = default;
	sparse_array(sparse_array&&)				 = default;
	sparse_array& operator=(sparse_array const&) = default;
	sparse_array& operator=(sparse_array&&)		 = default;
	~sparse_array()								 = default;

	/// \brief Fetches the stored data at the provided index.
	/// \param index The user provided index to the value.
	/// \return A reference to the stored data at the provided index.
	/// \details This function will assert if the index is not found in the sparse array.
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

	/// \brief Fetches the stored data at the provided index, inserting a new element if it does not exist.
	/// \param index The user provided index to the value.
	/// \return A reference to the stored data at the provided index.
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

	/// \brief Fetches the stored data at the provided index.
	/// \param index The user provided index to the value.
	/// \return A reference to the stored data at the provided index.
	/// \details This function will assert if the index is not found in the sparse array.
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

	/// \brief Fetches the stored data at the provided index.
	/// \param index The user provided index to the value.
	/// \return A reference to the stored data at the provided index.
	/// \details This function will assert if the index is not found in the sparse array.
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

	/// \brief Attempts to fetch the stored data at the provided index.
	/// \param index The user provided index to the value.
	/// \return A pointer to the stored data at the provided index, or nullptr if the index is not found.
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

	/// \brief Attempts to fetch the stored data at the provided index.
	/// \param index The user provided index to the value.
	/// \return A pointer to the stored data at the provided index, or nullptr if the index is not found.
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

	/// \brief Returns the number of elements in the sparse array.
	constexpr FORCEINLINE auto size() const noexcept -> index_type {
		return psl::narrow_cast<index_type>(m_Reverse.size());
	}

	/// \brief Returns the capacity of the sparse array.
	constexpr FORCEINLINE auto capacity() const noexcept -> index_type {
		return m_Data.capacity();
	}

	/// \brief Returns true if the sparse array is empty.
	constexpr FORCEINLINE auto empty() const noexcept -> bool {
		return size() == 0;
	}

	/// \brief Reserves space in the sparse array for at least count elements.
	constexpr FORCEINLINE void reserve(index_type count) noexcept {
		if(count <= capacity())
			return;
		count = psl::narrow_cast<index_type>(BufferGrowthStrategy {}.growth(m_Reverse.capacity(), count));
		m_Reverse.reserve(count);
		m_Data.reserve(count);
	}

	/// \brief Returns an iterator to the beginning of the dense storage.
	/// \details Only available if the dense storage type supports contiguous access (i.e. provides begin/end methods).
	constexpr FORCEINLINE auto begin() noexcept -> pointer
		requires(impl::StorageContiguousAccessible<dense_storage_type>)
	{
		return m_Data.begin();
	}

	/// \brief Returns an iterator to the beginning of the dense storage.
	/// \details Only available if the dense storage type supports contiguous access (i.e. provides begin/end methods).
	constexpr FORCEINLINE auto begin() const noexcept -> const_pointer
		requires(impl::StorageContiguousAccessible<dense_storage_type>)
	{
		return m_Data.begin();
	}

	/// \brief Returns an iterator to the end of the dense storage.
	/// \details Only available if the dense storage type supports contiguous access (i.e. provides begin/end methods).
	constexpr FORCEINLINE auto end() noexcept -> pointer
		requires(impl::StorageContiguousAccessible<dense_storage_type>)
	{
		return m_Data.end();
	}

	/// \brief Returns an iterator to the end of the dense storage.
	/// \details Only available if the dense storage type supports contiguous access (i.e. provides begin/end methods).
	constexpr FORCEINLINE auto end() const noexcept -> const_pointer
		requires(impl::StorageContiguousAccessible<dense_storage_type>)
	{
		return m_Data.end();
	}

	/// \brief Returns a pointer to the underlying data of the dense storage.
	/// \details Only available if the dense storage type supports data access (i.e. provides a data method).
	constexpr FORCEINLINE auto data() const noexcept -> const_pointer
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return reinterpret_cast<const_pointer>(m_Data.data());
	}

	/// \brief Returns a pointer to the underlying data of the dense storage.
	/// \details Only available if the dense storage type supports data access (i.e. provides a data method).
	constexpr FORCEINLINE auto data() noexcept -> pointer
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return reinterpret_cast<pointer>(m_Data.data());
	}

	/// \brief Returns a span of the user indices currently stored in the sparse array.
	FORCEINLINE auto indices() const noexcept -> std::span<user_index_type const> {
		if(m_Reverse.empty()) {
			return {};
		}
		return std::span<user_index_type const>(
		  reinterpret_cast<user_index_type const*>(m_Reverse.data()),
		  reinterpret_cast<user_index_type const*>(m_Reverse.data() + m_Reverse.size()));
	}

	/// \brief Clears the sparse array, optionally releasing memory.
	constexpr FORCEINLINE void clear(bool release_memory = false) noexcept {
		m_Reverse.clear();
		m_Sparse.clear();
		m_Data.clear(release_memory);
	}

	/// \brief Inserts a range of indices and optional data into the sparse array.
	/// \tparam IndexItFirst The type of the first iterator for the indices.
	/// \tparam IndexItLast The type of the last iterator for the indices.
	/// \tparam DataItFirst The type of the first iterator for the data.
	/// \tparam DataItLast The type of the last iterator for the data.
	///
	/// \details This function will insert the provided indices into the sparse array.
	/// If data iterators are provided, the corresponding data will be inserted as well.
	/// If only one data iterator is provided, it is assumed to be a single value to be used for all indices.
	/// If both data iterators are provided, they must match the number of indices.
	/// All indices must be unique and not already present in the sparse array.
	///
	/// \warning Do not provide an end data iterator if you are providing a single value.
	/// \return The number of indices that were actually inserted. Note that this will be equal to the number of indices provided, otherwise
	/// it's considered an error on the user's part.
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

		auto index_span = impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= impl::to_span_wrapper(it_data_first, it_data_last);
		auto lock		= m_Guard->scoped_guard();
		return insert_impl<insertion_mode::insert>(index_span, data_span);
	}

	/// \brief Inserts a single index and value into the sparse array.
	/// \see insert
	constexpr FORCEINLINE auto insert(user_index_type index, const_reference value) -> bool
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return insert(&index, &index + 1, &value) != 0;
	}

	/// \brief Inserts a single index into the sparse array.
	/// \see insert
	constexpr FORCEINLINE auto insert(user_index_type index) -> bool {
		return insert(&index, &index + 1) != 0;
	}

	/// \brief Attempts to insert a range of indices and optional data into the sparse array.
	/// \tparam IndexItFirst The type of the first iterator for the indices.
	/// \tparam IndexItLast The type of the last iterator for the indices.
	/// \tparam DataItFirst The type of the first iterator for the data.
	/// \tparam DataItLast The type of the last iterator for the data.
	///
	/// \details This function will attempt to insert the provided indices into the sparse array.
	/// It behaves exactly the same as `insert`, except that if an index is already present in the sparse array, it will
	/// be skipped instead of being considered an error.
	/// \see insert
	/// \return The number of indices that were actually inserted.
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

		auto index_span = impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= impl::to_span_wrapper(it_data_first, it_data_last);
		auto lock		= m_Guard->scoped_guard();
		return insert_impl<insertion_mode::try_insert>(index_span, data_span);
	}

	/// \brief Attempts to insert a single index and value into the sparse array.
	/// \see try_insert
	/// \return True if the index was inserted, false if it was already present.
	constexpr FORCEINLINE auto try_insert(user_index_type index, const_reference value) -> bool
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return try_insert(&index, &index + 1, &value) != 0;
	}

	/// \brief Attempts to insert a single index into the sparse array.
	/// \see try_insert
	/// \return True if the index was inserted, false if it was already present.
	constexpr FORCEINLINE auto try_insert(user_index_type index) -> bool {
		return try_insert(&index, &index + 1) != 0;
	}

	/// \brief Checks if the sparse array contains the provided index.
	/// \param index The user provided index to check.
	/// \return True if the index is present in the sparse array, false otherwise.
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

	/// \brief Sets the value at the provided indices, inserting them if they do not exist.
	/// \tparam IndexItFirst The type of the first iterator for the indices.
	/// \tparam IndexItLast The type of the last iterator for the indices.
	/// \tparam DataItFirst The type of the first iterator for the data.
	/// \tparam DataItLast The type of the last iterator for the data.
	///
	/// \details This function will set the provided indices in the sparse array to the provided values.
	/// If they already existed it will overwrite the existing value. If they did not exist, they will be inserted.
	/// This method requires that the dense storage type supports data access (i.e. provides a data method). Otherwise
	/// it is assumed the dense storage has no backing storage.
	///
	/// \return The number of indices that were newly inserted.
	/// \warning Do not provide an end data iterator if you are providing a single value.
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

		auto index_span = impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= impl::to_span_wrapper(it_data_first, it_data_last);
		auto lock		= m_Guard->scoped_guard();
		return insert_impl<insertion_mode::set>(index_span, data_span);
	}

	/// \brief Sets the value at the provided index, inserting it if it does not exist.
	/// \see set
	constexpr FORCEINLINE auto set(user_index_type index, const_reference value) -> bool
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return set(&index, &index + 1, &value) != 0;
	}

	/// \brief Assigns the value at the provided indices, overwriting them if they exist, skipping them if they do not.
	/// \tparam IndexItFirst The type of the first iterator for the indices.
	/// \tparam IndexItLast The type of the last iterator for the indices.
	/// \tparam DataItFirst The type of the first iterator for the data.
	/// \tparam DataItLast The type of the last iterator for the data.
	///
	/// \details This function will assign the provided indices in the sparse array to the provided values.
	/// If the indice did not exist, it will be skipped.
	/// This method requires that the dense storage type supports data access (i.e. provides a data method). Otherwise
	/// it is assumed the dense storage has no backing storage.
	///
	/// \return The number of indices that were actually assigned.
	/// \warning Do not provide an end data iterator if you are providing a single value.
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
		auto index_span = impl::to_span_wrapper(it_index_first, it_index_last);
		auto data_span	= impl::to_span_wrapper(it_data_first, it_data_last);
		return insert_impl<insertion_mode::assign>(index_span, data_span);
	}

	/// \brief Assigns the value at the provided index, overwriting it if it exists, skipping it if it does not.
	/// \see assign
	constexpr FORCEINLINE auto assign(user_index_type index, const_reference value) -> bool
		requires(impl::StorageDataAccessible<dense_storage_type>)
	{
		return assign(&index, &index + 1, &value) != 0;
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
		auto index_span = impl::to_span_wrapper(std::make_reverse_iterator(end), std::make_reverse_iterator(begin));
		auto lock		= m_Guard->scoped_guard();
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
		auto index_span = impl::to_span_wrapper(std::make_reverse_iterator(end), std::make_reverse_iterator(begin));
		auto lock		= m_Guard->scoped_guard();
		return erase_impl<true>(index_span);
	}

	/// \brief Attempts to erase the value at the provided index from the sparse array.
	/// \see try_erase
	constexpr FORCEINLINE auto try_erase(user_index_type index) -> bool {
		return try_erase(&index, &index + 1) != 0;
	}

	/// \brief Returns the internal dense storage index of the provided user index.
	/// \param index The user provided index to look up.
	/// \details This function is only available really for sparse arrays that do not store any data,
	/// this allows for a mapping between user indices and dense indices. Useful for tracking indices
	/// in other data structures.
	/// \see psl::sparse_indices_array
	constexpr FORCEINLINE auto index_of(user_index_type index) const -> index_type
		requires(impl::IsNoDataSpecialization<value_type> && !impl::StorageDataAccessible<dense_storage_type>)
	{
		auto element_index = static_cast<index_type>(index);
		auto chunk		   = std::as_const(*this).userspace_to_internal(element_index);
		psl_assert(chunk != TOMBSTONE && m_Sparse[chunk] != nullptr, "index not found in sparse array");
		auto dense_index = (*m_Sparse[chunk])[element_index];
		psl_assert(dense_index != TOMBSTONE, "index not found in sparse array");
		return dense_index;
	}

  private:
	/// \brief Entrypoint for all operations that modify the sparse array's underlying data, or add new indices.
	/// \tparam InsertMode The mode of insertion to perform. Can be insert, try_insert, set, or assign.
	/// \param index_span A span wrapper containing the indices to operate on.
	/// \param data_span A span wrapper containing the data to operate on. Can be nullptr if no data is to be used.
	/// \return The number of indices that were actually modified/inserted. Depending on the operation, this may be less than the number of indices provided.
	template <insertion_mode InsertMode>
	constexpr FORCEINLINE auto insert_impl(auto&& index_span, auto&& data_span) -> index_type {
		const auto size = psl::narrow_cast<index_type>(index_span.size());

		// assign is thread safe as it only modifies existing elements
		if constexpr(InsertMode != insertion_mode::assign) {
			m_Reverse.reserve(m_Reverse.size() + size);
			m_Data.reserve(psl::narrow_cast<index_type>(m_Reverse.size()) + size);
		}
		index_type count = 0;

		invoke_for_l0<true>(
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

	/// \brief Entrypoint for all erase operations on the sparse array.
	/// \tparam TryErase If true, will skip indices that are not present in the sparse array. If false, will assert if an index is not present.
	/// \param index_span A span wrapper containing the indices to erase.
	/// \return The number of indices that were actually erased.
	template <bool TryErase>
	constexpr FORCEINLINE auto erase_impl(auto&& range) -> index_type {
		if constexpr(!TryErase) {
			psl_assert(!std::empty(m_Reverse), "cannot erase from an empty staged_sparse_array");
		}

		index_type start_size = psl::narrow_cast<index_type>(m_Reverse.size());

		invoke_for_l0<false, std::greater<index_type>>(
		  range, [this](index_type user_index, chunk_type& chunk, index_type chunk_index, index_type chunk_offset) {
			  auto const reverse_index = chunk[chunk_offset];
			  if constexpr(TryErase) {
				  if(reverse_index == TOMBSTONE) {
					  return;
				  }
			  } else {
				  psl_assert(reverse_index != TOMBSTONE);
			  }
			  auto const last_index = m_Reverse.back();
			  // if we're not removing the last element, we need to swap the last element into the removed element's
			  // place otherwise we can just pop the last element
			  if(last_index != user_index) {
				  if(last_index >= chunk_index * CHUNKS_SIZE && last_index < (chunk_index + 1) * CHUNKS_SIZE) {
					  chunk[last_index - (chunk_index * CHUNKS_SIZE)] = reverse_index;
				  } else {
					  auto original_sparse_offset = last_index;
					  auto original_sparse		  = std::as_const(*this).userspace_to_internal(original_sparse_offset);
					  (*m_Sparse[original_sparse])[original_sparse_offset] = reverse_index;
				  }

				  m_Reverse[reverse_index] = last_index;
				  m_Data.swap(reverse_index, psl::narrow_cast<index_type>(m_Reverse.size()) - 1);
			  }

			  m_Reverse.pop_back();
			  m_Data.truncate(psl::narrow_cast<index_type>(m_Reverse.size()));
			  chunk[chunk_offset] = TOMBSTONE;
		  });

		return start_size - psl::narrow_cast<index_type>(m_Reverse.size());
	}

	/// \brief Helper intermediate that will invoke all indices in the provided span, creating chunks as needed if AutoCreate is true.
	/// It will determine if the span is sorted or not, and call the appropriate invoke_for_l1 function.
	/// \tparam AutoCreate If true, will create chunks as needed. If false, will assert if a chunk is missing.
	/// \tparam Cmp The comparator to use for determining if the span is sorted. Defaults to std::less.
	/// \tparam DataSpan The type of the data span wrapper. Can be nullptr if no data is to be used.
	template <bool AutoCreate,
			  typename Cmp		  = std::less<index_type>,
			  typename DataSpan	  = std::nullptr_t,
			  typename CbNotFound = std::nullptr_t>
	constexpr FORCEINLINE auto invoke_for_l0(auto index_span,
											 auto&& CallbackFound,
											 DataSpan data_span			   = nullptr,
											 CbNotFound&& CallbackNotFound = nullptr) {
		if(index_span.size() == 0) {
			return;
		}
		if(std::is_sorted(index_span.begin(), index_span.end(), Cmp {})) {
			if constexpr(AutoCreate) {
				sparse_guarantee_for_userspace(*(std::prev(index_span.end())));
			}
			invoke_for_l1<AutoCreate, true, std::is_same_v<Cmp, std::greater<index_type>>>(
			  index_span, CallbackFound, data_span, CallbackNotFound);
		} else {
			if constexpr(AutoCreate) {
				sparse_guarantee_for_userspace(*std::max_element(index_span.begin(), index_span.end()));
			}
			invoke_for_l1<AutoCreate, false, std::is_same_v<Cmp, std::greater<index_type>>>(
			  index_span, CallbackFound, data_span, CallbackNotFound);
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
			  typename DataSpan	  = std::nullptr_t,
			  typename CbNotFound = std::nullptr_t>
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
			auto const prev_treshold {chunk_index * CHUNKS_SIZE};
			auto const next_treshold {prev_treshold + CHUNKS_SIZE};

			// todo(jdl): This doesn't work properly yet, but we have no use case either
			if constexpr(HasNotFoundCb) {
				psl::not_implemented("Callback for not found is not fully implemented yet");
				/*if(chunk_index >= m_Sparse.size() || !m_Sparse[chunk_index]) {
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
				}*/
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
				if constexpr(HasNotFoundCb) {
					if(chunk[next_element_index] == TOMBSTONE) {
						CallbackNotFound(next_index);
						index_span.next();
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

	/// \brief Calculates the chunk index and element index within the chunk for a given user index.
	/// \param index The user provided index to the value.
	/// \param element_index The output parameter that will hold the index within the chunk.
	/// \param chunk_index The output parameter that will hold the chunk index.
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

	/// \brief Converts a user provided index to the internal chunk index and updates the index to be the offset within the chunk. It will create missing chunks as needed.
	/// \param index The user provided index to the value. This will be updated to be the offset within the chunk.
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

	/// \brief Converts a user index to the internal chunk index and element index.
	/// \param index The user provided index to the value. This will be updated to be the offset within the chunk.
	/// \note This function does not create chunks, and will return TOMBSTONE if the chunk does not exist.
	/// Similarly if the chunk does not exist, the index will be set to TOMBSTONE.
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

	/// \brief Returns the chunk for the given user index, and updates the index to be the offset within the chunk.
	/// \warning This function assumes that the chunk exists. Use sparse_guarantee_for_users
	/// to ensure that the chunk exists if not certain.
	/// \param index The user provided index to the value. This will be updated to be the offset within the chunk.
	constexpr FORCEINLINE auto chunk_for(index_type& index) const noexcept -> chunk_type& {
		auto chunk_index = userspace_to_internal(index);
		psl_assert(chunk_index != TOMBSTONE, "chunk for user index {} does not exist in staged_sparse_array", index);
		return *m_Sparse[chunk_index];
	}

	/// \brief Ensures that the chunk for the given user index exists, creating it if necessary, and updates the index
	/// to be the offset within the chunk.
	/// \param index The user provided index to the value. This will be updated to be the offset within the chunk.
	constexpr FORCEINLINE auto chunk_for_guarantee(index_type& index) noexcept -> chunk_type& {
		auto chunk_index = userspace_to_internal(index);
		psl_assert(chunk_index != TOMBSTONE, "chunk for user index {} does not exist in staged_sparse_array", index);
		return *m_Sparse[chunk_index].get();
	}

	dense_storage_type m_Data {};
	psl::array<index_type> m_Reverse {};
	chunk_storage_type m_Sparse {};

	std::shared_ptr<psl::dbg_thread_safety_guard_t> m_Guard {std::make_shared<psl::dbg_thread_safety_guard_t>()};
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
		  typename BufferGrowthStrategy = details::default_buffer_growth_strategy_t,
		  typename StorageType			= impl::no_storage_base_t<impl::no_data_t, IndexType>>
using sparse_indice_array =
  psl::sparse_array<impl::no_data_t, UserKey, IndexType, CHUNKS_SIZE, BufferGrowthStrategy, StorageType>;
}	 // namespace psl
