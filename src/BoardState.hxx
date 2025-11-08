#pragma once

#include <iostream>
#include <string_view>
#include <array>
#include <vector>
#include <span>
#include <algorithm>
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
    std::vector<Move> possible_moves;


    bool yellowTurn() const noexcept { return turn == YELLOW_TURN; }
    bool blackTurn()  const noexcept { return turn == BLACK_TURN;  }
    void nextTurn() noexcept { turn =! turn; }
    auto currentForward() const noexcept { return yellowTurn() ? YELLOW_FORWARD : BLACK_FORWARD; }
    auto& get(const Position pos) noexcept { return board[pos.y][pos.x]; }

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

        generateMoves();
    }


    bool prev_holding = false;
    bool curr_holding = false;
    Piece *dragging = nullptr;
    int holding_x, holding_y;
    std::vector<Move> holding_moves;

    void checkPressed(sf::RenderWindow& window) {
        prev_holding = curr_holding;
        curr_holding = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

        if (not prev_holding and not dragging and curr_holding) {
            const auto [mouse_x, mouse_y] = sf::Mouse::getPosition(window);

            for (const auto [y, row] : enumerate(board)) {
                for (const auto [x, piece] : enumerate(row)) {
                    // if not active or not the turn
                    if (not piece.isActive() or piece.isYellow() != yellowTurn()) continue;

                    const auto shape = piece.getShape(window, {static_cast<float>(x), static_cast<float>(y)});
                    if (shape.getGlobalBounds().contains({static_cast<float>(mouse_x), static_cast<float>(mouse_y)})) {
                        dragging = &piece;
                        holding_x = x;
                        holding_y = y;
                        piece.setDragged(true);

                        // holding_moves.clear();
                        for (const auto move : possible_moves) {
                            if (move.positions[0].x == x and move.positions[0].y == y) {
                                holding_moves.push_back(move);
                            }
                        }

                        return;
                    }
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

            // if (x == holding_x and y == holding_y) return;

            std::clog << "X: " << x << ", Y: " << y << "\nMoves:\n";
            const auto move_iter = std::ranges::find_if(holding_moves, [x, y] (const Move& move) {
                std::clog << move.positions[1].x << ", " << move.positions[1].y << '\n';
                return move.positions[1].x == x and move.positions[1].y == y;
            });

            if (move_iter == holding_moves.cend()) return holding_moves.clear();



            board[y][x] = board[holding_y][holding_x];
            board[holding_y][holding_x] = Piece{};



            if (move_iter->doesCapture()) get(move_iter->captures[0]) = Piece{}; // reset captured piece!

            // positions = [from -> to]. we're done here
            if (move_iter->positions.size() == 2) nextTurn();

            holding_moves.clear();

            generateMoves();
        }
    }


    std::span<Move> update(sf::RenderWindow& window) {
        checkPressed(window);
        checkReleased(window);

        return holding_moves;
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

    constexpr static bool isValidPosition(const Position pos) noexcept {
        return pos.x >= 0 and pos.x < 8 and pos.y >= 0 and pos.y <8;
    }


    void simulateCapture(const Piece piece, const Position from, const Position capture, const Position to) noexcept {
        get(from) = Piece{};
        get(capture) = Piece{};
        get(to) = piece;
    }

    void unsimulateCapture(const Piece piece, const Piece captured_piece, const Position from, const Position capture, const Position to) noexcept {
        get(from) = piece;
        get(capture) = captured_piece;
        get(to) = Piece{};
    }


    std::vector<Move> generatePawnCaptureMoves(const Piece piece, const Position pos) {
        prettyPrint();

        const auto forward = currentForward();

        std::vector<Move> moves;
        for(const auto direction : {forward, RIGHT, LEFT}) {
            const auto capture_pos = pos + direction;
            const auto captured_piece = get(capture_pos); 
            const auto landing_pos = capture_pos + direction;

            //                                      if the captured piece is not the opponent's
            if (
                   not isValidPosition(landing_pos)
                or     get(landing_pos).isActive()
                or not captured_piece.isActive()
                or     captured_piece.isYellow() == yellowTurn()
            ) continue;


            // can't capture my own piece


            // moves.push_back(Move{{pos, landing_pos}, {capture_pos}});

            // simulate a move;
            // get(landing_pos) = piece;
            // get(pos) = Piece{};
            // get(capture_pos) = Piece{};
            simulateCapture(piece, pos, capture_pos, landing_pos);

            auto continuations = generatePawnCaptureMoves(piece, landing_pos);

            Move move{{pos, /* landing_pos */ }, {capture_pos, }};
            if (continuations.empty()) {
                move.positions.push_back(landing_pos);
                moves.push_back(move);
            }
            else for (const auto& cont : continuations) {
                for (const auto& position : cont.positions /* | std::views::drop(1) */ ) { //// dropping the first position since it was accounted for in `move` above
                    move.positions.push_back(position);
                }

                for (const auto& capture : cont.captures) {
                    move.captures.push_back(capture);
                }

                moves.push_back(move);
            }


            // unsimulate a move;
            // get(pos) = piece;
            // get(landing_pos) = Piece{};
            // get(capture_pos) = captured_piece;
            unsimulateCapture(piece, captured_piece, pos, capture_pos, landing_pos);

        }


        return moves;
    }

    std::vector<Move> generatePawnMoves(const Piece piece, const Position pos) {

        const auto forward = currentForward();

        std::vector<Move> moves;
        for (const auto direction : {forward, RIGHT, LEFT}) {
            const auto landing_pos = pos + direction;

            if (not isValidPosition(landing_pos)) continue;


            if (const auto landing_piece = get(landing_pos); not landing_piece.isActive()) {
                moves.push_back({{pos, landing_pos}});
            }
            else if (landing_piece.isYellow() != yellowTurn()) { // landing piece is an opponent piece
                // check if the next block is a valid inactive piece, if so, this is a capturing move
                // const auto capture_pos = landing_pos;
                // const auto new_landing_pos = capture_pos + direction; // move an extra step
                // if (not isValidPosition(new_landing_pos)) continue;

                // // Move move{{pos, capture_landing_pos}, {landing_pos}};
                // const auto captured_piece = get(capture_pos);

                // simulateCapture(piece, pos, capture_pos, new_landing_pos);
                auto capture_moves = generatePawnCaptureMoves(piece, pos);
                // unsimulateCapture(piece, captured_piece, pos, capture_pos, new_landing_pos);


                moves.insert_range(moves.cend(), std::move(capture_moves));
            }
        }

        return moves;
    }

    std::vector<Move> generateShaikhMoves(const Piece piece, const Position pos) {
        return {};
    }


    void generateMoves() {
        possible_moves.clear();

        for (const auto [y, row] : enumerate(board)) {
            for (const auto [x, piece] : enumerate(row)) {
                if (not piece.isActive()) continue;

                if (piece.isYellow() == yellowTurn()) // either both piece and turn are yellow, or both are black
                    possible_moves.insert_range(
                        possible_moves.cend(),
                        piece.isPawn() ?
                            generatePawnMoves(piece, {static_cast<int>(x), static_cast<int>(y)})
                        : generateShaikhMoves(piece, {static_cast<int>(x), static_cast<int>(y)})
                    );
            }
        }


        if (std::ranges::any_of(possible_moves, [] (const auto& move) { return move.doesCapture(); }))
            // possible_moves = std::ranges::remove_if(possible_moves, [] (const auto& move) { return not move.doesCapture(); }) | std::ranges::to<std::vector<Move>>();
            // possible_moves.erase(std::remove_if(possible_moves.begin(), possible_moves.end(), [] (const auto& move) { return not move.doesCapture(); }), possible_moves.cend());
            // std::erase_if(std::begin(possible_moves), std::end(possible_moves), )
            std::erase_if(possible_moves, [] (const auto& move) { return not move.doesCapture(); });


        printMoves(possible_moves);
    }


    void prettyPrint() const {
        std::clog << "==================\n";

        for (const auto& row : board) {
            for (const auto& piece : row) {
                piece.prettyPrint();
            }
            std::clog << '\n';
        }

        std::clog << "==================\n";
    }

    static void printMoves(const std::vector<Move>& moves) {
        for (const auto& [idx, move] : enumerate(moves)) {
            std::clog << '[' << idx << "]: ";
            for (const auto pos : move.positions)
                std::clog << '{' << pos.x << ", " << pos.y << "} -> ";
            std::clog << '\n';

            std::clog << "caps: ";
            for (const auto pos : move.captures)
                std::clog << '(' << pos.x << ", " << pos.y << ") ,";
            std::clog << "\n\n";
        }
    }
};
