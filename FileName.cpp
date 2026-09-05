#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <cctype>
#include <fstream>
#include <limits>
#include <cstring>

using namespace std;
using namespace sf;

// ============================================================================
// CONSTANTS & GLOBAL RESOURCES
// ============================================================================

const int TILE_SIZE = 80;
const int BOARD_SIZE = 8;
const unsigned int WINDOW_SIZE = TILE_SIZE * BOARD_SIZE; // 640x640 pixels
const int ROWS = 8;
const int COLS = 8;

const float MENU_WIDTH = 150.f;
const float OPTION_HEIGHT = 35.f;
const int TOTAL_OPTIONS = 3;

// Default standard starting layout for Chess
const char INITIAL_BOARD[8][8] = {
    {'r','n','b','q','k','b','n','r'},
    {'p','p','p','p','p','p','p','p'},
    {'.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.'},
    {'P','P','P','P','P','P','P','P'},
    {'R','N','B','Q','K','B','N','R'}
};

Texture pieceTextures[128];
Font menuFont;

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

// ============================================================================
// FILE HANDLING OPERATIONS
// ============================================================================

bool saveGame(const string& filename, char board[8][8], int moves) {
    ofstream file(filename);
    if (!file.is_open()) {
        cout << "[ERROR] Could not open file for saving: " << filename << endl;
        return false;
    }
    file << moves << endl;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            file << board[r][c] << " ";
        }
        file << endl;
    }
    file.close();
    cout << "[SUCCESS] Game successfully saved to " << filename << endl;
    return true;
}

bool loadGame(const string& filename, char board[8][8], int& moves) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "[ERROR] Save file not found: " << filename << endl;
        return false;
    }
    file >> moves;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            file >> board[r][c];
        }
    }
    file.close();
    cout << "[SUCCESS] Game successfully loaded from " << filename << endl;
    return true;
}

bool saveGameStats(const string& filename, const string& winner, int moves) {
    ofstream file(filename, ios::app);
    if (!file.is_open()) {
        cout << "[ERROR] Could not record match statistics." << endl;
        return false;
    }
    file << "Winner: " << winner << " | Total Moves: " << moves << endl;
    file.close();
    cout << "[SUCCESS] Match stats recorded to history log." << endl;
    return true;
}

void displayStatsHistory(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "\n[INFO] No match history records found (" << filename << ").\n";
        return;
    }
    cout << "\n--- MATCH STATS HISTORY ---\n";
    string line;
    while (getline(file, line)) {
        cout << line << endl;
    }
    file.close();
}

// ============================================================================
// GAME UTILITIES & PIECE LOGIC
// ============================================================================

bool loadTextures() {
    if (!menuFont.openFromFile("arial.ttf")) {
        cout << "[WARNING] Could not load arial.ttf font." << endl;
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

    for (int i = 0; i < 12; i++) {
        if (!pieceTextures[(int)pieces[i]].loadFromFile(filenames[i])) {
            cout << "[ERROR] Could not load asset: " << filenames[i] << endl;
            return false;
        }
        pieceTextures[(int)pieces[i]].setSmooth(true);
    }
    return true;
}

PieceColour getPieceColour(int row, int col, const char board[8][8]) {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return PieceColour::invalid;
    }

    char piece = board[row][col];
    if (piece == '.') return PieceColour::empty;
    if (isupper(piece)) return PieceColour::white;
    if (islower(piece)) return PieceColour::black;

    return PieceColour::invalid;
}

PieceType getPieceType(int row, int col, const char board[8][8]) {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return PieceType::invalid;
    }

    char piece = board[row][col];
    if (piece == '.') return PieceType::empty;

    switch (tolower(piece)) {
    case 'p': return PieceType::pawn;
    case 'r': return PieceType::rook;
    case 'n': return PieceType::knight;
    case 'b': return PieceType::bishop;
    case 'q': return PieceType::queen;
    case 'k': return PieceType::king;
    default:  return PieceType::invalid;
    }
}

