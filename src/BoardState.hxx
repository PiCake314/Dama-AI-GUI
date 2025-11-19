#pragma once

#include <iostream>
#include <cmath>
#include <string_view>
#include <vector>
#include <array>
#include <algorithm>
#include <thread>
#include <chrono>
#include <span>
#include <limits>
#include <ranges>
#include <utility>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>


#include "Piece.hxx"
#include "Move.hxx"
#include "helper.hxx"


constexpr auto INF = 100000;


consteval auto precomputeData(){
    std::array<std::array<std::array<static_vector<Position, 8>, 4>, 8>, 8> data{};

    for(const auto i : range(8uz)){
        for(const auto j : range(8uz)){
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

    bool prev_holding = false;
    bool curr_holding = false;
    Piece *dragging = nullptr;
    int holding_x, holding_y;
    std::vector<Move> holding_moves;
    // Move inflight;
    // std::vector<Move> inflight_moves; // there could be multiple moves in flight!
    bool inflight = false; // is in the middle of a move?

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
        for (const auto& row : board) for (const auto piece : row) count += piece.isActive() and piece.isYellow();
        return count;
    }

    size_t blackCount() const noexcept {
        size_t count{};
        for (const auto& row : board) for (const auto piece : row) count += piece.isActive() and piece.isBlack();
        return count;
    }

    bool done() const noexcept { return yellowCount() == 0 or blackCount() == 0; }


    bool yellowTurn() const noexcept { return turn == YELLOW_TURN; }
    bool blackTurn()  const noexcept { return turn == BLACK_TURN;  }
    void nextTurn() noexcept { turn =! turn; }
    auto currentForward() const noexcept { return yellowTurn() ? YELLOW_FORWARD : BLACK_FORWARD; }
          auto& get(const Position pos)       noexcept { return board[size_t(pos.y)][size_t(pos.x)]; }
    const auto& get(const Position pos) const noexcept { return board[size_t(pos.y)][size_t(pos.x)]; }


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
                        holding_x = static_cast<int>(x);
                        holding_y = static_cast<int>(y);
                        piece.setDragged(true);

                        // holding_moves.clear();

                        if (not inflight) for (const auto& move : possible_moves) {
                            move.prettyPrint();
                            if (move.positions[0].x == static_cast<int>(x) and move.positions[0].y == static_cast<int>(y)) {
                                holding_moves.push_back(move);
                            }
                        }
                        else for (const auto& move : possible_moves) {
                            if (move.positions[0].x == static_cast<int>(x) and move.positions[0].y == static_cast<int>(y)) {
                                holding_moves.push_back(move);
                            }
                        }

                        return; // break the double loops
                    }
                }
            }
        }
    }


    void checkReleased(sf::RenderWindow& window) {
        SoundFX sound_fx = SoundFX::MOVE; // default

        if (dragging and not sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

            const auto [mouse_x, mouse_y] = sf::Mouse::getPosition(window);

            const auto [width, height] = window.getSize();
            const float block_size = width / 8;

            const int x = int(mouse_x / block_size);
            const int y = int(mouse_y / block_size);

            dragging->setDragged(false);
            dragging = nullptr;

            if (x == holding_x and y == holding_y) return holding_moves.clear();


            if (not inflight) {
                // find the move the player "chose"
                const auto move_iter = std::ranges::find_if(holding_moves, [x, y] (const Move& move) {
                    return move.positions[1].x == x and move.positions[1].y == y;
                });

                if (move_iter == holding_moves.cend()) return holding_moves.clear();


                get({x, y}) = get({holding_x, holding_y});
                get({holding_x, holding_y}).reset();

                if (not get({x, y}).isShaikh() and  (y == 0 or y == 7)) {
                    get({x, y}).promote();
                    sound_fx = SoundFX::PROMOTE;
                }


                if (move_iter->doesCapture()) {
                    get(move_iter->captures[0].pos).reset();

                    if (sound_fx != SoundFX::PROMOTE) sound_fx = SoundFX::CAPTURE;
                }

                // positions = [from -> to]. we're done here
                if (move_iter->positions.size() == 2) {
                    holding_moves.clear();
                    nextTurn();
                    generateMoves();
                }
                // we're inflight! let's make an the inflight moves list
                else {
                    inflight = true;

                    possible_moves.clear();
                    for (auto& move : holding_moves) {
                        if (move.positions[1].x == x and move.positions[1].y == y) {
                            move.positions.pop_front();
                            move.captures .pop_front();
                            possible_moves.push_back(move);
                        }
                    }

                    // possible_moves = inflight_moves;
                } 
            }
            else { // move is inflight
                const auto move_iter = std::ranges::find_if(possible_moves, [x, y] (const Move& move) {
                    return move.positions[1].x == x and move.positions[1].y == y;
                });

                if (move_iter == possible_moves.cend()) return holding_moves.clear();


                get({x, y}) = get({holding_x, holding_y});
                get({holding_x, holding_y}).reset();

                if (not get({x, y}).isShaikh() and  (y == 0 or y == 7)) {
                    get({x, y}).promote();
                    sound_fx = SoundFX::PROMOTE;
                }

                // this must be a capturing move since we're during an inflight move
                if (sound_fx != SoundFX::PROMOTE) sound_fx = SoundFX::CAPTURE;

                get(move_iter->captures[0].pos).reset();

                if (move_iter->positions.size() == 2) {
                    inflight = false;

                    play(sound_fx);
                    possible_moves.clear();
                    holding_moves.clear();
                    nextTurn();
                    generateMoves();
                    return;
                }

                std::erase_if(possible_moves, [x, y] (const Move& move) { return move.positions[1].x != x or move.positions[1].y != y; });

                for (auto& move : possible_moves) {
                    move.positions.pop_front();
                    move.captures .pop_front();
                }

                // inflight = not possible_moves.empty();
                // possible_moves = inflight_moves;
            }


            play(sound_fx);
            holding_moves.clear();
        }
    }



    std::tuple<std::span<Move>, std::span<Move>, Move> update(sf::RenderWindow& window) {
        checkPressed(window);
        checkReleased(window);

        return {possible_moves, holding_moves, AI_highlighs};
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


    std::vector<Move> generatePawnCaptureMoves(const Piece piece, const Position pos) {
        std::vector<Move> moves;

        if (pos.y != 0 and pos.y != 7) {
            const auto forward = currentForward();

            for(const auto direction : {forward, RIGHT, LEFT}) {
                const auto capture_pos = pos + direction;
                const auto captured_piece = get(capture_pos); 
                const auto landing_pos = capture_pos + direction;


                if (
                       not isValidPosition(landing_pos)              // outside the board
                    or not captured_piece.isActive()                 // not capturing a piece
                    or   get(landing_pos).isActive()                 // landing at an occupied position
                    or     captured_piece.isYellow() == yellowTurn() // captured piece is not the opponent's
                ) continue;


                  simulateCapture(piece,                 pos, capture_pos, landing_pos);
                const auto continuations = generatePawnCaptureMoves(piece,  landing_pos);
                unsimulateCapture(piece, captured_piece, pos, capture_pos, landing_pos);

                if (continuations.empty()) {
                    moves.push_back({{pos, landing_pos}, {{captured_piece, capture_pos}, }});
                }
                else for (const auto& cont : continuations) {
                    Move move{{pos, /* landing_pos */ }, {{captured_piece, capture_pos}, }};
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
                auto capture_moves = generatePawnCaptureMoves(piece, pos);
                moves.insert_range(moves.cend(), std::move(capture_moves));
            }
        }

        return moves;
    }


    std::vector<Move> generateShaikhCaptureMoves(const Piece piece, const Position pos, const size_t forbidden_direction) {
        std::vector<Move> moves;


        for (const auto direction_idx : range(4uz)) {
            if (direction_idx == forbidden_direction) continue;

            Position capture_pos;
            Piece captured_piece;
            for(int count{}; const auto distance : DATA_ARRAY[size_t(pos.y)][size_t(pos.x)][direction_idx]) {
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
                            moves.push_back({{pos, landing_pos}, {{captured_piece, capture_pos}, }});
                        }
                        else for (const auto& cont : continuations) {
                            Move move{{pos, /* landing_pos */ }, {{captured_piece, capture_pos}, }};
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

        for (const auto direction_idx : range(4uz)) {

                // counting opponent's pieces across a certain direction. 
            for (size_t count{}; const auto distance : DATA_ARRAY[size_t(pos.y)][size_t(pos.x)][direction_idx]) {
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

        if (inflight) return;

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


        if (std::ranges::any_of(possible_moves, [] (const auto& move) { return move.doesCapture(); })) {

            // possible_moves = std::ranges::remove_if(possible_moves, [] (const auto& move) { return not move.doesCapture(); }) | std::ranges::to<std::vector<Move>>();
            // possible_moves.erase(std::remove_if(possible_moves.begin(), possible_moves.end(), [] (const auto& move) { return not move.doesCapture(); }), possible_moves.cend());
            // std::erase_if(std::begin(possible_moves), std::end(possible_moves), )
            const auto max_captures = std::ranges::max_element(possible_moves, [] (const Move& m1, const Move& m2) { return m1.captures.size() < m2.captures.size(); })->captures.size();
            std::erase_if(possible_moves, [max_captures] (const Move& move) { return move.captures.size() < max_captures; });
        }
    }

    // AI shit

    int evaluate() const noexcept {
        int yellow_total{};
        int black_total{};

        for (const auto& row : board)
            for (const auto piece : row)
                if (piece.isYellow())   yellow_total += piece.value();
                else /* must be black */ black_total += piece.value();


        const int perspective = blackTurn() ? 1 : -1;
        return perspective * (black_total - yellow_total);
    }


    void makeMove(const Piece piece, const Move& move) noexcept {
        const auto [x, y] = move.positions.back();
        get({x, y}) = piece;

        if (y == 7 or y == 0 /* and not piece.isShaikh() */) get({x, y}).promote();

        get(move.positions[0]).reset();

        for (const auto& [_, pos] : move.captures) get(pos).reset();

        nextTurn();
    }

    void unMakeMove(const Piece piece, const Move& move) noexcept {
        get(move.positions[0]) = piece;
        get(move.positions.back()).reset();

        for (const auto& [captured_piece, pos] : move.captures) get(pos) = captured_piece;

        nextTurn();
    }


    int minimax(int depth) {
        if (not depth) return evaluate();

        // populates the "possible_moves" member
        generateMoves();
        if (possible_moves.empty()) return -INF;




        const auto moves = std::move(possible_moves);
        int best = -INF;
        for (const auto& move : moves) {
            const auto piece = get(move.positions[0]);
            makeMove(piece, move);
            const int eval = -minimax(depth - 1);
            unMakeMove(piece, move);

            if (eval > best) best = eval;
        }

        return best;
    }

    int alphaBeta(int depth, int alpha = -INF, int beta = INF) {
        if (depth == 0) return evaluate();

        // populates the "possible_moves" member
        generateMoves();
        const auto moves = std::move(possible_moves);
        if (moves.empty()) return -INF;


        for (const auto& move : moves) {
            const auto piece = get(move.positions[0]);
            makeMove(piece, move);
            const int eval = -alphaBeta(depth - 1, -beta, -alpha);
            unMakeMove(piece, move);

            // if move is too good
            if (eval >= beta) return beta; // prune the brach!
            if (eval > alpha) alpha = eval;

        }

        return alpha;
    }


    Move bestMove() {
        generateMoves(); // populates possible_moves;

        const auto moves = std::move(possible_moves);

        Move best_move;
        int best_score = std::numeric_limits<int>::min();
        for (const auto& move : moves) {
            const auto piece = get(move.positions[0]);
            makeMove(piece, move);
            const auto score = -alphaBeta(4);
            unMakeMove(piece, move);

            if (score >= best_score) {
                best_score = score;
                best_move  =  move;
            }
        }

        std::clog << "score: " << best_score << '\n';
        return best_move;
    }



    Move AI_highlighs;
    size_t AI_inflight_idx{};
    void play(const Move& move) noexcept {
        SoundFX sfx = SoundFX::MOVE; // default sound anyway!


        // push the first move
        if (AI_inflight_idx == 0) {
            AI_highlighs.clear(); //?

            AI_highlighs.positions.push_back(move.positions[0]);
        }

        // moving stuff
        AI_highlighs.positions.push_back(move.positions[AI_inflight_idx + 1]);

        const auto piece = get(move.positions[AI_inflight_idx]);
        get(move.positions[AI_inflight_idx]).reset();
        const auto to = move.positions[AI_inflight_idx + 1];
        get(to) = piece;

        if (not get(to).isShaikh() and (to.y == 7 or to.y == 0)) {
            get(to).promote();
            sfx = SoundFX::PROMOTE;
        }

        if (move.doesCapture()) {
            AI_highlighs.captures.push_back(move.captures[AI_inflight_idx]);
            get(move.captures[AI_inflight_idx].pos).reset();

            if (sfx != SoundFX::PROMOTE) sfx = SoundFX::CAPTURE;
        }
        // done moving stuff

        play(sfx);
        AI_inflight_idx++;

        if (AI_inflight_idx == move.positions.size() - 1) {
            AI_inflight_idx = 0;
            nextTurn();
            generateMoves();
        }

        // makeMove(get(move.positions[0]), move);
        // play(SoundFX::MOVE); // AI will always play the move sound
    }

    bool AIInflight() const noexcept { return AI_inflight_idx != 0; }


    // 
    void parseFen(const std::string_view fen) noexcept {
        for(size_t x{}, y{}; char c : fen){
            switch(c){
                case 'y':
                    // pieces[index] = Piece{{x, y}, Piece::Color::YELLOW, false};
                    // board[y][x] = Piece{{x, y}, Piece::Color::YELLOW, false};
                    board[y][x] = Piece::Flags::ACTIVE | Piece::Flags::YELLOW;

                    ++x;
                    break;
                case 'Y':
                    // pieces[index] = Piece{{x, y}, Piece::Color::YELLOW, true};
                    // board[y][x] = Piece{{x, y}, Piece::Color::YELLOW, true};
                    board[y][x] = Piece::Flags::ACTIVE | Piece::Flags::YELLOW | Piece::Flags::SHAIKH;
                    ++x;
                    break;
                case 'b':
                    // pieces[index] = Piece{{x, y}, Piece::Color::BLACK, false};
                    // board[y][x] = Piece{{x, y}, Piece::Color::BLACK, false};
                    board[y][x] = Piece::Flags::ACTIVE; // active assumed to be black pawn
                    ++x;
                    break;
                case 'B':
                    // pieces[index] = Piece{{x, y}, Piece::Color::BLACK, true};
                    // board[y][x] = Piece{{x, y}, Piece::Color::BLACK, true};
                    board[y][x] = Piece::Flags::ACTIVE | Piece::Flags::SHAIKH;
                    ++x;
                    break;
                case '/':
                    ++y;
                    x = 0;
                    break;
                default:
                    x += size_t(c - '0');
                    break;
            }
        }
    }

    void prettyPrint() const {
        std::clog << "==================\n";

        for (const auto& row : board) {
            for (const auto piece : row) {
                piece.prettyPrint();
            }
            std::clog << '\n';
        }

        std::clog << "==================\n";
    }

    static void printMoves(const std::vector<Move>& moves) {
        for (auto&& [idx, move] : enumerate(moves)) {
            std::clog << '[' << idx << "]: ";
            move.prettyPrint();
        }
    }


    std::string fen() const {
        std::string out;

        for (size_t count; const auto& row : board) {
            count = 0;
            for (const auto& piece : row) {
                if (piece) {
                    if (count) out += std::to_string(count);

                    count = 0;
                    out += char(piece);
                }
                else ++count;
            }

            if (count) out += std::to_string(count);

            out += '/';
        }

        out.pop_back();
        return out;
    }


    enum class SoundFX { MOVE, CAPTURE, PROMOTE };

    static void play(SoundFX fx) {
        switch (fx) {
            case SoundFX::MOVE:    return MOVE_SFX   .play();
            case SoundFX::CAPTURE: return CAPTURE_SFX.play();
            case SoundFX::PROMOTE: return PROMOTE_SFX.play();
        }
    }


    [[clang::no_destroy]] const static inline sf::SoundBuffer    MOVE_BUFFER{SOUNDS_PATH "move.wav"   };
    [[clang::no_destroy]] const static inline sf::SoundBuffer CAPTURE_BUFFER{SOUNDS_PATH "capture.wav"};
    [[clang::no_destroy]] const static inline sf::SoundBuffer PROMOTE_BUFFER{SOUNDS_PATH "check.wav"  };

    // method ".play()" is non-const
    [[clang::no_destroy]] static inline sf::Sound    MOVE_SFX{MOVE_BUFFER   };
    [[clang::no_destroy]] static inline sf::Sound CAPTURE_SFX{CAPTURE_BUFFER};
    [[clang::no_destroy]] static inline sf::Sound PROMOTE_SFX{PROMOTE_BUFFER};
};
