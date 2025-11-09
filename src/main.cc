#include <SFML/Graphics.hpp>

#include <print>
#include <span>

#include "BoardState.hxx"
#include "Move.hxx"
#include "helper.hxx"


const static sf::Font font{"src/assets/Roboto-Regular.ttf"};
void numbering(sf::RenderWindow& window, const int x, const int y, const sf::Vector2f block_size) {
    sf::Text text{font};
    text.setCharacterSize(18);

    if (y == 7) {
        text.setString(std::to_string(x + 1));

        text.setPosition({x * block_size.x + block_size.x - 20 + 2, y * block_size.y + block_size.y - 25 + 2});
        text.setFillColor(sf::Color::Black);
        window.draw(text);

        text.setPosition({x * block_size.x + block_size.x - 20, y * block_size.y + block_size.y - 25});
        text.setFillColor(sf::Color::White);
        window.draw(text);
    }

    if (x == 0) {
        text.setString(char('H' - y));

        text.setPosition({x * block_size.x + 5 + 2, y * block_size.y + 1 + 2});
        text.setFillColor(sf::Color::Black);
        window.draw(text);

        text.setPosition({x * block_size.x + 5, y * block_size.y + 1});
        text.setFillColor(sf::Color::White);
        window.draw(text);
    }
}

void background(sf::RenderWindow& window, const std::span<Move> highlight) {
    const auto winsize = window.getSize();
    // std::clog << "windo size - x: " << winsize.x << ", y: " << winsize.y << '\n';

    const sf::Vector2f block_size{
        static_cast<float>(winsize.x / 8),
        static_cast<float>(winsize.y / 8)
    };
    // std::clog << "block size - x: " << block_size.x << ", y: " << block_size.y << '\n';


    sf::RectangleShape block{block_size};

    for (bool flag = true; auto y : range(8)) {
        for (auto x : range(8)) {
            block.setPosition({x * block_size.x, y * block_size.y});
            block.setFillColor(flag ? sf::Color{240, 201, 152} : sf::Color{107, 77, 64});

            window.draw(block);

            flag =! flag;
        }

        flag =! flag;
    }




    sf::Text text{font};
    text.setCharacterSize(18);
    for (const auto& move : highlight) {
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

    auto window = sf::RenderWindow(sf::VideoMode({720uz, 720uz}), "Dama AI", sf::Style::Close);
    sf::Image icon{"src/assets/icon.png"};
    window.setIcon(icon);

    window.setFramerateLimit(144);


    BoardState board;
    std::span<Move> highlights;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>() or sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
                window.close();
                exit(0);
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) board = BoardState{};
        }


        window.clear();

        background(window, highlights);
        board.render(window);
        highlights = board.update(window);

        if (board.done()) winningText(window, board.yellowCount() ? "Yellow wins!" : "Black wins!");

        window.display();
    }
}