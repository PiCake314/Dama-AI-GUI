#include <SFML/Graphics.hpp>

#include <print>
#include <cmath>
#include <span>
#include <chrono>

#include "BoardState.hxx"
#include "Move.hxx"
#include "helper.hxx"

using std::chrono::operator""s;

/* Colors!
Dark  block
Light block

Dark piece
Light piece
Crown 


Capture blink
"from"
"to"
*/


constexpr sf::Color DARK_COLOR =  {107, 77, 64};
constexpr sf::Color LIGHT_COLOR = {240, 201, 152};


static const sf::Font font{"src/assets/Roboto-Regular.ttf"};

constexpr auto BLINKING_TIME = 0.5s;

void highlight(
    sf::RenderWindow& window,
    const std::span<Move> moves, const std::span<Move> highlights,
    sf::RectangleShape& block  , const sf::Vector2f block_size
) {

    if (highlights.size()) {
        sf::Text text{font};
        text.setCharacterSize(18);
        for (const auto& move : highlights) {
            {
                auto [x, y] = move.positions[0];
                block.setPosition({x * block_size.x, y * block_size.y});
                block.setFillColor({189, 60, 0});
                window.draw(block);
            }

            for (size_t step{1}; const auto [x, y] : move.positions | std::views::drop(1)) {
                block.setPosition({x * block_size.x, y * block_size.y});
                block.setFillColor((x + y) % 2 ? sf::Color{53, 181, 87} : sf::Color{71, 173, 98});
                window.draw(block);


                text.setString(std::to_string(step));

                text.setPosition({x * block_size.x + block_size.x - 20 + 2, y * block_size.y + 1 + 2});
                text.setFillColor(sf::Color::Black);
                window.draw(text);

                text.setPosition({x * block_size.x + block_size.x - 20    , y * block_size.y + 1    });
                text.setFillColor(sf::Color::White);
                window.draw(text);

                ++step;
            }
        }
    }

    else if (moves.size()) {
        static auto next_switch = std::chrono::steady_clock::now() + BLINKING_TIME;

        if (const auto now = std::chrono::steady_clock::now(); now < next_switch) {
            for (const auto& move : moves) {
                if (not move.doesCapture()) continue;

                auto [x, y] = move.positions[0];
                block.setPosition({x * block_size.x, y * block_size.y});
                // (x + y) % 2 ? sf::Color{53, 181, 87} : sf::Color{71, 173, 98}
                // block.setFillColor({255, 104, 26}); // {}
                block.setFillColor((x + y) % 2 ? sf::Color{53, 181, 87} : sf::Color{71, 173, 98});
                window.draw(block);
            }
        }
        else if (now > next_switch + BLINKING_TIME) next_switch = std::chrono::steady_clock::now() + BLINKING_TIME;

    }
}


void numbering(sf::RenderWindow& window, const int x, const int y, const sf::Vector2f block_size) {
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

void background(sf::RenderWindow& window, const std::span<Move> moves, const std::span<Move> highlights) {
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
            block.setFillColor(flag ? LIGHT_COLOR : DARK_COLOR);

            window.draw(block);

            flag =! flag;
        }

        flag =! flag;
    }


    highlight(window, moves, highlights, block, block_size);


    // effectivly O(1)
    for (auto x : range(8)) numbering(window, x, 7, block_size);
    for (auto y : range(8)) numbering(window, 0, y, block_size);

}


void winningText(sf::RenderWindow& window, const std::string_view t) {
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

int main() {
    using std::operator""sv;

    auto window = sf::RenderWindow(sf::VideoMode({720uz, 720uz}), "Dama AI", sf::Style::Close);

    sf::Image icon{"src/assets/icon2.png"};
    window.setIcon(icon);

    window.setFramerateLimit(144);


    BoardState board;
    // board = BoardState{"2B5/6b1/8/6b1/6y1/8/8/8"sv, true};
    board = BoardState{"Y4Y1Y/8/1bbbbbbb/1bbb3b/1b4b1/4b3/2yyyyyy/8"sv, true};
    // board = BoardState{"8/8/8/3b4/5b2/8/yb1b1Y2/8"sv, true};
    // board = BoardState{"8/6b1/5b2/3b4/5b2/8/yb1b1Y2/8"sv, true};

    std::span<Move> moves, highlights;

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


        if (previous_turn != board.turn) {
            puts(board.yellowTurn() ? "Yellow's Turn:" : "Black's Turn:");
            previous_turn = board.turn;
        }

        background(window, moves, highlights);

        if (board.blackTurn()) {
            const auto move = board.bestMove();
            move.prettyPrint();
            board.play(move);
        }

        board.render(window);
        std::tie(moves, highlights) = board.update(window);

        if (board.done()) winningText(window, board.yellowCount() ? "Yellow wins!" : "Black wins!");

        window.display();
    }
}