void handleMouseClick(float mouseX, float mouseY, char board[8][8], int& selectedRow, int& selectedCol, int& moves) {
    int col = static_cast<int>(mouseX) / TILE_SIZE;
    int row = static_cast<int>(mouseY) / TILE_SIZE;

    PieceColour clickedTarget = getPieceColour(row, col, board);
    if (clickedTarget == PieceColour::invalid) return;

    // Case 1: No piece currently selected
    if (selectedRow == -1) {
        if (clickedTarget != PieceColour::empty) {
            selectedRow = row;
            selectedCol = col;
        }
    }
    // Case 2: Moving or deselecting currently targeted piece
    else {
        PieceColour clickedColour = getPieceColour(selectedRow, selectedCol, board);
        if (selectedRow != row || selectedCol != col) {
            if (clickedColour != clickedTarget) {
                board[row][col] = board[selectedRow][selectedCol];
                board[selectedRow][selectedCol] = '.';
                moves++;
            }
        }
        selectedRow = -1;
        selectedCol = -1;
    }
}

// ============================================================================
// GRAPHICAL ENGINE & RENDERING
// ============================================================================

void renderGame(sf::RenderWindow& window, char board[8][8], int selectedRow, int selectedCol, bool isMenuOpen, Vector2f menuPos, const string& notificationMsg, const sf::Clock& notificationClock) {
    Color lightSquare(220, 220, 180);
    Color darkSquare(120, 145, 80);
    Color highlight(245, 245, 0, 220);

    RectangleShape tile(Vector2f((float)TILE_SIZE, (float)TILE_SIZE));
    window.clear();

    // 1. Render board tiles & piece sprites
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            tile.setPosition({ (float)c * TILE_SIZE, (float)r * TILE_SIZE });
            tile.setFillColor(((r + c) % 2 == 0) ? lightSquare : darkSquare);

            if (r == selectedRow && c == selectedCol) {
                tile.setFillColor(highlight);
            }
            window.draw(tile);

            char piece = board[r][c];
            if (piece != '.') {
                sf::Sprite sprite(pieceTextures[(int)piece]);
                Vector2u size = pieceTextures[(int)piece].getSize();
                sprite.setScale({ (float)TILE_SIZE / size.x, (float)TILE_SIZE / size.y });
                sprite.setPosition({ (float)c * TILE_SIZE, (float)r * TILE_SIZE });
                window.draw(sprite);
            }
        }
    }

    // 2. Render right-click context menu
    if (isMenuOpen) {
        string optionLabels[TOTAL_OPTIONS] = { "Save Game", "Load Game", "Log Stats" };

        RectangleShape menuBox(Vector2f(MENU_WIDTH, OPTION_HEIGHT * TOTAL_OPTIONS));
        menuBox.setPosition(menuPos);
        menuBox.setFillColor(Color(40, 40, 40));
        menuBox.setOutlineColor(Color::White);
        menuBox.setOutlineThickness(1.f);
        window.draw(menuBox);

        for (int i = 0; i < TOTAL_OPTIONS; i++) {
            Text optionText(menuFont, optionLabels[i], 14);
            optionText.setFillColor(Color::White);
            optionText.setPosition({ menuPos.x + 10.f, menuPos.y + (i * OPTION_HEIGHT) + 8.f });
            window.draw(optionText);
        }
    }

    // 3. Render temporary status notification overlay
    if (notificationClock.getElapsedTime().asSeconds() < 1.5f && !notificationMsg.empty()) {
        RectangleShape toastBox(Vector2f(220.f, 45.f));
        toastBox.setPosition({ (WINDOW_SIZE - 220.f) / 2.f, (WINDOW_SIZE - 45.f) / 2.f });
        toastBox.setFillColor(Color(20, 20, 20));
        toastBox.setOutlineColor(Color::White);
        toastBox.setOutlineThickness(1.5f);
        window.draw(toastBox);

        Text toastText(menuFont, notificationMsg, 16);
        toastText.setFillColor(Color::White);
        toastText.setPosition({ (WINDOW_SIZE - 220.f) / 2.f + 40.f, (WINDOW_SIZE - 45.f) / 2.f + 12.f });
        window.draw(toastText);
    }

    window.display();
}

