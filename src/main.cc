#include <SFML/Graphics.hpp>

#include <print>
#include <cmath>
#include <span>
#include <chrono>

#include "BoardState.hxx"
#include "Move.hxx"
#include "helper.hxx"


constexpr sf::Color  DARK_TILE      = {107, 77 , 64 };
constexpr sf::Color LIGHT_TILE      = {240, 201, 152};

constexpr sf::Color  DARK_HIGHLIGHT = {53 , 181, 87 };
constexpr sf::Color LIGHT_HIGHLIGHT = {71 , 173, 98 };

constexpr sf::Color   RED_HIGHLIGHT = {189, 60 , 0  };
constexpr sf::Color  BLUE_HIGHLIGHT = {41 , 74 , 153};


[[clang::no_destroy]] static const sf::Font font{FONTS_PATH "Roboto-Regular.ttf"};


template <bool AI = false>
static void highlighMove(sf::RenderWindow& window, const Move& move, sf::RectangleShape& block , const sf::Vector2f block_size) {
    if (not move) return;

    sf::Text text{font, "", 18}; // text.setCharacterSize(18);

    const auto [x_start, y_start] = move.positions[0];
    block.setPosition({x_start * block_size.x, y_start * block_size.y});

    if constexpr (AI) block.setFillColor(BLUE_HIGHLIGHT);
    else              block.setFillColor( RED_HIGHLIGHT);

    window.draw(block);

    for (size_t step{1}; const auto [x, y] : move.positions | std::views::drop(1)) {
        block.setPosition({x * block_size.x, y * block_size.y});
        block.setFillColor((x + y) % 2 ? DARK_HIGHLIGHT : LIGHT_HIGHLIGHT);
        window.draw(block);


        if constexpr (not AI) {
            text.setString(std::to_string(step));

            // shadow
            text.setPosition({x * block_size.x + block_size.x - 20 + 2, y * block_size.y + 1 + 2});
            text.setFillColor(sf::Color::Black);
            window.draw(text);

            // text
            text.setPosition({x * block_size.x + block_size.x - 20    , y * block_size.y + 1    });
            text.setFillColor(sf::Color::White);
            window.draw(text);

            ++step;
        }
    }

    if constexpr (AI) {
        const auto [x_end, y_end] = move.positions.back();
        block.setPosition({x_end * block_size.x, y_end * block_size.y});
        block.setFillColor(RED_HIGHLIGHT);
        window.draw(block);
    }
}

static void highlight(
    sf::RenderWindow& window,
    const std::span<Move> moves, const std::span<Move> highlights, const Move& AI_highlight,
    sf::RectangleShape& block  , const sf::Vector2f block_size
) {


    constexpr auto AI_MOVE = true;
    highlighMove<AI_MOVE>(window, AI_highlight, block, block_size);

    for (const auto& move : highlights) highlighMove(window, move, block, block_size);

    if (highlights.empty() and not moves.empty()) {
        using std::chrono::operator""s;

        constexpr auto BLINKING_TIME = 0.5s;
        static auto next_switch = std::chrono::steady_clock::now() + BLINKING_TIME;

        if (const auto now = std::chrono::steady_clock::now(); now < next_switch) {
            for (const auto& move : moves) {
                if (not move.doesCapture()) continue;

                auto [x, y] = move.positions[0];
                block.setPosition({x * block_size.x, y * block_size.y});
                block.setFillColor((x + y) % 2 ? DARK_HIGHLIGHT : LIGHT_HIGHLIGHT);
                window.draw(block);
            }
        }
        else if (now > next_switch + BLINKING_TIME) next_switch = std::chrono::steady_clock::now() + BLINKING_TIME;

    }
}


static void numbering(sf::RenderWindow& window, const int x, const int y, const sf::Vector2f block_size) {
    sf::Text text{font};
    text.setCharacterSize(18);

    if (y == 7) {
        text.setString(char('A' + x));

        text.setPosition({x * block_size.x + block_size.x - 20 + 2, y * block_size.y + block_size.y - 25 + 2});
        text.setFillColor(sf::Color::Black);
        window.draw(text);

        text.setPosition({x * block_size.x + block_size.x - 20, y * block_size.y + block_size.y - 25});
        text.setFillColor(sf::Color::White);
        window.draw(text);
    }

    if (x == 0) {
        text.setString(std::to_string(8 - y));

        text.setPosition({x * block_size.x + 5 + 2, y * block_size.y + 1 + 2});
        text.setFillColor(sf::Color::Black);
        window.draw(text);

        text.setPosition({x * block_size.x + 5, y * block_size.y + 1});
        text.setFillColor(sf::Color::White);
        window.draw(text);
    }
}


