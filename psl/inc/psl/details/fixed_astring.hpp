#pragma once
#include <compare>
#include <cstdint>
#include <string_view>

namespace psl {
namespace details {
	template <size_t N>
	struct fixed_astring;

	template <typename T>
	struct is_fixed_astring : std::false_type {};
	template <size_t N>
	struct is_fixed_astring<fixed_astring<N>> : std::true_type {};

	template <typename T>
	concept IsFixedAstring = is_fixed_astring<std::remove_cvref_t<T>>::value;

	template <size_t N>
	struct fixed_astring {
		char buf[N + 1] {};

		consteval fixed_astring() = default;
		consteval fixed_astring(char const* s) {
			for(size_t i = 0; i != N; ++i) buf[i] = s[i];
		}
		auto operator<=>(const fixed_astring&) const = default;

		constexpr char operator[](size_t index) const noexcept {
			return buf[index];
		}

		constexpr operator std::string_view() const noexcept {
			return std::string_view {buf, N};
		}
		constexpr operator char const*() const {
			return buf;
		}

		constexpr size_t size() const noexcept {
			return N;
		}

		template <size_t start, size_t end>
		consteval fixed_astring<end - start> substr() const noexcept {
			static_assert(start <= end);
			static_assert(end <= N + 1);
			return fixed_astring<end - start> {&buf[start]};
		}

		constexpr auto begin() const noexcept {
			return &buf[0];
		}
		constexpr auto cbegin() const noexcept {
			return &buf[0];
		}
		constexpr auto end() const noexcept {
			return &buf[N];
		}
		constexpr auto cend() const noexcept {
			return &buf[N];
		}

		constexpr auto data() const noexcept {
			return &buf[0];
		}

		template <size_t M>
		consteval fixed_astring<N + M> operator+(fixed_astring<M> const& other) const noexcept {
			fixed_astring<N + M> result {};
			for(size_t i = 0; i != N; ++i) result.buf[i] = buf[i];
			for(size_t i = 0; i != M; ++i) result.buf[N + i] = other.buf[i];
			return result;
		}
	};
	template <unsigned N>
	fixed_astring(char const (&)[N]) -> fixed_astring<N - 1>;
}	 // namespace details
}	 // namespace psl
