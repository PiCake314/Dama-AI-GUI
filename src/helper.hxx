#pragma once


#include <ranges>
#include <concepts>

constexpr auto range(std::integral auto limit) { return std::views::iota(0, limit); }
