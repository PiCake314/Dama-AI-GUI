#pragma once

#include <print>
#include <iostream>
#include <type_traits>

#include <SFML/Graphics.hpp>

#include "Position.hxx"


struct Piece {


    // assume black, undragged, pawn if active
    enum FLAGS {
        NONE        = 0b0000,

        ACTIVE      = 0b1000,
        YELLOW      = 0b0001,
        SHAIKH      = 0b0010,
        DRAGGED     = 0b0100,
    };

    constexpr Piece operator&(const Piece f) const noexcept {
        return static_cast<FLAGS>(static_cast<std::underlying_type_t<Piece::FLAGS>>(flags) & static_cast<std::underlying_type_t<Piece::FLAGS>>(f.flags));
    }

    constexpr Piece operator&(const FLAGS f) const noexcept {
        return static_cast<FLAGS>(static_cast<std::underlying_type_t<Piece::FLAGS>>(flags) & static_cast<std::underlying_type_t<Piece::FLAGS>>(f));
    }
    constexpr friend FLAGS operator&(const FLAGS f1, const FLAGS f2) noexcept {
        return static_cast<FLAGS>(static_cast<std::underlying_type_t<Piece::FLAGS>>(f1) & static_cast<std::underlying_type_t<Piece::FLAGS>>(f2));
    }
    constexpr friend FLAGS operator&(const FLAGS f1, const int f2) noexcept {
        return static_cast<FLAGS>(static_cast<std::underlying_type_t<Piece::FLAGS>>(f1) & f2);
    }

    constexpr Piece operator|(const Piece f) const noexcept {
        return static_cast<FLAGS>(static_cast<std::underlying_type_t<Piece::FLAGS>>(flags) | static_cast<std::underlying_type_t<Piece::FLAGS>>(f.flags));
    }
    constexpr Piece operator|(const FLAGS f) const noexcept {
        return static_cast<FLAGS>(static_cast<std::underlying_type_t<Piece::FLAGS>>(flags) | static_cast<std::underlying_type_t<Piece::FLAGS>>(f));
    }
    constexpr friend FLAGS operator|(const FLAGS f1, const FLAGS f2) noexcept {
        return static_cast<FLAGS>(static_cast<std::underlying_type_t<Piece::FLAGS>>(f1) | static_cast<std::underlying_type_t<Piece::FLAGS>>(f2));
    }
    constexpr friend FLAGS operator|(const FLAGS f1, const int f2) noexcept {
        return static_cast<FLAGS>(static_cast<std::underlying_type_t<Piece::FLAGS>>(f1) | f2);
    }

    constexpr Piece operator~() const noexcept {
        return static_cast<FLAGS>(~static_cast<std::underlying_type_t<Piece::FLAGS>>(flags));
    }
    constexpr friend FLAGS operator~(const FLAGS f) noexcept {
        return static_cast<FLAGS>((~static_cast<std::underlying_type_t<Piece::FLAGS>>(f)) & 0b1111);
    }


    // constexpr bool operator not() const noexcept {
    //     return not static_cast<std::underlying_type_t<Piece::FLAGS>>(flags);
    // }


    constexpr Piece(FLAGS f) noexcept : flags{f} {}
    constexpr Piece& operator=(const FLAGS& f) noexcept { flags = f; return *this; }

    // rule of 5,, just to be safe
    constexpr Piece() noexcept = default;
    constexpr Piece(const Piece& p) noexcept = default;
    constexpr Piece(Piece&& p) noexcept = default;
    constexpr Piece& operator=(const Piece& p) noexcept = default;
    constexpr Piece& operator=(Piece&& p) noexcept = default;

    // enum class Color { NONE, YELLOW, BLACK, };

    // Position position{};

    // Color color{};
    // bool shaikh{};
    // bool dragged = false;

    FLAGS flags : 4 = FLAGS::NONE;

    constexpr bool isActive()     const noexcept { return flags & FLAGS::ACTIVE;      }
    constexpr bool isYellow()     const noexcept { return flags & FLAGS::YELLOW;      }
    constexpr bool isBlack()      const noexcept { return not isYellow();             }
    constexpr bool isPawn()       const noexcept { return not isShaikh();             }
    constexpr bool isShaikh()     const noexcept { return flags & FLAGS::SHAIKH;      }
    constexpr bool isDragged()    const noexcept { return flags & FLAGS::DRAGGED;     }

    constexpr void setDragged(const bool d) noexcept {
        if (d) flags = flags | FLAGS::DRAGGED;
        else {
            flags = flags & ~FLAGS::DRAGGED;
        }
    }


    sf::CircleShape getShape(sf::RenderWindow& window, const sf::Vector2f pos, const bool dragging = false) const {
        const auto winsize = window.getSize();
        const float radius = winsize.x / 20; // / 10 / 2

        sf::CircleShape circle{radius};
        circle.setFillColor(isBlack() ? sf::Color{32, 20, 0} : sf::Color{204, 126, 0});
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
};

