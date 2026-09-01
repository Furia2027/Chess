#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <cctype>

using namespace std;
using namespace sf; // For simplify SFML formats

// Board Constants
const int TILE_SIZE = 80;
const int BOARD_SIZE = 8;
const unsigned int WINDOW_SIZE = TILE_SIZE * BOARD_SIZE; // 640x640 pixels

// Global array that creates 128 empty texture objects
Texture pieceTextures[128];

// Scoped Enumeration that represents the chess pieces' colours
enum class PieceColour {
    white,
    black,
    empty,
    invalid
};

enum class PieceType {
    pawn,
    rook,
    knight,
    bishop,
    queen,
    king,
    empty,
    invalid
};

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

PieceColour getPieceColour(int row, int col, const char board[8][8]) {
    // Check Board Grid Boundary
    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return PieceColour::invalid;
    }

    // Initialize Pieces
    char pieces = board[row][col];

    // Check Pieces States
    switch (pieces) {
        // Check for Empty Tiles
    case '.':
        return PieceColour::empty;
    default:
        // Check for Upper, Lower and Invalid Tiles
        if (isupper(pieces)) return PieceColour::white;
        if (islower(pieces)) return PieceColour::black;
        return PieceColour::invalid;
    }
}

PieceType getPieceType(int row, int col, const char board[8][8]) {
    // Initialize Pieces
    char pieces = board[row][col];
   
    // Check Board Grid Boundary
    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return PieceType::invalid;
    }

    // Check for Empty tiles
    if (pieces == '.') {
        return PieceType::empty;
    }

    // Convert all piece in board to lowercases
    char piecetype = tolower(pieces);

    // Map each to PieceType
    switch (piecetype) {
        case 'p': return PieceType::pawn;
        case 'r': return PieceType::rook;
        case 'n': return PieceType::knight;
        case 'b': return PieceType::bishop;
        case 'q': return PieceType::queen;
        case 'k': return PieceType::king;
        default: return PieceType::invalid;
    }
}

// Function to handle mouse input & moving pieces
void handleMouseClick(float mouseX, float mouseY, char board[8][8], int& selectedRow, int& selectedCol) {
    // Convert coordinates into array matrixes indices
    int col = static_cast<int>(mouseX) / TILE_SIZE;
    int row = static_cast<int>(mouseY) / TILE_SIZE;

    PieceColour clickedTarget = getPieceColour(row, col, board);

    // Ignore clicks outside the chess board
    if (clickedTarget == PieceColour::invalid) return;

    // Case 1: Nothing is Selected
    if (selectedRow == -1) {
        // Selects piece f a piece is clicked
        if (clickedTarget != PieceColour::empty) {
            selectedRow = row;
            selectedCol = col;
        }
    }
    // Case 2: A piece is selected
    else {
        PieceColour clickedColour = getPieceColour(selectedRow, selectedCol, board);
        // Check if the same tile is selected again
        if (selectedRow != row || selectedCol != col) {
            // Moves piece to the selected tile if a different tile is selected
            if (clickedColour != clickedTarget) {
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
    Color lightSquare(220, 220, 180);   // Light Green
    Color darkSquare(120, 145, 80);     // Dark Green
    Color highlight(245, 245, 0, 220);  // Highlight Yellow

    // Draws the board tiles with constant TILE_SIZE dimensions
    RectangleShape tile(Vector2f((float)TILE_SIZE, (float)TILE_SIZE));

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
                Vector2u size = pieceTextures[(int)piece].getSize();
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
    RenderWindow window(sf::VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "UTAR Chess");
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
            // Checks if the window's size is asjusted
            if (const auto* resized = event->getIf<Event::Resized>()) {
                // Convert new window's size from integer to float data type
                float w = (float)resized->size.x;
                float h = (float)resized->size.y;

                // Default to Full Window Size (100% width & height)
                FloatRect viewport({}, {1, 1});

                if (w > h) {
                    // Window is wider than height (Scales down new window's width)
                    viewport.size.x = h / w;
                    viewport.position.x = (1 - viewport.size.x) / 2; // Center the board horizontally
                }
                else {
                    // Window is taller than width (Scales down new window's height)
                    viewport.size.y = w / h;
                    viewport.position.y = (1 - viewport.size.y) / 2; // Center the board vertically
                }
                // 2D Camera locked to fixed 640*640 resolution
                View fixedView(FloatRect({}, {640, 640})); 
                // Assign calculated screen percentage bounds with black bars to camera view
                fixedView.setViewport(viewport);
                // Apply Camera view to window so graphics render at 1:1 aspect ratio
                window.setView(fixedView);
            }
            // Check for mouse click event
            if (const auto* click = event->getIf<Event::MouseButtonPressed>()) {
                // Check clicked mouse button (Left)
                if (click->button == Mouse::Button::Left) {
                    // Convert window's pixels to 640*640 game world coordinates
                    Vector2f worldPos = window.mapPixelToCoords(click->position);
                    // Pass converted board coordinates to select or move pieces
                    handleMouseClick(worldPos.x, worldPos.y, board, selectedRow, selectedCol);
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