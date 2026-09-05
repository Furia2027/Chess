#include <iostream>
#include <string>
#include <limits>

using namespace std;

// Function to display the simplified menu options
void displayMenu() {
    cout << "\n====================================\n";
    cout << "             CHESS SYSTEM           \n";
    cout << "====================================\n";
    cout << "1. Start\n";
    cout << "2. Load game\n";
    cout << "3. Display move history\n";
    cout << "4. Exit\n";
    cout << "====================================\n";
    cout << "Enter your choice (1-4): ";
}

// part for the gameplay
void displayBoard() {
    cout << "\n--- CHESS BOARD ---\n";
    cout << "8 [r][n][b][q][k][b][n][r]\n";
    cout << "7 [p][p][p][p][p][p][p][p]\n";
    cout << "6 [ .][ .][ .][ .][ .][ .][ .][ .]\n";
    cout << "5 [ .][ .][ .][ .][ .][ .][ .][ .]\n";
    cout << "4 [ .][ .][ .][ .][ .][ .][ .][ .]\n";
    cout << "3 [ .][ .][ .][ .][ .][ .][ .][ .]\n";
    cout << "2 [P][P][P][P][P][P][P][P]\n";
    cout << "1 [R][N][B][Q][K][B][N][R]\n";
    cout << "   a  b  c  d  e  f  g  h\n";
}

int main() {
    string groupName;

    // Prompt user for group name to start
    cout << "====================================\n";
    cout << "   WELCOME TO THE CHESS SYSTEM      \n";
    cout << "====================================\n";
    cout << "Enter group name to start: ";
    getline(cin, groupName);

    // Handle empty input gracefully
    while (groupName.empty()) {
        cout << "Group name cannot be empty. Please enter group name: ";
        getline(cin, groupName);
    }

    cout << "\nWelcome, Team [" << groupName << "]!\n";

    int choice = 0;
    bool keepRunning = true;

    // Main event loop for navigation
    while (keepRunning) {
        displayMenu();

        // Input parsing and validation step
        if (!(cin >> choice)) {
            cout << "\n[ERROR] Invalid input. Please enter a number between 1 and 4.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        // Process the choice
        switch (choice) {
        case 1:
            // Triggers action to start the game
            cout << "\n[ACTION] Game Started!\n";
            break;
        case 2:
            // Loads game and displays the board
            cout << "\n[ACTION] Loading game...\n";
            displayBoard();
            break;
        case 3:
            // template for Outputs move history
            cout << "\n[ACTION] Displaying move history...\n";
            cout << "1. e2 -> e4\n";
            cout << "2. e7 -> e5\n";
            break;
        case 4:
            // Terminate program loop cleanly
            cout << "\nExiting system. Goodbye, Team " << groupName << "!\n";
            keepRunning = false;
            break;
        default:
            cout << "\n[ERROR] Choice out of range. Please enter a number between 1 and 4.\n";
            break;
        }
    }

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <cctype>
#include <fstream>  // Handles reading (ifstream) and writing (ofstream) files

using namespace std;
using namespace sf; // For simplify SFML formats

// Board Constants
const int TILE_SIZE = 80;
const int BOARD_SIZE = 8;
const unsigned int WINDOW_SIZE = TILE_SIZE * BOARD_SIZE; // 640x640 pixels
const int ROWS = 8;
const int COLS = 8;

// Context Menu Dimensions
const float MENU_WIDTH = 150.f;     // Width of context menu box in pixels
const float OPTION_HEIGHT = 35.f;   // Height of each menu option in pixels
const int TOTAL_OPTIONS = 3;        // Number of options (Save, Load, Log Stats)

// Global array that creates 128 empty texture objects
Texture pieceTextures[128];
Font menuFont; // Holds font style for context menu text labels

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

// Opens save file and writes total move count and 8x8 board matrix
bool saveGame(string filename, char board[8][8], int moves) {
    ofstream file(filename); // Open output file stream
    if (!file.is_open()) {
        cout << "Error: Could not open file for saving." << endl;
        return false;
    }
    file << moves << endl; // Write move count on line 1
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) file << board[r][c] << " "; // Write grid characters
        file << endl;
    }
    file.close(); // Close file stream
    cout << "Game successfully saved to " << filename << endl;
    return true;
}

// Reads saved move count and loads board layout back into memory grid
bool loadGame(string filename, char board[8][8], int& moves) {
    ifstream file(filename); // Open input file stream
    if (!file.is_open()) {
        cout << "Error: Save file not found." << endl;
        return false;
    }
    file >> moves; // Read move count into referenced variable
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) file >> board[r][c]; // Overwrite board grid
    }
    file.close(); // Close file stream
    cout << "Game successfully loaded from " << filename << endl;
    return true;
}

// Appends match outcome and total move count to match history log
bool saveGameStats(string filename, string winner, int moves) {
    ofstream file(filename, ios::app); // Open in append mode to preserve history
    if (!file.is_open()) {
        cout << "Error: Could not save match statistics." << endl;
        return false;
    }
    file << "Winner: " << winner << " | Total Moves: " << moves << endl; // Write log entry
    file.close(); // Close file stream
    cout << "Match stats recorded to history log." << endl;
    return true;
}

