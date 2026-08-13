#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

using namespace std;

// Board Constants
const int TILE_SIZE = 80;
const int BOARD_SIZE = 8;
const unsigned int WINDOW_SIZE = TILE_SIZE * BOARD_SIZE; // 640x640 pixels

// Global array that creates 128 empty texture objects
sf::Texture pieceTextures[128];

// Function to load all 12 chess piece images
bool loadTextures() {
    char pieces[] = { 'P', 'p', 'R', 'r', 'N', 'n', 'B', 'b', 'Q', 'q', 'K', 'k' };

    string filenames[] = {
        "assets/W_Pawn.png",   "assets/B_Pawn.png",
        "assets/W_Rook.png",   "assets/B_Rook.png",
        "assets/W_Knight.png", "assets/B_Knight.png",
        "assets/W_Bishop.png", "assets/B_Bishop.png",
        "assets/W_Queen.png",  "assets/B_Queen.png",
        "assets/W_King.png",   "assets/B_King.png"
    };

    // Load 12 Images into pieces[] array
    for (int i = 0; i < 12; i++) {
        // Checks if image files are loaded
        if (!pieceTextures[(int)pieces[i]].loadFromFile(filenames[i])) {
            cout << "Error: Could not load " << filenames[i] << endl;
            return false;
        }
        // Loads image and enables bilinear filtering on image texture
        pieceTextures[(int)pieces[i]].setSmooth(true);
    }
    return true;
}

// Function to handle mouse input & moving pieces
void handleMouseClick(int mouseX, int mouseY, char board[8][8], int& selectedRow, int& selectedCol) {
    // Convert coordinates into array matrixes indices
    int col = mouseX / TILE_SIZE;
    int row = mouseY / TILE_SIZE;

    // Ignore clicks outside the chess board
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return;

    // Case 1: Nothing is Selected
    if (selectedRow == -1) {
        // Selects piece f a piece is clicked
        if (board[row][col] != '.') {
            selectedRow = row;
            selectedCol = col;
        }
    }
    // Case 2: A piece is selected
    else {
        // Check if the same tile is selected again
        if (selectedRow != row || selectedCol != col) {
            // Moves piece to the selected tile if a different tile is selected
            if (selectedRow >= 0 && selectedRow < 8 && selectedCol >= 0 && selectedCol < 8) {
                board[row][col] = board[selectedRow][selectedCol];
                board[selectedRow][selectedCol] = '.';
            }
        }
        // Resets selection
        selectedRow = -1;
        selectedCol = -1;
    }
}

// Function to display the window with the board
void renderGame(sf::RenderWindow& window, char board[8][8], int selectedRow, int selectedCol) {
    // Defines color of the board tiles with RGBA color channels
    sf::Color lightSquare(220, 220, 180);   // Light Green
    sf::Color darkSquare(120, 145, 80);     // Dark Green
    sf::Color highlight(245, 245, 0, 220);  // Highlight Yellow

    // Draws the board tiles with constant TILE_SIZE dimensions
    sf::RectangleShape tile(sf::Vector2f((float)TILE_SIZE, (float)TILE_SIZE));

    // Erase rendered previous frames
    window.clear();

    // Loop through board grid
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {

            // Configures the board tiles' color and position on the board
            tile.setPosition({ (float)c * TILE_SIZE, (float)r * TILE_SIZE });
            tile.setFillColor(((r + c) % 2 == 0) ? lightSquare : darkSquare);

            // Apply highlight color on selected tile
            if (r == selectedRow && c == selectedCol) {
                tile.setFillColor(highlight);
            }
            // Draws Tiles with the configured tile colour and size
            window.draw(tile);

            char piece = board[r][c];
            if (piece != '.') {
                // Binds piece image onto the tile if it is not empty (.)
                sf::Sprite sprite(pieceTextures[(int)piece]);

                // Auto-scale chess piece image to match tile size (80x80)
                sf::Vector2u size = pieceTextures[(int)piece].getSize();
                sprite.setScale({ (float)TILE_SIZE / size.x, (float)TILE_SIZE / size.y });
                sprite.setPosition({ (float)c * TILE_SIZE, (float)r * TILE_SIZE });

                // Draws Chess Piece with the image
                window.draw(sprite);
            }
        }
    }
    // Takes data from windows.draw() and display the window
    window.display();
}

// Function to initialize window and run the game loop
void runGame(char board[8][8]) {
    // Wait until texture is loaded
    if (!loadTextures()) 
        return;

    // Create the window with the loaded textures
    sf::RenderWindow window(sf::VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "UTAR Chess");
    window.setFramerateLimit(60);

    // Declare for mouse clicks
    int selectedRow = -1;
    int selectedCol = -1;

    // Game loop
    while (window.isOpen()) {
        // Receive user interactions from windows's OS queue
        while (const auto event = window.pollEvent()) {
            // Checks if polled window event is a close request
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            // Check for mouse click event
            if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
                // Check clicked mouse button (Left)
                if (click->button == sf::Mouse::Button::Left) {
                    handleMouseClick(click->position.x, click->position.y, board, selectedRow, selectedCol);
                }
            }
        }
        // Display chess board inside the window
        renderGame(window, board, selectedRow, selectedCol);
    }
}

int main() {
    // Board Array
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