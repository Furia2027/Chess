#include <SFML/Graphics.hpp>
#include <iostream>
#include <cctype>
#include <string>
#include <unordered_map>

using namespace std;

// Board Constants
const unsigned int TILE_SIZE = 80;   // Height and width
const unsigned int BOARD_SIZE = 8;    // 8x8 grid dimension
const unsigned int WINDOW_SIZE = TILE_SIZE * BOARD_SIZE; // 640x640 pixels

// Function to load chess piece images
bool loadTextures(std::unordered_map<char, sf::Texture>& pieceTextures) {
    std::unordered_map<char, std::string> fileMap = {
        {'P', "assets/W_Pawn.png"},   {'p', "assets/B_Pawn.png"},
        {'R', "assets/W_Rook.png"},   {'r', "assets/B_Rook.png"},
        {'N', "assets/W_Knight.png"}, {'n', "assets/B_Knight.png"},
        {'B', "assets/W_Bishop.png"}, {'b', "assets/B_Bishop.png"},
        {'Q', "assets/W_Queen.png"},  {'q', "assets/B_Queen.png"},
        {'K', "assets/W_King.png"},   {'k', "assets/B_King.png"}
    };

    for (const auto& [pieceChar, filePath] : fileMap) {
        if (!pieceTextures[pieceChar].loadFromFile(filePath)) {
            return false;
        }
        pieceTextures[pieceChar].setSmooth(true);
    }
    return true;
}

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
void renderGame(sf::RenderWindow& window, char board[8][8], int selectedRow, int selectedCol, const std::unordered_map<char, sf::Texture>& pieceTextures) {
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

            // Draw chess piece image if tile is not empty
            char pieceSymbol = board[r][c];
            if (pieceSymbol != '.') {
                auto it = pieceTextures.find(pieceSymbol);
                if (it != pieceTextures.end()) {
                    sf::Sprite sprite(it->second);

                    // Auto-scale sprite image to match grid tile size (80x80)
                    sf::Vector2u imageSize = it->second.getSize();
                    sprite.setScale({
                        static_cast<float>(TILE_SIZE) / imageSize.x,
                        static_cast<float>(TILE_SIZE) / imageSize.y
                        });

                    // Position piece image
                    sprite.setPosition({ static_cast<float>(c * TILE_SIZE), static_cast<float>(r * TILE_SIZE) });
                    window.draw(sprite);
                }
            }
        }
    }

    window.display(); // Display rendered frame on screen
}

// Initialize SFML objects and run render loop
void runGame(char board[8][8]) {
    sf::RenderWindow window(sf::VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "Chess");
    window.setFramerateLimit(60);

    std::unordered_map<char, sf::Texture> pieceTextures;
    if (!loadTextures(pieceTextures)) {
        cerr << "Error: Could not load all chess piece textures!" << endl;
        return;
    }

    int selectedRow = -1; // -1 indicates no active selection
    int selectedCol = -1;

    // Continuous Main Loop (runs until window is closed)
    while (window.isOpen()) {
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
        renderGame(window, board, selectedRow, selectedCol, pieceTextures);
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