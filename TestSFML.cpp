#include <SFML/Graphics.hpp>
#include <iostream>
#include <cctype>
#include <string>

const unsigned int TILE_SIZE = 80;
const unsigned int BOARD_SIZE = 8;
const unsigned int WINDOW_SIZE = TILE_SIZE * BOARD_SIZE;

// Function accepts the 8x8 board matrix by reference to update piece positions live
void displaySFMLChessUI(char board[8][8]) {
    sf::RenderWindow window(sf::VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "Interactive SFML 3 Chess");
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");

    // RGB Parameter (Red, Green, Blue, Alpha)
    sf::Color lightSquareColor(220, 220, 180);  // Light Green
    sf::Color darkSquareColor(120, 145, 80);    // Dark Green
    sf::Color highlightColor(255, 255, 0, 150); // Transparent Yellow

    sf::RectangleShape tile(sf::Vector2f({ static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE) }));

    // Selection Tracking State (-1 means no square selected)
    int selectedRow = -1;
    int selectedCol = -1;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            // 1. Capture Mouse Click Events
            else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    // Convert pixel coordinates (x, y) to board grid indices (col, row)
                    int c = mousePressed->position.x / TILE_SIZE;
                    int r = mousePressed->position.y / TILE_SIZE;

                    if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                        if (selectedRow == -1) {
                            // Step 1: Select a piece to move (must not be an empty tile)
                            if (board[r][c] != '.') {
                                selectedRow = r;
                                selectedCol = c;
                            }
                        }
                        else {
                            // Step 2: Deselect if clicking the same tile
                            if (selectedRow == r && selectedCol == c) {
                                selectedRow = -1;
                                selectedCol = -1;
                            }
                            // Step 3: Move piece to target destination in board array
                            else {
                                board[r][c] = board[selectedRow][selectedCol];
                                board[selectedRow][selectedCol] = '.';
                                selectedRow = -1;
                                selectedCol = -1;
                            }
                        }
                    }
                }
            }
        }

        window.clear();

        // 2. Render Grid, Highlights, and Pieces
        for (unsigned int r = 0; r < BOARD_SIZE; ++r) {
            for (unsigned int c = 0; c < BOARD_SIZE; ++c) {
                // Render Base Tile
                tile.setPosition({ static_cast<float>(c * TILE_SIZE), static_cast<float>(r * TILE_SIZE) });
                tile.setFillColor(((r + c) % 2 == 0) ? lightSquareColor : darkSquareColor);
                window.draw(tile);

                // Render Yellow Highlight on Selected Square
                if (static_cast<int>(r) == selectedRow && static_cast<int>(c) == selectedCol) {
                    tile.setFillColor(highlightColor);
                    window.draw(tile);
                }

                // Render Piece Glyph
                char pieceSymbol = board[r][c];
                if (pieceSymbol != '.' && fontLoaded) {
                    sf::Text text(font, std::string(1, pieceSymbol), 48);

                    if (std::isupper(pieceSymbol)) {
                        text.setFillColor(sf::Color::White);
                        text.setOutlineColor(sf::Color::Black);
                        text.setOutlineThickness(2.0f);
                    }
                    else {
                        text.setFillColor(sf::Color::Black);
                        text.setOutlineColor(sf::Color::White);
                        text.setOutlineThickness(1.5f);
                    }

                    text.setPosition({ static_cast<float>(c * TILE_SIZE + 24), static_cast<float>(r * TILE_SIZE + 8) });
                    window.draw(text);
                }
            }
        }

        window.display();
    }
}

int main() {
    char board[8][8] = {
        {'r','n','b','q','k','b','n','r'},
        {'p','p','p','p','p','p','p','p'},
        {'.','.','.','.','.','.','.','.'},
        {'.','.','.','.','.','.','.','.'},
        {'.','.','.','.','.','.','.','.'},
        {'.','.','.','.','.','.','.','.'},
        {'P','P','P','P','P','P','P','P'},
        {'R','N','B','Q','K','B','N','R'}
    };

    displaySFMLChessUI(board);
    return 0;
}