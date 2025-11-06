#pragma once


struct Position {
    int x{}, y{}; // int instead of, say, size_t to save on size and allow negative values. Maybe int8_t could work

    constexpr Position() noexcept { x = -1, y = -1; }
    constexpr Position(int xx, int yy) noexcept : x{xx}, y{yy} {}

    constexpr Position operator-() const noexcept { return {-x, -y}; }

    constexpr Position operator+(const Position other) const noexcept { return {x + other.x, y + other.y}; }
    constexpr Position operator-(const Position other) const noexcept { return *this + -other; }

    bool operator==(const Position&) const noexcept = default;
};

