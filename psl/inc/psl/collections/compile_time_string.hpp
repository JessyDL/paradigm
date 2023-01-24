#pragma once
#include <compare>
#include <cstdint>
#include <string_view>

namespace psl {
template <std::size_t N>
struct ct_string;

inline namespace _details {
	template <typename T>
	struct is_ct_string : std::false_type {};
	template <std::size_t N>
	struct is_ct_string<ct_string<N>> : std::true_type {};
}	 // namespace _details
template <typename T>
concept IsCTString = is_ct_string<std::remove_cvref_t<T>>::value;

template <ct_string Str>
struct ct_string_wrapper;

template <std::size_t N>
struct ct_string {
	char buf[N + 1] {};
	constexpr ct_string(char const* s) {
		for(std::size_t i = 0; i != N; ++i) buf[i] = s[i];
	}

	template <auto Str>
	constexpr ct_string(ct_string_wrapper<Str>) {
		for(std::size_t i = 0; i != N; ++i) buf[i] = Str[i];
	}
	auto operator<=>(const ct_string&) const = default;

	constexpr char operator[](std::size_t index) const noexcept { return buf[index]; }

	constexpr operator std::string_view() const noexcept { return std::string_view {buf, N}; }
	constexpr std::string_view view() const noexcept { return std::string_view {buf, N}; }
	constexpr operator char const*() const { return buf; }

	constexpr std::size_t size() const noexcept { return N; }

	template <std::size_t start, std::size_t end>
	consteval ct_string<end - start> substr() const noexcept {
		static_assert(start <= end);
		static_assert(end <= N + 1);
		return ct_string<end - start> {&buf[start]};
	}

	constexpr auto begin() const noexcept { return &buf[0]; }
	constexpr auto cbegin() const noexcept { return &buf[0]; }
	constexpr auto end() const noexcept { return &buf[N]; }
	constexpr auto cend() const noexcept { return &buf[N]; }
};
template <unsigned N>
ct_string(char const (&)[N]) -> ct_string<N - 1>;

template <ct_string Str>
struct ct_string_wrapper {
	static constexpr auto value {Str};
};

template <auto Str>
ct_string(ct_string_wrapper<Str>) -> ct_string<Str.size()>;

template <ct_string Str>
constexpr ct_string_wrapper<Str> operator""_ctstr() {
	return {};
}
}	 // namespace psl
