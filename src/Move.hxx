#pragma once

#include <array>
#include "Position.hxx"

constexpr struct Move {
    std::array<Position, 16> positions = {NULL_POSITION};
    std::array<Position, 16> captures = {NULL_POSITION};

    bool doesEat() const noexcept { return captures[0] != NULL_POSITION; }
} NULL_MOVE;