void runGame(char board[8][8], int& moves) {
    static bool texturesLoaded = false;
    if (!texturesLoaded) {
        if (!loadTextures()) return;
        texturesLoaded = true;
    }

    RenderWindow window(sf::VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "UTAR Chess");
    window.setFramerateLimit(60);

    int selectedRow = -1;
    int selectedCol = -1;

    bool isMenuOpen = false;
    Vector2f menuPos(0.f, 0.f);

    string notificationMsg = "";
    sf::Clock notificationClock;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* resized = event->getIf<Event::Resized>()) {
                float w = (float)resized->size.x;
                float h = (float)resized->size.y;
                FloatRect viewport({}, { 1, 1 });

                if (w > h) {
                    viewport.size.x = h / w;
                    viewport.position.x = (1 - viewport.size.x) / 2;
                }
                else {
                    viewport.size.y = w / h;
                    viewport.position.y = (1 - viewport.size.y) / 2;
                }
                View fixedView(FloatRect({}, { 640, 640 }));
                fixedView.setViewport(viewport);
                window.setView(fixedView);
            }

            if (const auto* click = event->getIf<Event::MouseButtonPressed>()) {
                Vector2f worldPos = window.mapPixelToCoords(click->position);

                if (click->button == Mouse::Button::Right) {
                    isMenuOpen = true;
                    menuPos = worldPos;
                }
                else if (click->button == Mouse::Button::Left) {
                    if (isMenuOpen) {
                        if (worldPos.x >= menuPos.x && worldPos.x <= menuPos.x + MENU_WIDTH &&
                            worldPos.y >= menuPos.y && worldPos.y <= menuPos.y + (OPTION_HEIGHT * TOTAL_OPTIONS)) {

                            int option = static_cast<int>((worldPos.y - menuPos.y) / OPTION_HEIGHT);

                            switch (option) {
                            case 0:
                                if (saveGame("savegame.txt", board, moves)) {
                                    notificationMsg = "Game Saved!";
                                    notificationClock.restart();
                                }
                                break;
                            case 1:
                                if (loadGame("savegame.txt", board, moves)) {
                                    notificationMsg = "Game Loaded!";
                                    notificationClock.restart();
                                }
                                break;
                            case 2:
                                if (saveGameStats("game_stats.txt", "Match_End", moves)) {
                                    notificationMsg = "Stats Logged!";
                                    notificationClock.restart();
                                }
                                break;
                            }
                        }
                        isMenuOpen = false;
                    }
                    else {
                        handleMouseClick(worldPos.x, worldPos.y, board, selectedRow, selectedCol, moves);
                    }
                }
            }
        }
        renderGame(window, board, selectedRow, selectedCol, isMenuOpen, menuPos, notificationMsg, notificationClock);
    }
}

// ============================================================================
// CONSOLE INTERFACE & MAIN APPLICATION ENTRY
// ============================================================================

void displayMenu() {
    cout << "\n====================================\n";
    cout << "             CHESS SYSTEM           \n";
    cout << "====================================\n";
    cout << "1. Start New Game\n";
    cout << "2. Load Game\n";
    cout << "3. Display Move & Match History\n";
    cout << "4. Exit\n";
    cout << "====================================\n";
    cout << "Enter your choice (1-4): ";
}

int main() {
    string groupName;

    cout << "====================================\n";
    cout << "   WELCOME TO THE CHESS SYSTEM      \n";
    cout << "====================================\n";
    cout << "Enter group name to start: ";
    getline(cin, groupName);

    while (groupName.empty()) {
        cout << "Group name cannot be empty. Please enter group name: ";
        getline(cin, groupName);
    }

    cout << "\nWelcome, Team [" << groupName << "]!\n";

    char board[8][8];
    int moves = 0;
    int choice = 0;
    bool keepRunning = true;

    while (keepRunning) {
        displayMenu();

        if (!(cin >> choice)) {
            cout << "\n[ERROR] Invalid input. Please enter a number between 1 and 4.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
        case 1:
            cout << "\n[ACTION] Launching New Graphical Chess Game...\n";
            memcpy(board, INITIAL_BOARD, sizeof(INITIAL_BOARD));
            moves = 0;
            runGame(board, moves);
            cout << "\n[INFO] Returned to main menu.\n";
            break;

        case 2:
            cout << "\n[ACTION] Loading saved game file...\n";
            if (loadGame("savegame.txt", board, moves)) {
                cout << "\nLaunching loaded match state...\n";
                runGame(board, moves);
            }
            else {
                cout << "[ERROR] Could not load save file. Start a new game first.\n";
            }
            cout << "\n[INFO] Returned to main menu.\n";
            break;

        case 3:
            cout << "\n[ACTION] Displaying match history...\n";
            displayStatsHistory("game_stats.txt");
            break;

        case 4:
            cout << "\nExiting system. Goodbye, Team " << groupName << "!\n";
            keepRunning = false;
            break;

        default:
            cout << "\n[ERROR] Choice out of range. Please enter a number between 1 and 4.\n";
            break;
        }
    }

    return 0;
}