static void background(sf::RenderWindow& window, const std::span<Move> moves, const std::span<Move> highlights, const Move& AI_highlights) {
    const auto winsize = window.getSize();
    // std::clog << "windo size - x: " << winsize.x << ", y: " << winsize.y << '\n';

    const sf::Vector2f block_size{
        static_cast<float>(winsize.x / 8),
        static_cast<float>(winsize.y / 8)
    };
    // std::clog << "block size - x: " << block_size.x << ", y: " << block_size.y << '\n';


    sf::RectangleShape block{block_size};

    for (bool flag{}; auto y : range(8)) {
        for (auto x : range(8)) {
            block.setPosition({x * block_size.x, y * block_size.y});
            //  : sf::Color
            block.setFillColor(flag ? LIGHT_TILE : DARK_TILE);

            window.draw(block);

            flag =! flag;
        }

        flag =! flag;
    }


    highlight(window, moves, highlights, AI_highlights, block, block_size);


    // effectivly O(1)
    for (auto x : range(8)) numbering(window, x, 7, block_size);
    for (auto y : range(8)) numbering(window, 0, y, block_size);

}


static void winningText(sf::RenderWindow& window, const std::string_view t) {
    sf::Text text{font};
    text.setString(t.data());
    text.setCharacterSize(64);
    text.setOutlineThickness(2);

    const sf::FloatRect textRect = text.getLocalBounds();
    text.setOrigin({textRect.position.x + textRect.size.x / 2, textRect.position.x + textRect.size.y / 2});
    const auto [width, height] = window.getSize();
    text.setPosition({static_cast<float>(width / 2), static_cast<float>(height / 2)});


    window.draw(text);
}


int main() {;
    using std::operator""sv;

    auto window = sf::RenderWindow(sf::VideoMode({720uz, 720uz}), "Dama AI", sf::Style::Close);

    sf::Image icon{IMAGES_PATH "icon2.png"};
    window.setIcon(icon);

    window.setFramerateLimit(144);


    BoardState board;
    // board = BoardState{"5B2/6b1/8/6b1/6y1/8/8/8"sv}; // TEST: move generation stops at promotion
    // board = BoardState{"8/8/8/3b4/5b2/8/yb1b1Y2/8"sv};  // TEST: picks longest move (without switching pieces mid move)
    // board = BoardState{"8/6b1/5b2/3b4/5b2/8/yb1b1Y2/8"sv}; // multiple inflight moves
    board = BoardState{"Y4Y1Y/8/1bbbbbbb/1bbb3b/1b4b1/4b3/2yyyyyy/8"sv}; // cool position it wins at
    // board = BoardState{"8/3b4/3b4/8/8/3y4/8/8"sv};

    std::span<Move> moves, highlights;


    Move AI_move;
    Move AI_highlights;


    bool previous_turn{};

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>() or sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
                window.close();
                exit(0);
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) board = BoardState{};

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) puts(board.fen().c_str());
        }

        window.clear();

        if (board.done()) {
            background(window, moves, highlights, AI_highlights);
            board.render(window);
            std::tie(moves, highlights, AI_highlights) = board.update(window);
            winningText(window, board.yellowCount() ? "Yellow wins!" : "Black wins!");
            window.display();
            continue;
        }

        background(window, moves, highlights, AI_highlights);


        if (previous_turn != board.turn) {
            std::clog << "Board Score: " << (1 - board.turn) * -1 * board.evaluate() << '\n';
            previous_turn = board.turn; // skip one frame 
        }
        else if (board.blackTurn()) {
            using std::chrono::operator""s;

            if (board.AIInflight())
                std::this_thread::sleep_for(.5s); // wait half a second between each move to make it clear whats going on
            else
                AI_move = board.bestMove(); // generate a new move

            // keep play the current move
            board.play(AI_move);
        }

        board.render(window);
        std::tie(moves, highlights, AI_highlights) = board.update(window);

        window.display();
    }
}

