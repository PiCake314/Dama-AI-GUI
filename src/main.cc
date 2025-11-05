#include <SFML/Graphics.hpp>

#include <print>

#include "BoardState.hxx"
#include "helper.hxx"



void background(sf::RenderWindow& window) {
    sf::Font font{"src/assets/Roboto-Regular.ttf"};
    sf::Text text{font};
    text.setCharacterSize(18);

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

            flag =! flag;
        }

        flag =! flag;
    }

}


int main() {

    auto window = sf::RenderWindow(sf::VideoMode({720uz, 720uz}), "Dama AI"); // sf::Style::Close

    window.setFramerateLimit(144);

    BoardState board;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>() or sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
                window.close();
                exit(0);
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) board = BoardState{};
        }


        window.clear();

        background(window);
        board.render(window);
        board.update(window);

        window.display();
    }
}