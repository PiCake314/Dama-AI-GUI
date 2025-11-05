#pragma once


constexpr struct Position {
    int x{}, y{}; // int instead of, say, size_t to save on size and allow negative values. Maybe int8_t could work


    bool operator==(const Position&) const noexcept = default;
} NULL_POSITION{-1, -1};

