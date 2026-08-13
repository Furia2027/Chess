#include <SFML/Graphics.hpp>
#include <iostream>
#include <cctype>
#include <string>

// Board Constants
const unsigned int TILE_SIZE = 80;   // Height and width
const unsigned int BOARD_SIZE = 8;    // 8x8 grid dimension
const unsigned int WINDOW_SIZE = TILE_SIZE * BOARD_SIZE; // 640x640 pixels

// Function to handle mouse input & moving pieces
void handleMouseClick(int mouseX, int mouseY, char board[8][8], int& selectedRow, int& selectedCol) {
    // Convert pixel coordinates (e.g., 240px) into matrix array indices
    int col = mouseX / TILE_SIZE;
    int row = mouseY / TILE_SIZE;

    // Ensure mouse click inside board
    if (row >= 0 && row < 8 && col >= 0 && col < 8) {

        // No piece is selected yet
        if (selectedRow == -1) {
            // Select if clicked area is a piece
            if (board[row][col] != '.') {
                selectedRow = row;
                selectedCol = col;
            }
        }
        // A piece is selected
        else {
            // Deselect if same tile is clicked
            if (selectedRow == row && selectedCol == col) {
                selectedRow = -1;
                selectedCol = -1;
            }
            // Move piece to another tile
            else {
                board[row][col] = board[selectedRow][selectedCol]; // Copy piece to target tile
                board[selectedRow][selectedCol] = '.';             // Clear old source tile
                selectedRow = -1;                                  // Reset selection state
                selectedCol = -1;
            }
        }
    }
}

// Function to handle graphics rendering
void renderGame(sf::RenderWindow& window, char board[8][8], int selectedRow, int selectedCol, const sf::Font& font, bool fontLoaded) {
    sf::Color lightSquare(220, 220, 180);   // Light Green
    sf::Color darkSquare(120, 145, 80);     // Dark Green
    sf::Color highlight(255, 255, 0, 150);  // Transparent Yellow

    sf::RectangleShape tile(sf::Vector2f({ static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE) }));

    window.clear(); // Erase previous frame

    // Loop for Row (0-7) & Column (0-7)
    for (unsigned int r = 0; r < BOARD_SIZE; ++r) {
        for (unsigned int c = 0; c < BOARD_SIZE; ++c) {

            // Position and Draw background tile
            tile.setPosition({ static_cast<float>(c * TILE_SIZE), static_cast<float>(r * TILE_SIZE) });
            tile.setFillColor(((r + c) % 2 == 0) ? lightSquare : darkSquare);
            window.draw(tile);

            // Highlight color if a tile is selected
            if (static_cast<int>(r) == selectedRow && static_cast<int>(c) == selectedCol) {
                tile.setFillColor(highlight);
                window.draw(tile);
            }

            // Draw chess piece text character if tile is not empty
            char pieceSymbol = board[r][c];
            if (pieceSymbol != '.' && fontLoaded) {
                sf::Text text(font, std::string(1, pieceSymbol), 48);

                
                if (std::isupper(pieceSymbol)) {    // For Uppercase = White Piece
                    text.setFillColor(sf::Color::White);
                    text.setOutlineColor(sf::Color::Black);
                    text.setOutlineThickness(2.0f);
                }
                else {                              // For Lowercase = Black Piece
                    text.setFillColor(sf::Color::Black);
                    text.setOutlineColor(sf::Color::White);
                    text.setOutlineThickness(1.5f);
                }

                text.setPosition({ static_cast<float>(c * TILE_SIZE + 24), static_cast<float>(r * TILE_SIZE + 8) });
                window.draw(text);
            }
        }
    }

    window.display(); // Display rendered frame on screen
}

// Initialize SFML objects and run render loop
void runGame(char board[8][8]) {
    sf::RenderWindow window(sf::VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "Chess");
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded = font.openFromFile("arial.ttf");

    int selectedRow = -1; // -1 indicates no active selection
    int selectedCol = -1;

    // Continuous Main Loop (runs until window is closed)
    while (window.isOpen()) {

        // Event Processing Stage
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    handleMouseClick(mousePressed->position.x, mousePressed->position.y, board, selectedRow, selectedCol);
                }
            }
        }

        // Rendering Stage
        renderGame(window, board, selectedRow, selectedCol, font, fontLoaded);
    }
}

// Main Function
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

    runGame(board);
    return 0;
}