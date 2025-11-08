#pragma once

#include <initializer_list>
#include <utility>
#include <iterator>
#include <type_traits>

#include "helper.hxx"
#include "Position.hxx"

template <typename T, size_t N>
class static_vector {
    T array[N];
    size_t _size{};

public:
    constexpr static_vector() noexcept = default;
    constexpr static_vector(std::initializer_list<T> it) noexcept(std::is_nothrow_copy_assignable_v<T>)
    : _size{it.size()}
    {
        for (const auto& [i, e] : enumerate(it)) array[i] = e;
    }


    constexpr void push_back(const T& e) noexcept(std::is_nothrow_copy_assignable_v<T>) { array[_size++] = e; }
    constexpr void push_back(T&& e) noexcept(std::is_nothrow_move_assignable_v<T>) { array[_size++] = std::move(e); }

    constexpr void pop_back() noexcept { --_size; }

    constexpr T& operator[](const size_t i) { return array[i]; }
    constexpr const T& operator[](const size_t i) const noexcept { return array[i]; }
    constexpr size_t size() const noexcept { return _size; }

    constexpr auto begin() noexcept { return array; }
    constexpr auto end() noexcept { return std::next(array, _size); }
    constexpr auto begin() const noexcept { return cbegin(); }
    constexpr auto end() const noexcept { return cend(); }

    constexpr auto cbegin() const noexcept { return array; }
    constexpr auto cend() const noexcept { return std::next(array, _size); }
};



struct Move {
    static_vector<Position, 16> positions;
    static_vector<Position, 16> captures;


    constexpr bool isValid() const noexcept { return positions.size(); }
    constexpr bool doesCapture() const noexcept { return  captures.size(); }
};