// Function to load all 12 chess piece images and menu font
bool loadTextures() {
    // Loads font file required for rendering context menu options
    if (!menuFont.openFromFile("arial.ttf")) {
        cout << "Warning: Could not load assets/arial.ttf font." << endl;
    }

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
void handleMouseClick(float mouseX, float mouseY, char board[8][8], int& selectedRow, int& selectedCol, int& moves) {
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
                moves++; // Increment move count on valid move
            }
        }
        // Resets selection
        selectedRow = -1;
        selectedCol = -1;
    }
}

// Function to display the window with the board, context menu, and fade notification
void renderGame(sf::RenderWindow& window, char board[8][8], int selectedRow, int selectedCol, bool isMenuOpen, Vector2f menuPos, const string& notificationMsg, const sf::Clock& notificationClock) {
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

    // Draw context menu options box when right-clicked
    if (isMenuOpen) {
        string optionLabels[TOTAL_OPTIONS] = { "Save Game", "Load Game", "Log Stats" };

        // Outer menu background box
        RectangleShape menuBox(Vector2f(MENU_WIDTH, OPTION_HEIGHT * TOTAL_OPTIONS));
        menuBox.setPosition(menuPos);
        menuBox.setFillColor(Color(40, 40, 40));
        menuBox.setOutlineColor(Color::White);
        menuBox.setOutlineThickness(1.f);
        window.draw(menuBox);

        // Draw text options directly inside menu
        for (int i = 0; i < TOTAL_OPTIONS; i++) {
            Text optionText(menuFont, optionLabels[i], 14);
            optionText.setFillColor(Color::White);
            optionText.setPosition({ menuPos.x + 10.f, menuPos.y + (i * OPTION_HEIGHT) + 8.f });
            window.draw(optionText);
        }
    }

    // Render temporary visual toast message after file action (solid popup for 1.5 seconds)
    if (notificationClock.getElapsedTime().asSeconds() < 1.5f && !notificationMsg.empty()) {
        // Popup box centered on screen
        RectangleShape toastBox(Vector2f(220.f, 45.f));
        toastBox.setPosition({ (WINDOW_SIZE - 220.f) / 2.f, (WINDOW_SIZE - 45.f) / 2.f });
        toastBox.setFillColor(Color(20, 20, 20));
        toastBox.setOutlineColor(Color::White);
        toastBox.setOutlineThickness(1.5f);
        window.draw(toastBox);

        // Notification text label
        Text toastText(menuFont, notificationMsg, 16);
        toastText.setFillColor(Color::White);
        toastText.setPosition({ (WINDOW_SIZE - 220.f) / 2.f + 40.f, (WINDOW_SIZE - 45.f) / 2.f + 12.f });
        window.draw(toastText);
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
    int moves = 0; // Move counter tracking total turns

    // Track context menu visibility and cursor position
    bool isMenuOpen = false;
    Vector2f menuPos(0.f, 0.f);

    // Track notification message string and timer
    string notificationMsg = "";
    sf::Clock notificationClock;

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
                FloatRect viewport({}, { 1, 1 });

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
                View fixedView(FloatRect({}, { 640, 640 }));
                // Assign calculated screen percentage bounds with black bars to camera view
                fixedView.setViewport(viewport);
                // Apply Camera view to window so graphics render at 1:1 aspect ratio
                window.setView(fixedView);
            }
            // Check for mouse click event
            if (const auto* click = event->getIf<Event::MouseButtonPressed>()) {
                // Convert window's pixels to 640*640 game world coordinates
                Vector2f worldPos = window.mapPixelToCoords(click->position);

                // Detect right-click to open context menu at cursor position
                if (click->button == Mouse::Button::Right) {
                    isMenuOpen = true;
                    menuPos = worldPos;
                }
                // Handle left-click inside menu or board grid
                else if (click->button == Mouse::Button::Left) {
                    if (isMenuOpen) {
                        // Verify left-click was inside context menu bounds
                        if (worldPos.x >= menuPos.x && worldPos.x <= menuPos.x + MENU_WIDTH &&
                            worldPos.y >= menuPos.y && worldPos.y <= menuPos.y + (OPTION_HEIGHT * TOTAL_OPTIONS)) {

                            // Calculate clicked menu row (0, 1, or 2)
                            int option = static_cast<int>((worldPos.y - menuPos.y) / OPTION_HEIGHT);

                            switch (option) {
                            case 0: // Save option selected
                                if (saveGame("savegame.txt", board, moves)) {
                                    notificationMsg = "Game Saved!";
                                    notificationClock.restart();
                                }
                                break;
                            case 1: // Load option selected
                                if (loadGame("savegame.txt", board, moves)) {
                                    notificationMsg = "Game Loaded!";
                                    notificationClock.restart();
                                }
                                break;
                            case 2: // Log stats option selected
                                if (saveGameStats("game_stats.txt", "Match_End", moves)) {
                                    notificationMsg = "Stats Logged!";
                                    notificationClock.restart();
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        isMenuOpen = false; // Close context menu after clicking
                    }
                    else {
                        // Pass converted board coordinates to select or move pieces
                        handleMouseClick(worldPos.x, worldPos.y, board, selectedRow, selectedCol, moves);
                    }
                }
            }
        }
        // Display chess board inside the window
        renderGame(window, board, selectedRow, selectedCol, isMenuOpen, menuPos, notificationMsg, notificationClock);
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