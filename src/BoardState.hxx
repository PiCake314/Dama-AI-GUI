#pragma once

#include <iostream>
#include <cmath>
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


consteval auto precomputeData(){
    std::array<std::array<std::array<static_vector<Position, 8>, 4>, 8>, 8> data{};

    for(int i = 0; i < 8; ++i){
        for(int j = 0; j < 8; ++j){
            const auto num_north = i;
            const auto num_east  = 8 - j - 1;
            const auto num_south = 8 - i - 1;
            const auto num_west  = j;

            for (const auto e : range(1, num_north +1)) data[i][j][0].push_back({ 0, -e});
            for (const auto e : range(1, num_east  +1)) data[i][j][1].push_back({ e,  0});
            for (const auto e : range(1, num_south +1)) data[i][j][2].push_back({ 0,  e});
            for (const auto e : range(1, num_west  +1)) data[i][j][3].push_back({-e,  0});

            // data[i][j] = {num_north, num_east, num_south, num_west};
        }
    }

    return data;
}
constexpr auto DATA_ARRAY = precomputeData();


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


    explicit constexpr BoardState(const std::string_view fen = STARTING_FEN, bool yellow_starts = true) noexcept
    : turn{yellow_starts}
    {
        parseFen(fen);
        generateMoves();
    }

    explicit constexpr BoardState(bool yellow_starts) noexcept
    : turn{yellow_starts}
    {
        parseFen(STARTING_FEN);
        generateMoves();
    }

    size_t yellowCount() const noexcept {
        size_t count{};
        for (const auto& row : board) for (const auto& piece : row) count += piece.isActive() and piece.isYellow();
        return count;
    }

    size_t blackCount() const noexcept {
        size_t count{};
        for (const auto& row : board) for (const auto& piece : row) count += piece.isActive() and piece.isBlack();
        return count;
    }

    bool done() const noexcept { return yellowCount() == 0 or blackCount() == 0; }


    bool yellowTurn() const noexcept { return turn == YELLOW_TURN; }
    bool blackTurn()  const noexcept { return turn == BLACK_TURN;  }
    void nextTurn() noexcept { turn =! turn; }
    auto currentForward() const noexcept { return yellowTurn() ? YELLOW_FORWARD : BLACK_FORWARD; }
          auto& get(const Position pos)       noexcept { return board[pos.y][pos.x]; }
    const auto& get(const Position pos) const noexcept { return board[pos.y][pos.x]; }

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



            get({x, y}) = get({holding_x, holding_y});
            get({holding_x, holding_y}).reset();

            if (y == 0 or y == 7) get({x, y}).promote();



            if (move_iter->doesCapture()) get(move_iter->captures[0]).reset(); // reset captured piece!

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
        get(from).reset();
        get(capture).reset();
        get(to) = piece;
    }

    void unsimulateCapture(const Piece piece, const Piece captured_piece, const Position from, const Position capture, const Position to) noexcept {
        get(from) = piece;
        get(capture) = captured_piece;
        get(to).reset();
    }


    std::vector<Move> generateManCaptureMoves(const Piece piece, const Position pos) {
        const auto forward = currentForward();

        std::vector<Move> moves;
        for(const auto direction : {forward, RIGHT, LEFT}) {
            const auto capture_pos = pos + direction;
            const auto captured_piece = get(capture_pos); 
            const auto landing_pos = capture_pos + direction;


            if (
                   not isValidPosition(landing_pos) // outside the board
                or not captured_piece.isActive()    // not capturing a piece
                or   get(landing_pos).isActive()    // landing at an occupied position
                or     captured_piece.isYellow() == yellowTurn() // captured piece is not the opponent's
            ) continue;


              simulateCapture(piece,                 pos, capture_pos, landing_pos);
            const auto continuations = generateManCaptureMoves(piece,  landing_pos);
            unsimulateCapture(piece, captured_piece, pos, capture_pos, landing_pos);

            if (continuations.empty()) {
                moves.push_back({{pos, landing_pos}, {capture_pos, }});
            }
            else for (const auto& cont : continuations) {
                Move move{{pos, /* landing_pos */ }, {capture_pos, }};
                for (const auto& position : cont.positions) {
                    move.positions.push_back(position);
                }

                for (const auto& capture : cont.captures) {
                    move.captures.push_back(capture);
                }

                moves.push_back(move);
            }

        }


        return moves;
    }


    std::vector<Move> generateManMoves(const Piece piece, const Position pos) {
        const auto forward = currentForward();

        std::vector<Move> moves;
        for (const auto direction : {forward, RIGHT, LEFT}) {
            const auto landing_pos = pos + direction;

            if (not isValidPosition(landing_pos)) continue;


            if (const auto landing_piece = get(landing_pos); not landing_piece.isActive()) {
                moves.push_back({{pos, landing_pos}});
            }
            else if (landing_piece.isYellow() != yellowTurn()) { // landing piece is an opponent piece
                auto capture_moves = generateManCaptureMoves(piece, pos);
                moves.insert_range(moves.cend(), std::move(capture_moves));
            }
        }

        return moves;
    }


    std::vector<Move> generateShaikhCaptureMoves(const Piece piece, const Position pos, const int forbidden_direction) {
        std::vector<Move> moves;


        for (const auto direction_idx : range(4)) {
            if (direction_idx == forbidden_direction) continue;

            Position capture_pos;
            Piece captured_piece;
            for(int count{}; const auto distance : DATA_ARRAY[pos.y][pos.x][direction_idx]) {
                const auto landing_pos = pos + distance;

                const auto landing_piece = get(landing_pos);

                if (landing_piece.isActive()) {
                    if (landing_piece.isYellow() == yellowTurn()) break; // can't jump over my own piece. Go to next direction

                    // landing_piece.isYellow() != yellowTurn()
                    // counting the opponents pieces I encounter
                    else {
                        ++count;                                                                   // up here v
                        if (count == 1) {
                            capture_pos = landing_pos;
                            captured_piece = get(capture_pos);
                        }
                        // count has to be greater than 1 since it starts as 0 and we're incrementing up there ^
                        else break; // cannot jump over more than 1 opponent piece
                    }
                }

                // not landing_piece.isActive()
                else {
                    if (count == 1) {

                        simulateCapture(piece,                 pos, capture_pos,     landing_pos);
                        const auto continuations = generateShaikhCaptureMoves(piece, landing_pos, (direction_idx + 2) % 4);
                        unsimulateCapture(piece, captured_piece, pos, capture_pos,   landing_pos);


                        if (continuations.empty()) {
                            moves.push_back({{pos, landing_pos}, {capture_pos, }});
                        }
                        else for (const auto& cont : continuations) {
                            Move move{{pos, /* landing_pos */ }, {capture_pos, }};
                            for (const auto& position : cont.positions) {
                                move.positions.push_back(position);
                            }

                            for (const auto& capture : cont.captures) {
                                move.captures.push_back(capture);
                            }

                            moves.push_back(move);
                        }
                    }
                }
            }
        }

        return moves;
    }


    std::vector<Move> generateShaikhMoves(const Piece piece, const Position pos) {
        std::vector<Move> moves;

        for (const auto direction_idx : range(4)) {

                // counting opponent's pieces across a certain direction. 
            for (int count{}; const auto distance : DATA_ARRAY[pos.y][pos.x][direction_idx]) {
                const auto landing_pos = pos + distance;

                const auto landing_piece = get(landing_pos);

                if (landing_piece.isActive()) {
                    if (landing_piece.isYellow() == yellowTurn()) break; // can't jump over my own piece. Go to next direction

                    // landing_piece.isYellow() != yellowTurn()
                    // counting the opponents pieces I encounter
                    else if (++count > 1) break; // cannot jump over more than 1 opponent piece
                }

                // not landing_piece.isActive()
                else {
                    if (count == 0) {
                        moves.push_back({{pos, landing_pos}});
                    }
                    if (count == 1) {
                        auto capture_moves = generateShaikhCaptureMoves(piece, pos, (direction_idx + 2) % 4);
                        moves.insert_range(moves.cend(), std::move(capture_moves));
                    }
                }
            }
        }

        return moves;
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
                            generateManMoves(piece, {static_cast<int>(x), static_cast<int>(y)})
                        : generateShaikhMoves(piece, {static_cast<int>(x), static_cast<int>(y)})
                    );
            }
        }


        if (std::ranges::any_of(possible_moves, [] (const auto& move) { return move.doesCapture(); })) {

            // possible_moves = std::ranges::remove_if(possible_moves, [] (const auto& move) { return not move.doesCapture(); }) | std::ranges::to<std::vector<Move>>();
            // possible_moves.erase(std::remove_if(possible_moves.begin(), possible_moves.end(), [] (const auto& move) { return not move.doesCapture(); }), possible_moves.cend());
            // std::erase_if(std::begin(possible_moves), std::end(possible_moves), )
            const auto max_captures = std::ranges::max_element(possible_moves, [] (const Move& m1, const Move& m2) { return m1.captures.size() < m2.captures.size(); })->captures.size();
            std::clog << "Max captures: " << max_captures << '\n';
            std::erase_if(possible_moves, [max_captures] (const Move& move) { return move.captures.size() < max_captures; });
        }


        printMoves(possible_moves);
    }


    void parseFen(const std::string_view fen) noexcept {
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
                    board[y][x] = Piece::Flags::ACTIVE; // active assumed to be black man
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
