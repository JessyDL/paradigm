#pragma once
#include "psl/static_array.h"
#include "psl/template_utils.h"
#include <algorithm>
#include <cmath>	// std::sqrt, etc..
#include <cstddef>
#include <cstring>	  // std::mem*
#include <limits>
#include <utility>
namespace psl
{
	template <typename precision_t, size_t dimensions>
	struct tvec;

	namespace details
	{
		template <typename T, size_t... index>
		struct accessor
		{
			static constexpr size_t length			 = sizeof...(index);
			static constexpr bool is_single_accessor = length == 1;

			inline operator std::conditional_t<is_single_accessor, T&, tvec<T, length>>() noexcept
			{
				if constexpr(length > 1)
				{
					return tvec<T, length> {data[index]...};
				}
				else
				{
					return {data[index]...};
				}
			}

			inline operator std::conditional_t<is_single_accessor, const T&, tvec<T, length>>() const noexcept
			{
				if constexpr(length > 1)
				{
					return tvec<T, length> {data[index]...};
				}
				else
				{
					return {data[index]...};
				}
			}

			inline tvec<T, length> operator=(const tvec<T, length>& other) noexcept
			{
				if constexpr(length > 1)
				{
					auto it = std::begin(other.data);
					(void(data.at(index) = *it++), ...);
				}
				else
				{
					data.at(index...) = other;
				}

				return {data[index]...};
			}

