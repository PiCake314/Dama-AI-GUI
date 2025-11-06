#pragma once

#include <iostream>
#include <vector>
#include <string_view>
#include <array>
#include <ranges>

#include <SFML/Graphics.hpp>

#include "Piece.hxx"
#include "Move.hxx"
#include "helper.hxx"


struct BoardState {
    constexpr static std::string_view STARTING_FEN = "8/bbbbbbbb/bbbbbbbb/8/8/yyyyyyyy/yyyyyyyy/8";

    // whoever is true, goes first
    constexpr static auto YELLOW_TURN = true;
    constexpr static auto BLACK_TURN  = not YELLOW_TURN;

    constexpr static auto YELLOW_FORWARD = YELLOW_TURN ? Position{0, -1} :  Position{0, 1}; // yellow goes up, which is in the negative direction
    constexpr static auto BLACK_FORWARD  = -YELLOW_FORWARD;
    constexpr static auto RIGHT = Position{ 1, 0};
    constexpr static auto LEFT  = Position{-1, 0};



    std::array<std::array<Piece, 8>, 8> board;
    bool turn = YELLOW_TURN;



    bool yellowTurn() const noexcept { return turn == YELLOW_TURN; }
    bool blackTurn()  const noexcept { return turn == BLACK_TURN;  }
    void nextTurn() noexcept { turn =! turn; }
    auto currentForward() const noexcept { return yellowTurn() ? YELLOW_FORWARD : BLACK_FORWARD; }


    explicit BoardState(const std::string_view fen = STARTING_FEN) noexcept {
        int x = 0;
        int y = 0;

        size_t index{};

        for(char c : fen){
            switch(c){
                case 'y':
                    // pieces[index] = Piece{{x, y}, Piece::Color::YELLOW, false};
                    // board[y][x] = Piece{{x, y}, Piece::Color::YELLOW, false};
                    board[y][x] = Piece::Flags::ACTIVE | Piece::Flags::YELLOW;

                    ++x;
                    ++index;
                    break;
                case 'Y':
                    // pieces[index] = Piece{{x, y}, Piece::Color::YELLOW, true};
                    // board[y][x] = Piece{{x, y}, Piece::Color::YELLOW, true};
                    board[y][x] = Piece::Flags::ACTIVE | Piece::Flags::YELLOW | Piece::Flags::SHAIKH;
                    ++x;
                    ++index;
                    break;
                case 'b':
                    // pieces[index] = Piece{{x, y}, Piece::Color::BLACK, false};
                    // board[y][x] = Piece{{x, y}, Piece::Color::BLACK, false};
                    board[y][x] = Piece::Flags::ACTIVE; // active assumed to be black pawn
                    ++x;
                    ++index;
                    break;
                case 'B':
                    // pieces[index] = Piece{{x, y}, Piece::Color::BLACK, true};
                    // board[y][x] = Piece{{x, y}, Piece::Color::BLACK, true};
                    board[y][x] = Piece::Flags::ACTIVE | Piece::Flags::SHAIKH;
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


    void checkPressed(sf::RenderWindow& window) {
        prev_holding = curr_holding;
        curr_holding = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        if (not prev_holding and not dragging and curr_holding) {
            const auto [mouse_x, mouse_y] = sf::Mouse::getPosition(window);

            for (const auto [y, row] : enumerate(board)) {
                for (const auto [x, piece] : enumerate(row)) {
                    const auto shape = piece.getShape(window, {static_cast<float>(x), static_cast<float>(y)});

                    if (shape.getGlobalBounds().contains({static_cast<float>(mouse_x), static_cast<float>(mouse_y)})) {
                        dragging = &piece;
                        holding_x = x;
                        holding_y = y;
                        piece.setDragged(true);

                        break;
                    }

                    if (dragging) break;
                }
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


            const auto moves = generateMoves();

            for (const auto& [index, move] : enumerate(moves)) {
                std::clog << '[' << index << "]: ";
                for (const auto& pos : move.positions) {
                    std::clog << '{' << pos.x << ", " << pos.y << "} -> ";
                }

                std::clog << "\ncaptures: ";
                for (const auto& pos : move.captures) {
                    std::clog << '{' << pos.x << ", " << pos.y << "} -> ";
                }

                puts("");
            }
        }
    }


    void update(sf::RenderWindow& window) {
        checkPressed(window);
        checkReleased(window);
    }


    void render(sf::RenderWindow& window) const {
        for (const auto [y, row] : enumerate(board)) {
            for (const auto [x, piece] : enumerate(row)) {
                if (not piece.isDragged()) piece.render(window, {static_cast<float>(x), static_cast<float>(y)});
            }
        }

        if (dragging) {
            const auto [x, y] = sf::Mouse::getPosition(window);
            dragging->render(window, {static_cast<float>(x), static_cast<float>(y)}, true);
        }
    }

    constexpr static bool isValidPosition(const Position& pos) noexcept {
        return pos.x >= 0 and pos.x < 8 and pos.y >= 0 and pos.y <8;
    }

    std::vector<Move> generatePawnMoves(const Piece piece, const Position& pos) {
        const auto forward = currentForward();


        std::vector<Move> moves;
        for (const auto direction : {forward, RIGHT, LEFT}) {
            const auto landing_pos = pos + direction;

            if (not isValidPosition(landing_pos)) continue;


            if (const auto landing_piece = board[landing_pos.y][landing_pos.x]; not landing_piece.isActive()) {
                moves.push_back({{pos, landing_pos}});
            }
            else if (landing_piece.isYellow() != yellowTurn()) { // landing piece is an opponent piece
                // check if the next block is a valid inactive piece, if so, this is a capturing move

                const auto capture_landing_pos = landing_pos + direction; // move an extra step
                if (isValidPosition(capture_landing_pos)) {
                    moves.push_back({{pos, capture_landing_pos}, {landing_pos}});
                }
            }
        }


        return moves;
    }

    std::vector<Move> generateShaikhMoves(const Piece piece, const Position& pos) {
        return {};
    }


    std::vector<Move> generateMoves() {
        std::vector<Move> moves;

        for (const auto [y, row] : enumerate(board)) {
            for (const auto [x, piece] : enumerate(row)) {
                if (not piece.isActive()) continue;

                if (piece.isYellow() == yellowTurn()) // either both piece and turn are yellow, or both are black
                    moves.insert_range(
                        moves.cend(),
                        piece.isPawn() ?
                            generatePawnMoves(piece, {static_cast<int>(x), static_cast<int>(y)})
                        : generateShaikhMoves(piece, {static_cast<int>(x), static_cast<int>(y)})
                    );
            }
        }



        return moves;
    }
};
