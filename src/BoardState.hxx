#pragma once

#include <iostream>
#include <array>
#include <vector>
#include <string_view>
#include <memory>

#include <SFML/Graphics.hpp>

#include "Piece.hxx"
#include "Move.hxx"



struct BoardState {
    constexpr static std::string_view STARTING_FEN = "8/yyyyyyyy/yyyyyyyy/8/8/bbbbbbbb/bbbbbbbb/8";

    std::array<std::array<Piece, 8>, 8> board;

    // std::array<Piece, 32> pieces;


    explicit BoardState(const std::string_view fen = STARTING_FEN) noexcept {
        int x = 0;
        int y = 0;

        size_t index{};

        for(char c : fen){
            switch(c){
                case 'y':
                    // pieces[index] = Piece{{x, y}, Piece::Color::YELLOW, false};
                    // board[y][x] = Piece{{x, y}, Piece::Color::YELLOW, false};
                    board[y][x] = Piece::FLAGS::ACTIVE | Piece::FLAGS::YELLOW;

                    ++x;
                    ++index;
                    break;
                case 'Y':
                    // pieces[index] = Piece{{x, y}, Piece::Color::YELLOW, true};
                    // board[y][x] = Piece{{x, y}, Piece::Color::YELLOW, true};
                    board[y][x] = Piece::FLAGS::ACTIVE | Piece::FLAGS::YELLOW | Piece::FLAGS::SHAIKH;
                    ++x;
                    ++index;
                    break;
                case 'b':
                    // pieces[index] = Piece{{x, y}, Piece::Color::BLACK, false};
                    // board[y][x] = Piece{{x, y}, Piece::Color::BLACK, false};
                    board[y][x] = Piece::FLAGS::ACTIVE; // active assumed to be black pawn
                    ++x;
                    ++index;
                    break;
                case 'B':
                    // pieces[index] = Piece{{x, y}, Piece::Color::BLACK, true};
                    // board[y][x] = Piece{{x, y}, Piece::Color::BLACK, true};
                    board[y][x] = Piece::FLAGS::ACTIVE | Piece::FLAGS::SHAIKH;
                    ++x;
                    ++index;
                    break;
                case '/':
                    ++y;
                    x = 0;
                    break;
                default:
                    int num = c - '0';
                    x += num;
                    break;
            }
        }
    }


    bool prev_holding = false;
    bool curr_holding = false;
    Piece *dragging = nullptr;
    int holding_x, holding_y;
    // // bool dragging_king = false;
    // Piece dragable;


    void checkPressed(sf::RenderWindow& window) {
        prev_holding = curr_holding;
        curr_holding = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        if (not prev_holding and not dragging and curr_holding) {
            // for (auto& piece : pieces) {
            const auto [mouse_x, mouse_y] = sf::Mouse::getPosition(window);

            for (int y{}; auto& row : board) {
                for (int x{}; auto& piece : row) {
                    const auto shape = piece.getShape(window, {static_cast<float>(x), static_cast<float>(y)});

                    if (shape.getGlobalBounds().contains({static_cast<float>(mouse_x), static_cast<float>(mouse_y)})) {
                        dragging = &piece;
                        holding_x = x;
                        holding_y = y;
                        piece.setDragged(true);

                        // piece.color = Piece::Color::NONE;
                        break;
                    }

                    if (dragging) break;
                    ++x;
                }
                ++y;
            }
        }
    }

    void checkReleased(sf::RenderWindow& window) {
        if (dragging and not sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

            const auto [mouse_x, mouse_y] = sf::Mouse::getPosition(window);

            const auto [width, height] = window.getSize();
            const float block_size = width / 8;

            const int x = mouse_x / block_size;
            const int y = mouse_y / block_size;

            dragging->setDragged(false);
            dragging = nullptr;

            if (x == holding_x and y == holding_y) return;


            board[y][x] = board[holding_y][holding_x];
            board[holding_y][holding_x] = Piece{};

            // std::clog << "x: " << x << ", y: " << y << '\n';
            // std::clog << "holding x: " << holding_x << ", holding y: " << holding_y << '\n';


        }
    }


    void update(sf::RenderWindow& window) {
        checkPressed(window);

        checkReleased(window);
    }


    void render(sf::RenderWindow& window) const {

        for (int y{}; const auto& row : board) {
            for (int x{}; const auto& piece : row) {
                if (not piece.isDragged()) piece.render(window, {static_cast<float>(x), static_cast<float>(y)});

                ++x;
            }
            ++y;
        }

        if (dragging) {
            const auto [x, y] = sf::Mouse::getPosition(window);
            dragging->render(window, {static_cast<float>(x), static_cast<float>(y)}, true);
        }
    }
};