			static constexpr size_t max_elements = utility::templates::max<index...>() + 1;
			psl::static_array<T, max_elements> data;
		};
	}	 // namespace details
#define ACCESSOR_IMPL(type, name, ...) ::psl::details::accessor<type, __VA_ARGS__> name;

#define ACCESSOR_1D_IMPL(type, name, index) ACCESSOR_IMPL(type, name, index)

#define ACCESOR_2D_SHUFFLE(type, name1, name2, index1, index2)                                                         \
	ACCESSOR_IMPL(type, name1##name2, index1, index2)                                                                \
	ACCESSOR_IMPL(type, name2##name1, index2, index1)

#define ACCESOR_3D_SHUFFLE(type, name1, name2, index1, index2)                                                         \
	ACCESSOR_IMPL(type, name1##name1##name2, index1, index1, index2)                                                 \
	ACCESSOR_IMPL(type, name1##name2##name1, index1, index2, index1)                                                 \
	ACCESSOR_IMPL(type, name1##name2##name2, index1, index2, index2)                                                 \
	ACCESSOR_IMPL(type, name2##name1##name1, index2, index1, index1)                                                 \
	ACCESSOR_IMPL(type, name2##name2##name1, index2, index2, index1)                                                 \
	ACCESSOR_IMPL(type, name2##name1##name2, index2, index1, index2)


#define ACCESOR_4D_SHUFFLE3(type, name1, name2, index1, index2)                                                        \
	ACCESSOR_IMPL(type, name1##name1##name1##name2, index1, index1, index1, index2)                                  \
	ACCESSOR_IMPL(type, name1##name1##name2##name1, index1, index1, index2, index1)                                  \
	ACCESSOR_IMPL(type, name1##name2##name1##name1, index1, index2, index1, index1)                                  \
	ACCESSOR_IMPL(type, name2##name1##name1##name1, index2, index1, index1, index1)                                  \
	ACCESSOR_IMPL(type, name1##name2##name2##name1, index1, index2, index2, index1)                                  \
	ACCESSOR_IMPL(type, name1##name2##name1##name2, index1, index2, index1, index2)                                  \
	ACCESSOR_IMPL(type, name1##name1##name2##name2, index1, index1, index2, index2)


#define ACCESOR_4D_SHUFFLE2(type, name1, name2, index1, index2)                                                        \
	ACCESOR_4D_SHUFFLE3(type, name1, name2, index1, index2)                                                            \
	ACCESOR_4D_SHUFFLE3(type, name2, name1, index2, index1)

#define ACCESOR_4D_SHUFFLE1(type, name1, name2, name3, index1, index2, index3)                                         \
	ACCESOR_4D_SHUFFLE2(type, name1, name2, index1, index2)                                                            \
	ACCESOR_4D_SHUFFLE2(type, name1, name3, index1, index3)                                                            \
	ACCESOR_4D_SHUFFLE2(type, name2, name3, index2, index3)


#define ACCESOR_4D_SHUFFLE(type, name1, name2, name3, name4, index1, index2, index3, index4)                           \
	ACCESOR_4D_SHUFFLE1(type, name1, name2, name3, index1, index2, index3)                                             \
	ACCESOR_4D_SHUFFLE1(type, name1, name2, name3, index1, index2, index3)

#define ACCESSOR_2D_IMPL(type, name, index) ACCESSOR_IMPL(type, name##name, index, index)

#define ACCESSOR_3D_IMPL(type, name, index) ACCESSOR_IMPL(type, name##name##name, index, index, index)
#define ACCESSOR_4D_IMPL(type, name, index) ACCESSOR_IMPL(type, name##name##name##name, index, index, index, index)

#define ACCESSOR_1D(type) ACCESSOR_1D_IMPL(type, x, 0)

#define ACCESSOR_2D(type)                                                                                              \
	ACCESSOR_1D_IMPL(type, x, 0)                                                                                       \
	ACCESSOR_1D_IMPL(type, y, 1)                                                                                       \
	ACCESSOR_2D_IMPL(type, x, 0)                                                                                       \
	ACCESSOR_2D_IMPL(type, y, 1)                                                                                       \
	ACCESOR_2D_SHUFFLE(type, x, y, 0, 1)

#define ACCESSOR_3D(type)                                                                                              \
	ACCESSOR_2D(type)                                                                                                  \
	ACCESSOR_1D_IMPL(type, z, 2)                                                                                       \
	ACCESSOR_2D_IMPL(type, z, 1)                                                                                       \
	ACCESOR_2D_SHUFFLE(type, x, z, 0, 2)                                                                               \
	ACCESOR_2D_SHUFFLE(type, y, z, 1, 2)                                                                               \
                                                                                                                       \
	ACCESSOR_3D_IMPL(type, x, 0)                                                                                       \
	ACCESSOR_3D_IMPL(type, y, 1)                                                                                       \
	ACCESSOR_3D_IMPL(type, z, 2)                                                                                       \
	ACCESOR_3D_SHUFFLE(type, x, y, 0, 1)                                                                               \
	ACCESOR_3D_SHUFFLE(type, x, z, 0, 2)                                                                               \
	ACCESOR_3D_SHUFFLE(type, y, z, 1, 2)

#define ACCESSOR_4D(type)                                                                                              \
	ACCESSOR_3D(type)                                                                                                  \
	ACCESSOR_1D_IMPL(type, w, 3)                                                                                       \
	ACCESSOR_2D_IMPL(type, w, 3)                                                                                       \
	ACCESSOR_3D_IMPL(type, w, 3)                                                                                       \
	ACCESOR_2D_SHUFFLE(type, x, w, 0, 3)                                                                               \
	ACCESOR_2D_SHUFFLE(type, y, w, 1, 3)                                                                               \
	ACCESOR_2D_SHUFFLE(type, z, w, 2, 3)                                                                               \
	ACCESOR_3D_SHUFFLE(type, x, w, 0, 3)                                                                               \
	ACCESOR_3D_SHUFFLE(type, y, w, 1, 3)                                                                               \
	ACCESOR_3D_SHUFFLE(type, z, w, 2, 3)                                                                               \
                                                                                                                       \
	ACCESSOR_4D_IMPL(type, x, 0)                                                                                       \
	ACCESSOR_4D_IMPL(type, y, 1)                                                                                       \
	ACCESSOR_4D_IMPL(type, z, 2)                                                                                       \
	ACCESSOR_4D_IMPL(type, w, 3)


#pragma warning(push)
#pragma warning(disable : 5103)
	template <typename precision_t, size_t dimensions>
	struct tvec
	{
		static constexpr size_t dimensions_n {dimensions};
		using tvec_t	  = tvec<precision_t, dimensions>;
		using container_t = psl::static_array<precision_t, dimensions>;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;

		constexpr tvec(const container_t& value) noexcept : value(value) {};
		constexpr tvec(const precision_t& value) noexcept : value(utility::templates::make_array<dimensions>(value)) {};

		template <typename... Args>
		constexpr tvec(Args&&... args) noexcept : value({static_cast<precision_t>(args)...}) {};


		operator container_t() const noexcept { return value; }
		operator container_t&() noexcept { return value; }

		tvec_t& operator=(const container_t& container)
		{
			value = container;
			return *this;
		}
		// ---------------------------------------------
		// operators
		// ---------------------------------------------
		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_standard_layout<tvec_t>::value && std::is_trivially_copyable_v<tvec_t>,
						  "should remain POD");

			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template <size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}
		template <size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}

		template <typename Y>
		operator tvec<Y, dimensions>() const noexcept requires std::convertible_to<precision_t, Y>
		{
			tvec<Y, dimensions> res {};
			for(auto i = 0; i < dimensions; ++i) res[i] = static_cast<Y>(value[i]);
			return res;
		}

		constexpr tvec operator%=(const tvec& other) noexcept
		{
			for(auto i = 0; i < dimensions; ++i) value[i] %= other.value[i];
			return *this;
		}

		constexpr tvec operator%(const tvec& other) const noexcept
		{
			tvec res {*this};
			res %= other;
			return res;
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------

		template <size_t new_size>
		tvec<precision_t, new_size> resize() const noexcept
		{
			if constexpr(new_size == dimensions_n)
			{
				return *this;
			}
			else
			{
				tvec<precision_t, new_size> res {};
				constexpr size_t max_copy_size = std::min(dimensions_n, new_size);
				std::copy(std::begin(value), std::next(std::begin(value), max_copy_size), std::begin(res.value));
				return res;
			}
		}

		container_t value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 1>
	{
		static constexpr size_t dimensions_n {1};
		using tvec_t	  = tvec<precision_t, 1>;
		using container_t = precision_t;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;
		constexpr tvec(const precision_t& value) noexcept : value(value) {};
		constexpr tvec(precision_t&& value) noexcept : value(std::move(value)) {};

		// ---------------------------------------------
		// operators
		// ---------------------------------------------
		operator container_t() const noexcept { return value; }
		operator container_t&() noexcept { return value; }


		tvec_t& operator=(const container_t& container)
		{
			value = container;
			return *this;
		}

		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_standard_layout<tvec_t>::value && std::is_trivially_copyable_v<tvec_t>,
						  "should remain POD");
			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template <size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}
		template <size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}

		template <typename Y>
		operator tvec<Y, 1>() const noexcept requires std::convertible_to<precision_t, Y>
		{
			return {static_cast<Y>(value[0])};
		}

		constexpr tvec operator%=(const tvec& other) noexcept
		{
			value %= other.value;
			return *this;
		}

		constexpr tvec operator%(const tvec& other) const noexcept
		{
			tvec res {*this};
			res %= other;
			return res;
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		template <size_t new_size>
		tvec<precision_t, new_size> resize() const noexcept
		{
			if constexpr(new_size == dimensions_n)
			{
				return *this;
			}
			else
			{
				tvec<precision_t, new_size> res {};
				constexpr size_t max_copy_size = std::min(dimensions_n, new_size);
				std::copy(std::begin(value), std::next(std::begin(value), max_copy_size), std::begin(res.value));
				return res;
			}
		}

		container_t value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 2>
	{
		static constexpr size_t dimensions_n {2};
		using tvec_t	  = tvec<precision_t, 2>;
		using container_t = psl::static_array<precision_t, 2>;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t up;
		const static tvec_t down;
		const static tvec_t left;
		const static tvec_t right;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;
		constexpr tvec(const precision_t& x, const precision_t& y) noexcept : value({x, y}) {};

		// template <typename precision_t2>
		// constexpr tvec(precision_t2&& x, precision_t2&& y) noexcept
		//	: value({std::forward<precision_t2>(x), std::forward<precision_t2>(y)}){};
		constexpr tvec(const precision_t& value) noexcept : value({value, value}) {};

		// ---------------------------------------------
		// getters
		// ---------------------------------------------
		/*constexpr precision_t& x() noexcept { return value[0]; }
		constexpr const precision_t& x() const noexcept { return value[0]; }
		constexpr precision_t& y() noexcept { return value[1]; }
		constexpr const precision_t& y() const noexcept { return value[1]; }*/

		// ---------------------------------------------
		// operators
		// ---------------------------------------------
		operator container_t() const noexcept { return value; }
		operator container_t&() noexcept { return value; }

		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_standard_layout<tvec_t>::value && std::is_trivially_copyable_v<tvec_t>,
						  "should remain POD");
			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template <size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}
		template <size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}

		template <typename Y>
		operator tvec<Y, 2>() const noexcept requires std::convertible_to<precision_t, Y>
		{
			return {static_cast<Y>(value[0]), static_cast<Y>(value[1])};
		}

		constexpr tvec operator%=(const tvec& other) noexcept
		{
			value[0] %= other.value[0];
			value[1] %= other.value[1];
			return *this;
		}

		constexpr tvec operator%(const tvec& other) const noexcept
		{
			tvec res {*this};
			res %= other;
			return res;
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		template <size_t new_size>
		tvec<precision_t, new_size> resize() const noexcept
		{
			if constexpr(new_size == dimensions_n)
			{
				return *this;
			}
			else
			{
				tvec<precision_t, new_size> res {};
				constexpr size_t max_copy_size = std::min(dimensions_n, new_size);
				std::copy(std::begin(value), std::next(std::begin(value), max_copy_size), std::begin(res.value));
				return res;
			}
		}
		union
		{
			container_t value;
			ACCESSOR_2D(precision_t)
		};
	};

	template <typename precision_t>
	struct tvec<precision_t, 3>
	{
		static constexpr size_t dimensions_n {3};
		using tvec_t	  = tvec<precision_t, 3>;
		using container_t = psl::static_array<precision_t, 3>;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t up;
		const static tvec_t down;
		const static tvec_t left;
		const static tvec_t right;
		const static tvec_t forward;
		const static tvec_t back;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;
		constexpr tvec(const precision_t& x, const precision_t& y, const precision_t& z) noexcept : value({x, y, z}) {};
		constexpr tvec(precision_t&& x, precision_t&& y, precision_t&& z) noexcept :
			value({std::move(x), std::move(y), std::move(z)}) {};
		constexpr tvec(const precision_t& value) noexcept : value({value, value, value}) {};

		constexpr tvec(const tvec<precision_t, 2>& v2, const precision_t& value) noexcept :
			value({v2[0], v2[1], value}) {};

		// ---------------------------------------------
		// getters
		// ---------------------------------------------
		/*constexpr precision_t& x() noexcept { return value[0]; }
		constexpr const precision_t& x() const noexcept { return value[0]; }
		constexpr precision_t& y() noexcept { return value[1]; }
		constexpr const precision_t& y() const noexcept { return value[1]; }
		constexpr precision_t& z() noexcept { return value[2]; }
		constexpr const precision_t& z() const noexcept { return value[2]; }*/

		// ---------------------------------------------
		// operators
		// ---------------------------------------------
		operator container_t() const noexcept { return value; }
		operator container_t&() noexcept { return value; }

		tvec_t& operator=(const container_t& container)
		{
			value = container;
			return *this;
		}

		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_standard_layout<tvec_t>::value && std::is_trivially_copyable_v<tvec_t>,
						  "should remain POD");
			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template <size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}
		template <size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}

		template <typename Y>
		operator tvec<Y, 3>() const noexcept requires std::convertible_to<precision_t, Y>
		{
			return {static_cast<Y>(value[0]), static_cast<Y>(value[1]), static_cast<Y>(value[2])};
		}

		constexpr tvec operator%=(const tvec& other) noexcept
		{
			value[0] %= other.value[0];
			value[1] %= other.value[1];
			value[2] %= other.value[2];
			return *this;
		}

		constexpr tvec operator%(const tvec& other) const noexcept
		{
			tvec res {*this};
			res %= other;
			return res;
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		template <size_t new_size>
		tvec<precision_t, new_size> resize() const noexcept
		{
			if constexpr(new_size == dimensions_n)
			{
				return *this;
			}
			else
			{
				tvec<precision_t, new_size> res {};
				constexpr size_t max_copy_size = std::min(dimensions_n, new_size);
				std::copy(std::begin(value), std::next(std::begin(value), max_copy_size), std::begin(res.value));
				return res;
			}
		}
		union
		{
			container_t value;
			ACCESSOR_3D(precision_t)
		};
	};

	// todo alignas should be handled more gracefully
	template <typename precision_t>
	struct alignas(16) tvec<precision_t, 4>
	{
		static constexpr size_t dimensions_n {4};
		using tvec_t	  = tvec<precision_t, 4>;
		using container_t = psl::static_array<precision_t, 4>;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t up;
		const static tvec_t down;
		const static tvec_t left;
		const static tvec_t right;
		const static tvec_t forward;
		const static tvec_t back;
		const static tvec_t position;
		const static tvec_t direction;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;
		constexpr tvec(const precision_t& x, const precision_t& y, const precision_t& z, const precision_t& w) noexcept
			:
			value({x, y, z, w}) {};
		constexpr tvec(precision_t&& x, precision_t&& y, precision_t&& z, precision_t&& w) noexcept :
			value({std::move(x), std::move(y), std::move(z), std::move(w)}) {};
		constexpr tvec(const precision_t& value) noexcept : value({value, value, value, value}) {};

		constexpr tvec(const tvec<precision_t, 2>& a, const tvec<precision_t, 2>& b) noexcept :
			value({a[0], a[1], b[0], b[1]}) {};
		constexpr tvec(const tvec<precision_t, 3>& v3, const precision_t& value) noexcept :
			value({v3[0], v3[1], v3[2], value}) {};

		template <typename src_precision_t>
		constexpr tvec(psl::static_array<src_precision_t, dimensions_n>&& arr) noexcept : value({std::move(arr)}) {};

		template <typename src_precision_t>
		constexpr tvec(const psl::static_array<src_precision_t, dimensions_n>& value) noexcept : value({value}) {};

		// ---------------------------------------------
		// getters
		// ---------------------------------------------
		/*constexpr precision_t& x() noexcept { return value[0]; }
		constexpr const precision_t& x() const noexcept { return value[0]; }
		constexpr precision_t& y() noexcept { return value[1]; }
		constexpr const precision_t& y() const noexcept { return value[1]; }
		constexpr precision_t& z() noexcept { return value[2]; }
		constexpr const precision_t& z() const noexcept { return value[2]; }
		constexpr precision_t& w() noexcept { return value[3]; }
		constexpr const precision_t& w() const noexcept { return value[3]; }*/

		// ---------------------------------------------
		// operators
		// ---------------------------------------------
		operator container_t() const noexcept { return value; }
		operator container_t&() noexcept { return value; }

		tvec_t& operator=(const container_t& container)
		{
			value = container;
			return *this;
		}

		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_standard_layout<tvec_t>::value && std::is_trivially_copyable_v<tvec_t>,
						  "should remain POD");
			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template <size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}
		template <size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < dimensions_n, "out of range");
			return value.at(index);
		}

		template <typename Y>
		operator tvec<Y, 4>() const noexcept requires std::convertible_to<precision_t, Y>
		{
			return {
			  static_cast<Y>(value[0]), static_cast<Y>(value[1]), static_cast<Y>(value[2]), static_cast<Y>(value[3])};
		}

		constexpr tvec operator%=(const tvec& other) noexcept
		{
			value[0] %= other.value[0];
			value[1] %= other.value[1];
			value[2] %= other.value[2];
			value[3] %= other.value[3];
			return *this;
		}

		constexpr tvec operator%(const tvec& other) const noexcept
		{
			tvec res {*this};
			res %= other;
			return res;
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		template <size_t new_size>
		tvec<precision_t, new_size> resize() const noexcept
		{
			if constexpr(new_size == dimensions_n)
			{
				return *this;
			}
			else
			{
				tvec<precision_t, new_size> res {};
				constexpr size_t max_copy_size = std::min(dimensions_n, new_size);
				std::copy(std::begin(value), std::next(std::begin(value), max_copy_size), std::begin(res.value));
				return res;
			}
		}

		union
		{
			container_t value;
			ACCESSOR_4D(precision_t)
		};
	};

#pragma warning(pop)

	using vec1 = psl::tvec<float, 1>;
	using vec2 = psl::tvec<float, 2>;
	using vec3 = psl::tvec<float, 3>;
	using vec4 = psl::tvec<float, 4>;

	using dvec1 = psl::tvec<double, 1>;
	using dvec2 = psl::tvec<double, 2>;
	using dvec3 = psl::tvec<double, 3>;
	using dvec4 = psl::tvec<double, 4>;

	using ivec1 = psl::tvec<int, 1>;
	using ivec2 = psl::tvec<int, 2>;
	using ivec3 = psl::tvec<int, 3>;
	using ivec4 = psl::tvec<int, 4>;

	using vec1_sz = psl::tvec<size_t, 1>;
	using vec2_sz = psl::tvec<size_t, 2>;
	using vec3_sz = psl::tvec<size_t, 3>;
	using vec4_sz = psl::tvec<size_t, 4>;


	template <typename precision_t, size_t dimensions>
	const tvec<precision_t, dimensions> tvec<precision_t, dimensions>::zero {0};
	template <typename precision_t, size_t dimensions>
	const tvec<precision_t, dimensions> tvec<precision_t, dimensions>::one {1};
	template <typename precision_t, size_t dimensions>
	const tvec<precision_t, dimensions> tvec<precision_t, dimensions>::infinity {
	  std::numeric_limits<precision_t>::infinity()};
	template <typename precision_t, size_t dimensions>
	const tvec<precision_t, dimensions> tvec<precision_t, dimensions>::negative_infinity {
	  -std::numeric_limits<precision_t>::infinity()};


	template <typename precision_t>
	const tvec<precision_t, 2> tvec<precision_t, 2>::zero {0};
	template <typename precision_t>
	const tvec<precision_t, 2> tvec<precision_t, 2>::one {1};
	template <typename precision_t>
	const tvec<precision_t, 2> tvec<precision_t, 2>::infinity {std::numeric_limits<precision_t>::infinity()};
	template <typename precision_t>
	const tvec<precision_t, 2> tvec<precision_t, 2>::negative_infinity {-std::numeric_limits<precision_t>::infinity()};
	template <typename precision_t>
	const tvec<precision_t, 2> tvec<precision_t, 2>::up {0, 1};
	template <typename precision_t>
	const tvec<precision_t, 2> tvec<precision_t, 2>::down {0, -1};
	template <typename precision_t>
	const tvec<precision_t, 2> tvec<precision_t, 2>::left {-1, 0};
	template <typename precision_t>
	const tvec<precision_t, 2> tvec<precision_t, 2>::right {1, 0};

	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::zero {0};
	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::one {1};
	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::infinity {std::numeric_limits<precision_t>::infinity()};
	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::negative_infinity {-std::numeric_limits<precision_t>::infinity()};
	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::up {0, 1, 0};
	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::down {0, -1, 0};
	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::left {-1, 0, 0};
	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::right {1, 0, 0};
	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::forward {0, 0, 1};
	template <typename precision_t>
	const tvec<precision_t, 3> tvec<precision_t, 3>::back {0, 0, -1};

	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::zero {0};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::one {1};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::infinity {std::numeric_limits<precision_t>::infinity()};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::negative_infinity {-std::numeric_limits<precision_t>::infinity()};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::up {0, 1, 0, 0};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::down {0, -1, 0, 0};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::left {-1, 0, 0, 0};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::right {1, 0, 0, 0};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::forward {0, 0, 1, 0};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::back {0, 0, -1, 0};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::position {0, 0, 0, 1};
	template <typename precision_t>
	const tvec<precision_t, 4> tvec<precision_t, 4>::direction {0};
}	 // namespace psl


namespace psl
{
	// ---------------------------------------------
	// operators tvec<precision_t, 1>
	// ---------------------------------------------
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator+=(tvec<precision_t, 1>& owner, const tvec<precision_t, 1>& other) noexcept
	{
		owner.value += other.value;
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator*=(tvec<precision_t, 1>& owner, const tvec<precision_t, 1>& other) noexcept
	{
		owner.value *= other.value;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator/=(tvec<precision_t, 1>& owner, const tvec<precision_t, 1>& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if(other.value[0] == 0) throw std::runtime_exception("division by 0");
#endif
		owner.value /= other.value;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator*=(tvec<precision_t, 1>& owner, const precision_t& other) noexcept
	{
		owner.value *= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator/=(tvec<precision_t, 1>& owner, const precision_t& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if(other.value[0] == 0) throw std::runtime_exception("division by 0");
#endif
		owner.value /= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator-=(tvec<precision_t, 1>& owner, const tvec<precision_t, 1>& other) noexcept
	{
		owner.value -= other.value;
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator+(const tvec<precision_t, 1>& left,
											 const tvec<precision_t, 1>& right) noexcept
	{
		auto cpy = left;
		cpy.value += right.value;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator*(const tvec<precision_t, 1>& left,
											 const tvec<precision_t, 1>& right) noexcept
	{
		auto cpy = left;
		cpy.value *= right.value;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator/(const tvec<precision_t, 1>& left,
											 const tvec<precision_t, 1>& right) noexcept
	{
		auto cpy = left;
		cpy.value /= right.value;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator*(const tvec<precision_t, 1>& left, const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator/(const tvec<precision_t, 1>& left, const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value /= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator-(const tvec<precision_t, 1>& left,
											 const tvec<precision_t, 1>& right) noexcept
	{
		auto cpy = left;
		cpy.value -= right.value;
		return cpy;
	}

	template <typename precision_t>
	constexpr bool operator==(const tvec<precision_t, 1>& left, const tvec<precision_t, 1>& right) noexcept
	{
		return left[0] == right[0];
	}

	template <typename precision_t>
	constexpr bool operator!=(const tvec<precision_t, 1>& left, const tvec<precision_t, 1>& right) noexcept
	{
		return left[0] != right[0];
	}

	// ---------------------------------------------
	// operators tvec<precision_t, 2>
	// ---------------------------------------------
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator+=(tvec<precision_t, 2>& owner, const tvec<precision_t, 2>& other) noexcept
	{
		owner.value[0] += other.value[0];
		owner.value[1] += other.value[1];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator*=(tvec<precision_t, 2>& owner, const tvec<precision_t, 2>& other) noexcept
	{
		owner.value[0] *= other.value[0];
		owner.value[1] *= other.value[1];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator/=(tvec<precision_t, 2>& owner, const tvec<precision_t, 2>& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if(other.value[0] == 0 || other.value[1] == 0) throw std::runtime_exception("division by 0");
#endif
		owner.value[0] /= other.value[0];
		owner.value[1] /= other.value[1];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator*=(tvec<precision_t, 2>& owner, const precision_t& other) noexcept
	{
		owner.value[0] *= other;
		owner.value[1] *= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator/=(tvec<precision_t, 2>& owner, const precision_t& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if(other.value[0] == 0 || other.value[1] == 0) throw std::runtime_exception("division by 0");
#endif
		owner.value[0] /= other;
		owner.value[1] /= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator-=(tvec<precision_t, 2>& owner, const tvec<precision_t, 2>& other) noexcept
	{
		owner.value[0] -= other.value[0];
		owner.value[1] -= other.value[1];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator+(const tvec<precision_t, 2>& left,
											 const tvec<precision_t, 2>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] += right.value[0];
		cpy.value[1] += right.value[1];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator*(const tvec<precision_t, 2>& left,
											 const tvec<precision_t, 2>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] *= right.value[0];
		cpy.value[1] *= right.value[1];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator/(const tvec<precision_t, 2>& left,
											 const tvec<precision_t, 2>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] /= right.value[0];
		cpy.value[1] /= right.value[1];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator*(const tvec<precision_t, 2>& left, const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] *= right;
		cpy.value[1] *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator/(const tvec<precision_t, 2>& left, const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] /= right;
		cpy.value[1] /= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator-(const tvec<precision_t, 2>& left,
											 const tvec<precision_t, 2>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] -= right.value[0];
		cpy.value[1] -= right.value[1];
		return cpy;
	}

	template <typename precision_t>
	constexpr bool operator==(const tvec<precision_t, 2>& left, const tvec<precision_t, 2>& right) noexcept
	{
		return left[0] == right[0] && left[1] == right[1];
	}

	template <typename precision_t>
	constexpr bool operator!=(const tvec<precision_t, 2>& left, const tvec<precision_t, 2>& right) noexcept
	{
		return left[0] != right[0] || left[1] != right[1];
	}

	// ---------------------------------------------
	// operators tvec<precision_t, 3>
	// ---------------------------------------------
	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator+=(tvec<precision_t, 3>& owner, const tvec<precision_t, 3>& other) noexcept
	{
		owner.value[0] += other.value[0];
		owner.value[1] += other.value[1];
		owner.value[2] += other.value[2];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator*=(tvec<precision_t, 3>& owner, const tvec<precision_t, 3>& other) noexcept
	{
		owner.value[0] *= other.value[0];
		owner.value[1] *= other.value[1];
		owner.value[2] *= other.value[2];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator/=(tvec<precision_t, 3>& owner, const tvec<precision_t, 3>& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if(other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0)
			throw std::runtime_exception("division by 0");
#endif
		owner.value[0] /= other.value[0];
		owner.value[1] /= other.value[1];
		owner.value[2] /= other.value[2];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator*=(tvec<precision_t, 3>& owner, const precision_t& other) noexcept
	{
		owner.value[0] *= other;
		owner.value[1] *= other;
		owner.value[2] *= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator/=(tvec<precision_t, 3>& owner, const precision_t& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if(other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0)
			throw std::runtime_exception("division by 0");
#endif
		owner.value[0] /= other;
		owner.value[1] /= other;
		owner.value[2] /= other;
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator-=(tvec<precision_t, 3>& owner, const tvec<precision_t, 3>& other) noexcept
	{
		owner.value[0] -= other.value[0];
		owner.value[1] -= other.value[1];
		owner.value[2] -= other.value[2];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator+(const tvec<precision_t, 3>& left,
											 const tvec<precision_t, 3>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] += right.value[0];
		cpy.value[1] += right.value[1];
		cpy.value[2] += right.value[2];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator*(const tvec<precision_t, 3>& left,
											 const tvec<precision_t, 3>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] *= right.value[0];
		cpy.value[1] *= right.value[1];
		cpy.value[2] *= right.value[2];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator/(const tvec<precision_t, 3>& left,
											 const tvec<precision_t, 3>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] /= right.value[0];
		cpy.value[1] /= right.value[1];
		cpy.value[2] /= right.value[2];
		return cpy;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator*(const tvec<precision_t, 3>& left, const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] *= right;
		cpy.value[1] *= right;
		cpy.value[2] *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator/(const tvec<precision_t, 3>& left, const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] /= right;
		cpy.value[1] /= right;
		cpy.value[2] /= right;
		return cpy;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator-(const tvec<precision_t, 3>& left,
											 const tvec<precision_t, 3>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] -= right.value[0];
		cpy.value[1] -= right.value[1];
		cpy.value[2] -= right.value[2];
		return cpy;
	}

	template <typename precision_t>
	constexpr bool operator==(const tvec<precision_t, 3>& left, const tvec<precision_t, 3>& right) noexcept
	{
		return left[0] == right[0] && left[1] == right[1] && left[2] == right[2];
	}

	template <typename precision_t>
	constexpr bool operator!=(const tvec<precision_t, 3>& left, const tvec<precision_t, 3>& right) noexcept
	{
		return left[0] != right[0] || left[1] != right[1] || left[2] != right[2];
	}

	// ---------------------------------------------
	// operators tvec<precision_t, 4>
	// ---------------------------------------------

	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator*=(tvec<precision_t, 4>& owner, const precision_t& other) noexcept
	{
		owner.value[0] *= other;
		owner.value[1] *= other;
		owner.value[2] *= other;
		owner.value[3] *= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator/=(tvec<precision_t, 4>& owner, const precision_t& other) noexcept
	{
		owner.value[0] /= other;
		owner.value[1] /= other;
		owner.value[2] /= other;
		owner.value[3] /= other;
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator+(const tvec<precision_t, 4>& left,
											 const tvec<precision_t, 4>& right) noexcept
	{
		auto cpy = left;
		cpy += right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator*(const tvec<precision_t, 4>& left,
											 const tvec<precision_t, 4>& right) noexcept
	{
		auto cpy = left;
		cpy *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator/(const tvec<precision_t, 4>& left,
											 const tvec<precision_t, 4>& right) noexcept
	{
		auto cpy = left;
		cpy /= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator*(const tvec<precision_t, 4>& left, const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator/(const tvec<precision_t, 4>& left, const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy /= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator-(const tvec<precision_t, 4>& left,
											 const tvec<precision_t, 4>& right) noexcept
	{
		auto cpy = left;
		cpy -= right;
		return cpy;
	}


	template <typename precision_t>
	constexpr bool operator==(const tvec<precision_t, 4>& left, const tvec<precision_t, 4>& right) noexcept
	{
		return left[0] == right[0] && left[1] == right[1] && left[2] == right[2] && left[3] == right[3];
	}

	template <typename precision_t>
	constexpr bool operator!=(const tvec<precision_t, 4>& left, const tvec<precision_t, 4>& right) noexcept
	{
		return left[0] != right[0] || left[1] != right[1] || left[2] != right[2] || left[3] != right[3];
	}

	// ---------------------------------------------
	// operators tvec<precision_t, N>
	// ---------------------------------------------

	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator-(const tvec<precision_t, dimensions>& owner) noexcept
	{
		auto cpy = owner;
		for(size_t i = 0; i < dimensions; ++i) cpy.value[i] = -cpy.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator+=(tvec<precision_t, dimensions>& owner,
														const tvec<precision_t, dimensions>& other) noexcept
	{
		for(size_t i = 0; i < dimensions; ++i) owner.value[i] += other.value[i];
		return owner;
	}

	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator*=(tvec<precision_t, dimensions>& owner,
														const tvec<precision_t, dimensions>& other) noexcept
	{
		for(size_t i = 0; i < dimensions; ++i) owner.value[i] *= other.value[i];
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator/=(tvec<precision_t, dimensions>& owner,
														const tvec<precision_t, dimensions>& other) noexcept
	{
		for(size_t i = 0; i < dimensions; ++i)
		{
#ifdef MATH_DIV_ZERO_CHECK
			if(other.value[i] == 0) throw std::runtime_exception("division by 0");
#endif
			owner.value[i] /= other.value[i];
		}
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator*=(tvec<precision_t, dimensions>& owner,
														const precision_t& other) noexcept
	{
		for(size_t i = 0; i < dimensions; ++i) owner.value[i] *= other;
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator/=(tvec<precision_t, dimensions>& owner,
														const precision_t& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if(other == 0) throw std::runtime_exception("division by 0");
#endif
		for(size_t i = 0; i < dimensions; ++i)
		{
			owner.value[i] /= other;
		}
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator-=(tvec<precision_t, dimensions>& owner,
														const tvec<precision_t, dimensions>& other) noexcept
	{
		for(size_t i = 0; i < dimensions; ++i) owner.value[i] -= other.value[i];
		return owner;
	}

	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator+(const tvec<precision_t, dimensions>& left,
													  const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for(size_t i = 0; i < dimensions; ++i) cpy.value[i] += right.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator*(const tvec<precision_t, dimensions>& left,
													  const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for(size_t i = 0; i < dimensions; ++i) cpy.value[i] *= right.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator/(const tvec<precision_t, dimensions>& left,
													  const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for(size_t i = 0; i < dimensions; ++i) cpy.value[i] /= right.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator-(const tvec<precision_t, dimensions>& left,
													  const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for(size_t i = 0; i < dimensions; ++i) cpy.value[i] -= right.value[i];
		return cpy;
	}


	template <typename precision_t, size_t dimensions>
	constexpr bool operator==(const tvec<precision_t, dimensions>& left,
							  const tvec<precision_t, dimensions>& right) noexcept
	{
		return std::memcmp(left.value.data(), right.value.data(), dimensions * sizeof(precision_t)) == 0;
	}

	template <typename precision_t, size_t dimensions>
	constexpr bool operator!=(const tvec<precision_t, dimensions>& left,
							  const tvec<precision_t, dimensions>& right) noexcept
	{
		return std::memcmp(left.value.data(), right.value.data(), dimensions * sizeof(precision_t)) != 0;
	}

}	 // namespace psl

namespace psl::math
{
	// ---------------------------------------------
	// operations between tvec<precision_t, N>
	// ---------------------------------------------
	template <typename precision_t, size_t dimensions>
	constexpr static precision_t dot(const tvec<precision_t, dimensions>& left,
									 const tvec<precision_t, dimensions>& right) noexcept
	{
		precision_t res = precision_t {0};
		for(size_t i = 0; i < dimensions; ++i) res += left[i] * right[i];
		return res;
	}

	template <typename precision_t, size_t dimensions>
	constexpr static precision_t compound(const tvec<precision_t, dimensions>& vec) noexcept
	{
		precision_t res = precision_t {vec[0]};
		for(size_t i = 1; i < dimensions; ++i) res += vec[i];
		return res;
	}

	template <typename precision_t, size_t dimensions>
	constexpr static precision_t compound_mul(const tvec<precision_t, dimensions>& vec) noexcept
	{
		precision_t res = precision_t {vec[0]};
		for(size_t i = 1; i < dimensions; ++i) res *= vec[i];
		return res;
	}

	template <typename precision_t, size_t dimensions>
	constexpr static precision_t square_magnitude(const tvec<precision_t, dimensions>& vec) noexcept
	{
		precision_t res = precision_t {0};
		for(size_t i = 0; i < dimensions; ++i) res += vec[i] * vec[i];
		return res;
	}

	template <typename precision_t, size_t dimensions>
	constexpr static precision_t magnitude(const tvec<precision_t, dimensions>& vec) noexcept
	{
		precision_t res = precision_t {0};
		for(size_t i = 0; i < dimensions; ++i) res += vec[i] * vec[i];
		return std::sqrt(res);
	}

	template <typename precision_t, size_t dimensions>
	constexpr static precision_t length(const tvec<precision_t, dimensions>& vec) noexcept
	{
		return std::sqrt(dot(vec, vec));
	}

	template <typename precision_t, size_t dimensions>
	constexpr static tvec<precision_t, dimensions> sqrt(const tvec<precision_t, dimensions>& vec) noexcept
	{
		tvec<precision_t, dimensions> res {};
		for(size_t i = 0; i < dimensions; ++i)
		{
			res[i] = std::sqrt(vec[i]);
		}
		return res;
	}

	template <typename precision_t, size_t dimensions>
	constexpr static tvec<precision_t, dimensions> pow(const tvec<precision_t, dimensions>& vec,
													   precision_t pow_value = precision_t {2}) noexcept
	{
		tvec<precision_t, dimensions> res {};
		for(size_t i = 0; i < dimensions; ++i)
		{
			res[i] = std::pow(vec[i], pow_value);
		}
		return res;
		{}
	}

	template <typename precision_t, size_t dimensions>
	constexpr static tvec<precision_t, dimensions> normalize(const tvec<precision_t, dimensions>& vec) noexcept
	{
		return vec / magnitude(vec);
	}

	template <typename precision_t>
	constexpr static tvec<precision_t, 3> cross(const tvec<precision_t, 3>& left,
												const tvec<precision_t, 3>& right) noexcept
	{
		return tvec<precision_t, 3> {left[1] * right[2] - left[2] * right[1],
									 left[2] * right[0] - left[0] * right[2],
									 left[0] * right[1] - left[1] * right[0]};
	}
}	 // namespace psl::math

#include "psl/math/AVX/vec.h"
#include "psl/math/AVX2/vec.h"
#include "psl/math/SSE/vec.h"
#include "psl/math/fallback/vec.h"
