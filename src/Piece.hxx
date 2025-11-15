#pragma once

#include <print>
#include <iostream>
#include <type_traits>

#include <SFML/Graphics.hpp>

#include "Position.hxx"



constexpr sf::Color AWAY_COLOR  = {32, 20 ,  0};
constexpr sf::Color HOME_COLOR  = {204, 126, 0};
// constexpr sf::Color CROWN_COLOR = 


struct Piece {
    // assume black, undragged, pawn if active
    enum Flags : unsigned char {
        NONE        = 0b0000,

        ACTIVE      = 0b1000,
        YELLOW      = 0b0001,
        SHAIKH      = 0b0010,
        DRAGGED     = 0b0100,
    } flags : 4 = Flags::NONE;


    constexpr Piece(Flags f) noexcept : flags{f} {}
    constexpr Piece& operator=(const Flags& f) noexcept { flags = f; return *this; }

    // rule of 5,, just to be safe
    constexpr Piece() noexcept = default;
    constexpr Piece(const Piece&) noexcept = default;
    constexpr Piece(Piece&&) noexcept = default;
    constexpr Piece& operator=(const Piece&) noexcept = default;
    constexpr Piece& operator=(Piece&&) noexcept = default;


    constexpr bool isActive() const noexcept { return flags & Flags::ACTIVE;      }
    constexpr operator bool() const noexcept { return isActive(); }
    constexpr operator char() const noexcept {
        if (isActive()) {
            if (isYellow()) return isShaikh() ? 'Y' : 'y';
            else return isShaikh() ? 'B' : 'b';
        }
        else return ' ';
    }

    // should probably add a isActive check but for now we're gonna assume it is!
    constexpr bool isYellow()     const noexcept { return flags & Flags::YELLOW;      }
    constexpr bool isBlack()      const noexcept { return not isYellow();             }
    constexpr bool isPawn()       const noexcept { return not isShaikh();             }
    constexpr bool isShaikh()     const noexcept { return flags & Flags::SHAIKH;      }
    constexpr bool isDragged()    const noexcept { return flags & Flags::DRAGGED;     }

    constexpr void promote() noexcept { flags = flags | Flags::SHAIKH; }
    constexpr void reset() noexcept { flags = Flags::NONE; }

    constexpr void setDragged(const bool d) noexcept {
        if (d) flags = flags |  Flags::DRAGGED;
        else   flags = flags & ~Flags::DRAGGED;
    }


    sf::CircleShape getShape(sf::RenderWindow& window, const sf::Vector2f pos, const bool dragging = false) const {
        const auto winsize = window.getSize();
        const float radius = winsize.x / 20; // / 10 / 2

        sf::CircleShape circle{radius};
        //  32, 20, 0
        circle.setFillColor(isYellow() ? HOME_COLOR : AWAY_COLOR);
        circle.setOrigin({radius, radius});


        if (dragging) circle.setPosition(pos);
        else {
            const float block_size = winsize.x / 8;
            const float half_block_size = winsize.x / 16;
            circle.setPosition({pos.x * block_size + half_block_size, pos.y * block_size + half_block_size});
        }

        return circle;
    }

    void render(sf::RenderWindow& window, sf::Vector2f pos, const bool dragging = false) const {
        // std::clog << "is active? " << (isActive() ? "yes\n" : "no\n");
        // std::println(std::clog, "{:b}", int(flags));
        if (not isActive()) return;

        const auto circle = getShape(window, pos, dragging);

        window.draw(circle);

        if (isShaikh()) {

            if (dragging) {
                crown.setPosition(pos);
            }
            else {
                const auto winsize = window.getSize();
                const float block_size = winsize.x / 8;
                const float half_block_size = winsize.x / 16;
                crown.setPosition({pos.x * block_size + half_block_size, pos.y * block_size + half_block_size});
            }

            window.draw(crown);
        }
    }


    // Crown can be static but not circle is because circle technically depends on the size of the window!
    const static inline sf::Texture CROWN_IMAGE{"src/assets/crown_brown.png", false, sf::IntRect{{0, 0}, {512, 512}}};
    static inline sf::Sprite crown = [] {
        sf::Sprite crown{CROWN_IMAGE};
        crown.setOrigin({256, 256});
        crown.setScale({.1, .1});
        return crown;
    }();




    void prettyPrint() const {
        if (not isActive()) {
            std::clog << ' ';
        }
        else if (isYellow()) {
            std::clog << (isShaikh() ? 'Y' : 'y');
        }
        else {
            std::clog << (isShaikh() ? 'B' : 'b');
        }
    }

    constexpr Piece operator&(const Piece f) const noexcept {
        return static_cast<Flags>(static_cast<std::underlying_type_t<Piece::Flags>>(flags) & static_cast<std::underlying_type_t<Piece::Flags>>(f.flags));
    }
    constexpr Piece operator&(const Flags f) const noexcept {
        return static_cast<Flags>(static_cast<std::underlying_type_t<Piece::Flags>>(flags) & static_cast<std::underlying_type_t<Piece::Flags>>(f));
    }
    constexpr friend Flags operator&(const Flags f1, const Flags f2) noexcept {
        return static_cast<Flags>(static_cast<std::underlying_type_t<Piece::Flags>>(f1) & static_cast<std::underlying_type_t<Piece::Flags>>(f2));
    }
    constexpr friend Flags operator&(const Flags f1, const int f2) noexcept {
        return static_cast<Flags>(static_cast<std::underlying_type_t<Piece::Flags>>(f1) & f2);
    }
    constexpr Piece operator|(const Piece f) const noexcept {
        return static_cast<Flags>(static_cast<std::underlying_type_t<Piece::Flags>>(flags) | static_cast<std::underlying_type_t<Piece::Flags>>(f.flags));
    }
    constexpr Piece operator|(const Flags f) const noexcept {
        return static_cast<Flags>(static_cast<std::underlying_type_t<Piece::Flags>>(flags) | static_cast<std::underlying_type_t<Piece::Flags>>(f));
    }
    constexpr friend Flags operator|(const Flags f1, const Flags f2) noexcept {
        return static_cast<Flags>(static_cast<std::underlying_type_t<Piece::Flags>>(f1) | static_cast<std::underlying_type_t<Piece::Flags>>(f2));
    }
    constexpr friend Flags operator|(const Flags f1, const int f2) noexcept {
        return static_cast<Flags>(static_cast<std::underlying_type_t<Piece::Flags>>(f1) | f2);
    }
    constexpr Piece operator~() const noexcept {
        return static_cast<Flags>(~static_cast<std::underlying_type_t<Piece::Flags>>(flags));
    }
    constexpr friend Flags operator~(const Flags f) noexcept {
        return static_cast<Flags>((~static_cast<std::underlying_type_t<Piece::Flags>>(f)) & 0b1111);
    }


    enum class Value {
        INACTIVE = 0,
        PAWN = 10,
        SHAIKH = 50,
    };

    constexpr int value() const noexcept {
        if (not isActive()) return static_cast<int>(Value::INACTIVE);
        return static_cast<int>(isShaikh() ? Value::SHAIKH : Value::PAWN);
    }

};

