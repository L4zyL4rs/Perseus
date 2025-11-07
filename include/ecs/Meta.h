#pragma once
#include <utility>
#include <type_traits>
#include <array>

//-------------------------------------------
// Metaprogramming utilities
//-------------------------------------------
namespace meta {

	template <typename U, typename... Ts>
	consteval std::size_t indexOfType() {
		static_assert((std::is_same_v<U, Ts> || ...), "Type not found in parameter pack!");

		constexpr bool matches[] = { std::is_same_v<U, Ts>... };

		for (std::size_t i = 0; i < sizeof...(Ts); ++i) {
			if (matches[i]) { return i; }
		}

		return static_cast<std::size_t>(-1);
	}

	template <typename F, typename... Ts, std::size_t... I>
	void forEachTypeImpl(F&& f, std::index_sequence<I...>) {
		(f(std::integral_constant<std::size_t, I>{}), ...);
	}

	template <typename... Ts, typename F>
	void forEachType(F&& f) {
		forEachTypeImpl(
			std::forward<F>(f),
			std::make_index_sequence<sizeof...(Ts)> {}
		);

	}

	template <typename... Ts, typename F>
	auto make_tuple_from_pack(F&& f) {
		auto tuple = std::make_tuple(f(std::type_identity<Ts>())...);
		return tuple;
	}
}