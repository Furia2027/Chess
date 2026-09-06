#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <cmath>

using namespace std;
using namespace sf;

const unsigned int WINDOW_SIZE = 640;
const int TILE_SIZE = 80;
const float MENU_WIDTH = 150.f;
const float OPTION_HEIGHT = 35.f;
const int TOTAL_OPTIONS = 5;

// UI & Chessboard Color Palette
const Color lightSquare(220, 220, 180);
const Color darkSquare(120, 145, 80);
const Color highlightColor(255, 255, 51, 130);
const Color bgDark(34, 38, 30);
const Color panelBg(45, 52, 40, 245);
const Color btnNormal(52, 62, 46);
const Color btnHover(120, 145, 80);
const Color btnBorder(160, 185, 120);
const Color hintDotColor(0, 0, 0, 60);
const Color hintCaptureRingColor(0, 0, 0, 60);
const Color checkHighlightColor(220, 50, 50, 180);

// Standard Starting Chess Position Grid
const char INITIAL_BOARD[8][8] = {
    {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'},
    {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
    {'.', '.', '.', '.', '.', '.', '.', '.'},
    {'.', '.', '.', '.', '.', '.', '.', '.'},
    {'.', '.', '.', '.', '.', '.', '.', '.'},
    {'.', '.', '.', '.', '.', '.', '.', '.'},
    {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
    {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'}
};

// Global Asset Containers
Texture pieceTextures[128];
Font menuFont;

// Piece Character Properties & UI States
enum class PieceColour { white, black, empty, invalid };
enum class PieceType { pawn, rook, knight, bishop, queen, king, empty, invalid };

enum class MenuState {
    GroupNameInput,
    MainMenu,
    LoadGamePreview,
    MoveHistory,
    InGame
};

// Forward declaration of GameSession structure
struct GameSession;

// Forward declarations for chess logic functions
bool findKing(PieceColour kingColor, const char board[8][8], int& outRow, int& outCol);
bool isSquareAttacked(int targetRow, int targetCol, PieceColour attackingSide, const char board[8][8]);
bool isKingInCheck(PieceColour kingColor, const char board[8][8]);
bool isMoveValid(int fromRow, int fromCol, int toRow, int toCol, const char board[8][8], int turn, const GameSession* session);
void renderMoveHints(RenderWindow& window, const GameSession& session);
bool hasAnyLegalMoves(PieceColour playerColor, const GameSession& session);
bool isInsufficientChess(const char board[8][8]);

// Interactive button UI element with hover effects and centered text rendering
struct MenuButton {
    FloatRect rect;
    string label;

    // Checks if mouse position is within button bounds
    bool isHovered(Vector2f mousePos) const {
        return rect.contains(mousePos);
    }

    // Draws button background and text label onto window
    void draw(RenderWindow& window, Font& font, Vector2f mousePos) const {
        bool hover = isHovered(mousePos);
        RectangleShape box({ rect.size.x, rect.size.y });
        box.setPosition({ rect.position.x, rect.position.y });
        box.setFillColor(hover ? btnHover : btnNormal);
        box.setOutlineColor(hover ? lightSquare : btnBorder);
        box.setOutlineThickness(hover ? 2.5f : 1.5f);
        window.draw(box);

        Text text(font, label, 18);
        text.setFillColor(hover ? Color::White : lightSquare);
        FloatRect tb = text.getLocalBounds();
        text.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
        text.setPosition({ rect.position.x + rect.size.x / 2.f, rect.position.y + rect.size.y / 2.f });
        window.draw(text);
    }
};

// Represents a single executed move record
struct Move {
    char piece;
    int fromRow, fromCol;
    int toRow, toCol;
    char captured;
};

// Timer structure to manage time limits for White and Black players
struct ChessClock {
    int whiteSecondsLeft = 600;
    int blackSecondsLeft = 600;
    time_t turnStartTime = 0;
};

vector<Move> moveHistory;
ChessClock clock1;

// Increments the game turn counter
void turnCounter(int& turn) {
    turn += 1;
}

// Stores move details into history vector and outputs details to console
void recordMove(char piece, int fromRow, int fromCol, int toRow, int toCol, char captured, int& turn) {
    Move thisMove = { piece, fromRow, fromCol, toRow, toCol, captured };
    moveHistory.push_back(thisMove);
    cout << turn << ". " << (isupper(piece) ? "White " : "Black ") << piece
        << " (" << fromRow << "," << fromCol << ") -> ("
        << toRow << "," << toCol << ")";
    if (captured != '.') cout << " x" << captured;
    cout << endl;
}

// Records timestamp at the start of player turn
void startTurnTimer(ChessClock& gameClock) {
    gameClock.turnStartTime = time(nullptr);
}

// Calculates elapsed turn time and subtracts it from active player clock
void stopTurnTimer(ChessClock& gameClock, bool wasWhiteTurn) {
    time_t now = time(nullptr);
    int elapsed = (int)(now - gameClock.turnStartTime);
    if (wasWhiteTurn) {
        gameClock.whiteSecondsLeft -= elapsed;
    }
    else {
        gameClock.blackSecondsLeft -= elapsed;
    }
}

// Converts raw time in seconds to "00:00" string format
string formatTime(int totalSeconds) {
    if (totalSeconds < 0) totalSeconds = 0;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    string secStr = (seconds < 10 ? "0" : "") + to_string(seconds);
    return to_string(minutes) + ":" + secStr;
}

// Checks if specified player timer has reached zero
bool hasTimedOut(const ChessClock& gameClock, bool isWhite) {
    return isWhite ? gameClock.whiteSecondsLeft <= 0 : gameClock.blackSecondsLeft <= 0;
}

// Holds complete active game state, saved state, selections, and flags
struct GameSession {
    MenuState state = MenuState::GroupNameInput;
    string groupName = "";

    // Active Board State
    char activeBoard[8][8];
    int activeMoves = 1;

    // Selection Tracking
    int selectedRow = -1;
    int selectedCol = -1;
    bool hasSelection = false;

    // Last Move Tracking for Highlights
    bool hasLastMove = false;
    int lastFromRow = -1, lastFromCol = -1;
    int lastToRow = -1, lastToCol = -1;

    // Saved Game Cache
    char savedBoard[8][8];
    int savedMoves = 1;
    bool hasSaveData = false;

    // Context Menu State
    bool isContextMenuOpen = false;
    Vector2f contextMenuPos{ 0.f, 0.f };

    // Toast Notification Trackers
    string notificationMsg = "";
    sf::Clock notificationClock;
    sf::Clock cursorClock;

    // Pawn Promotion
    bool isPromoting = false;
    int promoRow = -1;
    int promoCol = -1;
    int promoFromRow = -1;
    int promoFromCol = -1;
    char promoCaptured = '.';

    // Castling Rights
    bool whiteKingMoved = false;
    bool whiteRookAMoved = false;
    bool whiteRookHMoved = false;
    bool blackKingMoved = false;
    bool blackRookAMoved = false;
    bool blackRookHMoved = false;

    bool isGameOver = false;

    // Displays temporary toast notification on UI
    void notify(const string& msg) {
        notificationMsg = msg;
        notificationClock.restart();
    }

    // Resets game board, timers, move history, and flags for a new match
    void resetToNewGame() {
        memcpy(activeBoard, INITIAL_BOARD, sizeof(INITIAL_BOARD));
        activeMoves = 1;
        hasSelection = false;
        selectedRow = -1;
        selectedCol = -1;
        hasLastMove = false;
        lastFromRow = -1; lastFromCol = -1;
        lastToRow = -1;   lastToCol = -1;
        isContextMenuOpen = false;
        isPromoting = false;
        whiteKingMoved = whiteRookAMoved = whiteRookHMoved = false;
        blackKingMoved = blackRookAMoved = blackRookHMoved = false;
        isGameOver = false;
        state = MenuState::InGame;

        moveHistory.clear();
        clock1.whiteSecondsLeft = 600;
        clock1.blackSecondsLeft = 600;
        startTurnTimer(clock1);
    }

    // Restores active session board from saved game buffer
    void loadSavedToActive() {
        memcpy(activeBoard, savedBoard, sizeof(savedBoard));
        activeMoves = savedMoves;
        hasSelection = false;
        selectedRow = -1;
        selectedCol = -1;
        hasLastMove = false;
        lastFromRow = -1; lastFromCol = -1;
        lastToRow = -1;   lastToCol = -1;
        isContextMenuOpen = false;
        isPromoting = false;
        whiteKingMoved = whiteRookAMoved = whiteRookHMoved = false;
        blackKingMoved = blackRookAMoved = blackRookHMoved = false;
        isGameOver = false;
        state = MenuState::InGame;

        startTurnTimer(clock1);
    }
};

// Finalizes move execution: records history, adjusts timers, and updates session
void completeMove(char piece, int fromRow, int fromCol, int toRow, int toCol, char captured, int& turn, GameSession& session) {
    bool wasWhiteTurn = (turn % 2 == 1);
    recordMove(piece, fromRow, fromCol, toRow, toCol, captured, turn);
    stopTurnTimer(clock1, wasWhiteTurn);
    turnCounter(turn);
    startTurnTimer(clock1);

    session.hasLastMove = true;
    session.lastFromRow = fromRow;
    session.lastFromCol = fromCol;
    session.lastToRow = toRow;
    session.lastToCol = toCol;
}

// Reverts the last executed move in history and updates board state
bool undoMoveDirect(GameSession& session) {
    if (moveHistory.empty()) {
        cout << "No moves to undo." << endl;
        return false;
    }
    Move lastMove = moveHistory.back();

    // Revert Castling Rook Transfer
    if (tolower(lastMove.piece) == 'k' && abs(lastMove.toCol - lastMove.fromCol) == 2) {
        int homeRow = lastMove.fromRow;
        if (lastMove.toCol == 6) {
            session.activeBoard[homeRow][7] = session.activeBoard[homeRow][5];
            session.activeBoard[homeRow][5] = '.';
        }
        else if (lastMove.toCol == 2) {
            session.activeBoard[homeRow][0] = session.activeBoard[homeRow][3];
            session.activeBoard[homeRow][3] = '.';
        }
    }

    // Revert En Passant Captured Piece Position
    if (tolower(lastMove.piece) == 'p' && abs(lastMove.toCol - lastMove.fromCol) == 1 && lastMove.captured != '.' && session.activeBoard[lastMove.toRow][lastMove.toCol] == lastMove.piece) {
        session.activeBoard[lastMove.fromRow][lastMove.fromCol] = lastMove.piece;
        session.activeBoard[lastMove.toRow][lastMove.toCol] = '.';
        session.activeBoard[lastMove.fromRow][lastMove.toCol] = lastMove.captured;
    }
    else {
        session.activeBoard[lastMove.fromRow][lastMove.fromCol] = lastMove.piece;
        session.activeBoard[lastMove.toRow][lastMove.toCol] = lastMove.captured;
    }

    moveHistory.pop_back();

    bool wasWhiteTurn = (session.activeMoves % 2 == 1);
    stopTurnTimer(clock1, wasWhiteTurn);
    if (session.activeMoves > 1) session.activeMoves--;
    startTurnTimer(clock1);

    if (!moveHistory.empty()) {
        Move prevMove = moveHistory.back();
        session.hasLastMove = true;
        session.lastFromRow = prevMove.fromRow;
        session.lastFromCol = prevMove.fromCol;
        session.lastToRow = prevMove.toRow;
        session.lastToCol = prevMove.toCol;
    }
    else {
        session.hasLastMove = false;
    }

    session.isGameOver = false;
    session.isPromoting = false;
    cout << "Undo success." << endl;
    return true;
}

// Writes current board layout and turn counter to save file
bool saveGame(const string& filename, char board[8][8], int moves) {
    ofstream file(filename);
    if (!file.is_open()) return false;
    file << moves << endl;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            file << board[r][c] << " ";
        }
        file << endl;
    }
    return true;
}

// Reads saved board layout and turn counter from file
bool loadSaveGame(char board[8][8], int& moves) {
    ifstream file("savegame.txt");
    if (!file.is_open()) return false;
    file >> moves;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) file >> board[r][c];
    }
    return true;
}

// Appends game outcome results and move counts to match statistics log file
bool saveGameStats(const string& filename, const string& winner, int moves) {
    ofstream file(filename, ios::app);
    if (!file.is_open()) return false;
    file << "Winner: " << winner << " | Total Moves: " << moves << endl;
    return true;
}

// Reads and returns all entries stored in match statistics log file
vector<string> loadStatsLines() {
    vector<string> lines;
    ifstream file("game_stats.txt");
    if (!file.is_open()) return lines;
    string line;
    while (getline(file, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

// Evaluates piece color at given board coordinates
PieceColour getPieceColour(int row, int col, const char board[8][8]) {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return PieceColour::invalid;
    char piece = board[row][col];
    if (piece == '.') return PieceColour::empty;
    if (isupper(piece)) return PieceColour::white;
    if (islower(piece)) return PieceColour::black;
    return PieceColour::invalid;
}

// Evaluates piece type at given board coordinates
PieceType getPieceType(int row, int col, const char board[8][8]) {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return PieceType::invalid;
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

// Determines if active board state triggers draw by insufficient material
bool isInsufficientChess(const char board[8][8]) {
    vector<char> whitePieces;
    vector<char> blackPieces;
    pair<int, int> whiteBishopPos = { -1, -1 };
    pair<int, int> blackBishopPos = { -1, -1 };

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char p = board[r][c];
            if (p == '.') continue;

            if (p == 'p' || p == 'P' || p == 'r' || p == 'R' || p == 'q' || p == 'Q') {
                return false;
            }

            if (isupper(p)) {
                if (p != 'K') whitePieces.push_back(p);
                if (p == 'B') whiteBishopPos = { r, c };
            }
            else {
                if (p != 'k') blackPieces.push_back(p);
                if (p == 'b') blackBishopPos = { r, c };
            }
        }
    }

    // 1. King vs King
    if (whitePieces.empty() && blackPieces.empty()) return true;

    // 2. King + Minor vs King
    if ((whitePieces.empty() && blackPieces.size() == 1) || (blackPieces.empty() && whitePieces.size() == 1)) {
        return true;
    }

    // 3. King + Bishop vs King + Bishop on same color square
    if (whitePieces.size() == 1 && blackPieces.size() == 1) {
        if (whitePieces[0] == 'B' && blackPieces[0] == 'b') {
            bool whiteBishopOnLight = (whiteBishopPos.first + whiteBishopPos.second) % 2 == 0;
            bool blackBishopOnLight = (blackBishopPos.first + blackBishopPos.second) % 2 == 0;
            if (whiteBishopOnLight == blackBishopOnLight) return true;
        }
    }

    return false;
}

// Loads font files and piece texture sprites from memory/disk
bool loadAssets() {
    if (!menuFont.openFromFile("arial.ttf")) {
        cout << "[WARNING] Could not load arial.ttf\n";
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
        if (pieceTextures[(int)pieces[i]].loadFromFile(filenames[i])) {
            pieceTextures[(int)pieces[i]].setSmooth(true);
        }
    }
    return true;
}

// Draws highlight box overlay over a designated board tile
void highlight(RenderWindow& window, int row, int col, int selRow, int selCol, bool hasSel) {
    if (hasSel && row == selRow && col == selCol) {
        const float tileSize = WINDOW_SIZE / 8.f;
        RectangleShape highlightBox({ tileSize, tileSize });
        highlightBox.setPosition({ col * tileSize, row * tileSize });
        highlightBox.setFillColor(highlightColor);
        window.draw(highlightBox);
    }
}

// Draws full chessboard, tile highlights, piece sprites, and move hint indicators
void renderChessBoard(RenderWindow& window, const char board[8][8], const GameSession* session = nullptr) {
    const float tileSize = WINDOW_SIZE / 8.f;

    // 1. Draw tiles & base highlights
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            RectangleShape tile({ tileSize, tileSize });
            tile.setPosition({ c * tileSize, r * tileSize });
            tile.setFillColor(((r + c) % 2 == 0) ? lightSquare : darkSquare);
            window.draw(tile);

            // Last move origin and destination highlights
            if (session && session->hasLastMove) {
                if ((r == session->lastFromRow && c == session->lastFromCol) ||
                    (r == session->lastToRow && c == session->lastToCol)) {
                    highlight(window, r, c, r, c, true);
                }
            }

            // Active piece selection highlight
            if (session) {
                highlight(window, r, c, session->selectedRow, session->selectedCol, session->hasSelection);
            }
        }
    }

    // 2. King In Check Red Highlight
    if (session) {
        bool isWhiteTurn = (session->activeMoves % 2 == 1);
        PieceColour activeTurnColour = isWhiteTurn ? PieceColour::white : PieceColour::black;

        if (isKingInCheck(activeTurnColour, board)) {
            int kingRow = -1, kingCol = -1;
            if (findKing(activeTurnColour, board, kingRow, kingCol)) {
                RectangleShape checkTile({ tileSize, tileSize });
                checkTile.setPosition({ kingCol * tileSize, kingRow * tileSize });
                checkTile.setFillColor(checkHighlightColor);
                window.draw(checkTile);
            }
        }
    }

    // 3. Draw piece sprites
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char piece = board[r][c];
            if (piece != '.' && pieceTextures[(int)piece].getSize().x > 0) {
                Sprite sprite(pieceTextures[(int)piece]);
                Vector2u sz = pieceTextures[(int)piece].getSize();
                sprite.setScale({ tileSize / sz.x, tileSize / sz.y });
                sprite.setPosition({ c * tileSize, r * tileSize });
                window.draw(sprite);
            }
        }
    }

    // 4. Draw destination dots & capture rings on top
    if (session) {
        renderMoveHints(window, *session);
    }
}

// Main user interface class handling screen rendering, input fields, modals, and HUD
class ChessUI {
private:
    // Draw centered text with minimal code duplication
    void drawCenteredText(RenderWindow& window, Font& font, const string& str, float y, unsigned int size, Color color, float x = WINDOW_SIZE / 2.f) const {
        Text text(font, str, size);
        text.setFillColor(color);
        FloatRect b = text.getLocalBounds();
        text.setOrigin({ b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f });
        text.setPosition({ x, y });
        window.draw(text);
    }

    // Draw filled & outlined rectangle shapes efficiently
    void drawPanel(RenderWindow& window, Vector2f pos, Vector2f size, Color fill, Color outline = Color::Transparent, float thickness = 0.f) const {
        RectangleShape box(size);
        box.setPosition(pos);
        box.setFillColor(fill);
        box.setOutlineColor(outline);
        box.setOutlineThickness(thickness);
        window.draw(box);
    }

public:
    MenuButton btnStart, btnLoad, btnHistory, btnExit;
    MenuButton btnBackFromLoad, btnPlaySaved, btnBackFromHist, btnConfirmGroup;

    // Initializes menu buttons with bounds and labels
    ChessUI() :
        btnStart({ { (WINDOW_SIZE - 320.f) / 2.f, 230.f }, { 320.f, 46.f } }, "1. Start New Game"),
        btnLoad({ { (WINDOW_SIZE - 320.f) / 2.f, 290.f }, { 320.f, 46.f } }, "2. Load Game"),
        btnHistory({ { (WINDOW_SIZE - 320.f) / 2.f, 350.f }, { 320.f, 46.f } }, "3. Display Move History"),
        btnExit({ { (WINDOW_SIZE - 320.f) / 2.f, 410.f }, { 320.f, 46.f } }, "4. Exit"),
        btnBackFromLoad({ { (WINDOW_SIZE - 200.f) / 2.f, 560.f }, { 200.f, 40.f } }, "Back to Menu"),
        btnPlaySaved({ { (WINDOW_SIZE - 200.f) / 2.f, 510.f }, { 200.f, 40.f } }, "Play Game"),
        btnBackFromHist({ { (WINDOW_SIZE - 200.f) / 2.f, 560.f }, { 200.f, 40.f } }, "Back to Menu"),
        btnConfirmGroup({ { (WINDOW_SIZE - 180.f) / 2.f, 380.f }, { 180.f, 44.f } }, "Confirm") {
    }

    // Draws checkerboard pattern border along top edge of window
    void renderTopBorder(RenderWindow& window) const {
        const float checkSize = 20.f;
        for (unsigned int i = 0; i < WINDOW_SIZE / (unsigned int)checkSize; ++i) {
            drawPanel(window, { (float)i * checkSize, 0.f }, { checkSize, 6.f }, (i % 2 == 0) ? lightSquare : darkSquare);
        }
    }

    // Renders team name input form screen
    void renderGroupNameInput(RenderWindow& window, Font& font, const string& groupName, bool showCursor, Vector2f mousePos) const {
        drawPanel(window, { (WINDOW_SIZE - 440.f) / 2.f, 160.f }, { 440.f, 320.f }, panelBg, darkSquare, 2.f);
        drawCenteredText(window, font, "WELCOME TO CHESS SYSTEM", 205.f, 22, lightSquare);
        drawCenteredText(window, font, "Enter Group / Team Name to Start:", 255.f, 15, Color(200, 200, 200));

        drawPanel(window, { (WINDOW_SIZE - 320.f) / 2.f, 295.f }, { 320.f, 44.f }, Color(25, 28, 22), highlightColor, 1.5f);

        Text inputText(font, groupName + (showCursor ? "_" : ""), 18);
        inputText.setFillColor(Color::White);
        inputText.setPosition({ (WINDOW_SIZE - 300.f) / 2.f, 307.f });
        window.draw(inputText);

        btnConfirmGroup.draw(window, font, mousePos);
    }

    // Renders main navigation menu screen
    void renderMainMenu(RenderWindow& window, Font& font, const string& groupName, Vector2f mousePos) const {
        drawCenteredText(window, font, "CHESS SYSTEM", 75.f, 32, lightSquare);
        drawCenteredText(window, font, "Team: [" + groupName + "]", 118.f, 18, highlightColor);

        const float alertX = (WINDOW_SIZE - 460.f) / 2.f;
        drawPanel(window, { alertX, 158.f }, { 460.f, 40.f }, Color(40, 48, 35, 230), highlightColor, 1.5f);
        drawPanel(window, { alertX, 158.f }, { 4.f, 40.f }, highlightColor);

        drawCenteredText(window, font, "[!] Right-click during game for menu options (Save/Load/Menu)", 178.f, 13, Color(240, 240, 220), alertX + 460.f / 2.f + 4.f);

        btnStart.draw(window, font, mousePos);
        btnLoad.draw(window, font, mousePos);
        btnHistory.draw(window, font, mousePos);
        btnExit.draw(window, font, mousePos);
    }

    // Renders preview board screen for loaded save data
    void renderLoadGamePreview(RenderWindow& window, Font& font, bool hasSaveData, const char savedBoard[8][8], Vector2f mousePos) const {
        drawCenteredText(window, font, "SAVED GAME PREVIEW", 45.f, 24, lightSquare);
        if (hasSaveData) {
            renderChessBoard(window, savedBoard, nullptr);
            btnPlaySaved.draw(window, font, mousePos);
        }
        btnBackFromLoad.draw(window, font, mousePos);
    }

    // Renders past match statistics and move logs view
    void renderMoveHistory(RenderWindow& window, Font& font, const vector<string>& stats, Vector2f mousePos) const {
        drawCenteredText(window, font, "MATCH STATISTICS", 45.f, 24, lightSquare);
        float lineY = 115.f;
        for (size_t i = max(0, (int)stats.size() - 10); i < stats.size(); ++i) {
            Text entry(font, to_string(i + 1) + ". " + stats[i], 15);
            entry.setFillColor(lightSquare);
            entry.setPosition({ 80.f, lineY });
            window.draw(entry);
            lineY += 34.f;
        }
        btnBackFromHist.draw(window, font, mousePos);
    }

    // Renders right-click context menu overlay during gameplay
    void renderContextMenu(RenderWindow& window, Font& font, Vector2f pos) const {
        string optionLabels[TOTAL_OPTIONS] = { "Save Game", "Load Game", "Log Stats", "Undo Move", "Back to Menu" };
        drawPanel(window, pos, { MENU_WIDTH, OPTION_HEIGHT * TOTAL_OPTIONS }, Color(40, 40, 40, 240), Color::White, 1.f);

        for (int i = 0; i < TOTAL_OPTIONS; i++) {
            Text optionText(font, optionLabels[i], 14);
            optionText.setFillColor(Color::White);
            optionText.setPosition({ pos.x + 10.f, pos.y + (i * OPTION_HEIGHT) + 8.f });
            window.draw(optionText);
        }
    }

    // Returns index of clicked context menu option based on cursor position (-1 if outside)
    int getContextMenuOption(Vector2f mousePos, Vector2f contextMenuPos) const {
        FloatRect bounds(contextMenuPos, { MENU_WIDTH, OPTION_HEIGHT * TOTAL_OPTIONS });
        return bounds.contains(mousePos) ? static_cast<int>((mousePos.y - contextMenuPos.y) / OPTION_HEIGHT) : -1;
    }

    // Renders top HUD displaying active player turn and remaining clock timers
    void renderClockHUD(RenderWindow& window, Font& font, int turn) const {
        time_t now = time(nullptr);
        int elapsed = (clock1.turnStartTime > 0) ? static_cast<int>(now - clock1.turnStartTime) : 0;
        bool isWhiteTurn = (turn % 2 == 1);

        int currentWhite = clock1.whiteSecondsLeft - (isWhiteTurn ? elapsed : 0);
        int currentBlack = clock1.blackSecondsLeft - (!isWhiteTurn ? elapsed : 0);

        string hudStr = (isWhiteTurn ? "[White's Turn]  " : "[Black's Turn]  ") +
            ("W: " + formatTime(currentWhite) + " | B: " + formatTime(currentBlack));

        drawPanel(window, { (WINDOW_SIZE - 340.f) / 2.f, 2.f }, { 340.f, 24.f }, Color(25, 28, 22, 220), btnBorder, 1.f);
        drawCenteredText(window, font, hudStr, 13.f, 13, Color(240, 240, 240));
    }

    // Renders pawn promotion selection modal overlay
    void renderPromotionModal(RenderWindow& window, Font& font, const GameSession& session) const {
        if (!session.isPromoting) return;

        drawPanel(window, { 0.f, 0.f }, { (float)WINDOW_SIZE, (float)WINDOW_SIZE }, Color(0, 0, 0, 150));

        const float boxWidth = 340.f, boxHeight = 120.f;
        const float boxX = (WINDOW_SIZE - boxWidth) / 2.f;
        const float boxY = (WINDOW_SIZE - boxHeight) / 2.f;

        drawPanel(window, { boxX, boxY }, { boxWidth, boxHeight }, panelBg, highlightColor, 2.f);
        drawCenteredText(window, font, "Choose Promotion Piece:", boxY + 22.f, 16, lightSquare);

        bool isWhite = (session.activeMoves % 2 == 1);
        char choices[4] = { isWhite ? 'Q' : 'q', isWhite ? 'R' : 'r', isWhite ? 'B' : 'b', isWhite ? 'N' : 'n' };

        const float pieceSize = 56.f, spacing = 18.f;
        float startX = boxX + (boxWidth - (4 * pieceSize + 3 * spacing)) / 2.f;
        float startY = boxY + 46.f;

        for (int i = 0; i < 4; i++) {
            Vector2f pos = { startX + i * (pieceSize + spacing), startY };
            drawPanel(window, pos, { pieceSize, pieceSize }, Color(25, 28, 22, 200), btnBorder, 1.f);

            char p = choices[i];
            if (pieceTextures[(int)p].getSize().x > 0) {
                Sprite sprite(pieceTextures[(int)p]);
                Vector2u sz = pieceTextures[(int)p].getSize();
                sprite.setScale({ pieceSize / sz.x, pieceSize / sz.y });
                sprite.setPosition(pos);
                window.draw(sprite);
            }
        }
    }

    // Handles user selection click inside pawn promotion modal
    bool handlePromotionClick(GameSession& session, Vector2f mousePos) const {
        if (!session.isPromoting) return false;

        const float boxWidth = 340.f, boxHeight = 120.f;
        const float boxX = (WINDOW_SIZE - boxWidth) / 2.f;
        const float boxY = (WINDOW_SIZE - boxHeight) / 2.f;

        const float pieceSize = 56.f, spacing = 18.f;
        float startX = boxX + (boxWidth - (4 * pieceSize + 3 * spacing)) / 2.f;
        float startY = boxY + 46.f;

        bool isWhite = (session.activeMoves % 2 == 1);
        char choices[4] = { isWhite ? 'Q' : 'q', isWhite ? 'R' : 'r', isWhite ? 'B' : 'b', isWhite ? 'N' : 'n' };

        for (int i = 0; i < 4; i++) {
            FloatRect slotRect({ startX + i * (pieceSize + spacing), startY }, { pieceSize, pieceSize });
            if (slotRect.contains(mousePos)) {
                char chosenPiece = choices[i];

                session.activeBoard[session.promoRow][session.promoCol] = chosenPiece;
                completeMove(chosenPiece, session.promoFromRow, session.promoFromCol, session.promoRow, session.promoCol, session.promoCaptured, session.activeMoves, session);

                session.isPromoting = false;

                if (isInsufficientChess(session.activeBoard)) {
                    session.isGameOver = true;
                    session.notify("Draw: Insufficient Chess!");
                    saveGameStats("game_stats.txt", "Draw (Chess)", session.activeMoves);
                    return true;
                }

                PieceColour opponentColour = isWhite ? PieceColour::black : PieceColour::white;
                bool inCheck = isKingInCheck(opponentColour, session.activeBoard);
                bool canMove = hasAnyLegalMoves(opponentColour, session);

                if (!canMove) {
                    session.isGameOver = true;
                    if (inCheck) {
                        session.notify(isWhite ? "White wins by Checkmate!" : "Black wins by Checkmate!");
                        saveGameStats("game_stats.txt", (isWhite ? "White" : "Black"), session.activeMoves);
                    }
                    else {
                        session.notify("Draw by Stalemate!");
                        saveGameStats("game_stats.txt", "Draw", session.activeMoves);
                    }
                }
                else if (inCheck) {
                    session.notify("Check!");
                }
                return true;
            }
        }
        return false;
    }

    // Renders active gameplay view combining chessboard, clock HUD, context menu, and promotion dialogs
    void renderInGame(RenderWindow& window, Font& font, const GameSession& session) const {
        renderChessBoard(window, session.activeBoard, &session);
        renderClockHUD(window, font, session.activeMoves);

        if (session.isContextMenuOpen) renderContextMenu(window, font, session.contextMenuPos);
        if (session.isPromoting) renderPromotionModal(window, font, session);
    }

    // Renders temporary fading toast message notification centered on screen
    void renderToastNotification(RenderWindow& window, Font& font, const string& msg, float elapsed, float fadeDuration = 1.6f) const {
        if (elapsed >= fadeDuration || msg.empty()) return;

        float alphaRatio = 1.0f - (elapsed / fadeDuration);
        uint8_t alpha = static_cast<uint8_t>(255 * alphaRatio);

        drawPanel(window, { (WINDOW_SIZE - 300.f) / 2.f, (WINDOW_SIZE - 46.f) / 2.f }, { 300.f, 46.f },
            Color(20, 20, 20, static_cast<uint8_t>(220 * alphaRatio)), Color(255, 255, 255, alpha), 1.5f);
        drawCenteredText(window, font, msg, WINDOW_SIZE / 2.f, 16, Color(255, 255, 255, alpha));
    }
};

// Adjusts viewport scaling on window resize events to keep square aspect ratio
void updateWindowView(RenderWindow& window, const Event::Resized* resized) {
    float w = static_cast<float>(resized->size.x);
    float h = static_cast<float>(resized->size.y);
    FloatRect viewport({}, { 1.f, 1.f });

    if (w > h) {
        viewport.size.x = h / w;
        viewport.position.x = (1.f - viewport.size.x) / 2.f;
    }
    else {
        viewport.size.y = w / h;
        viewport.position.y = (1.f - viewport.size.y) / 2.f;
    }
    View fixedView(FloatRect({}, { static_cast<float>(WINDOW_SIZE), static_cast<float>(WINDOW_SIZE) }));
    fixedView.setViewport(viewport);
    window.setView(fixedView);
}

// Handles character typing events for team/group name input
void handleTextInput(GameSession& session, const Event::TextEntered* textEvent) {
    char32_t unicode = textEvent->unicode;
    if (unicode == 8) { // Backspace key
        if (!session.groupName.empty()) session.groupName.pop_back();
    }
    else if (unicode == 13 || unicode == 10) { // Enter key
        if (!session.groupName.empty()) {
            session.state = MenuState::MainMenu;
            session.notify("Welcome, Team " + session.groupName + "!");
        }
    }
    else if (unicode >= 32 && unicode < 127 && session.groupName.size() < 18) { // Printable characters
        session.groupName += static_cast<char>(unicode);
    }
}

// Processes context menu selections (Save, Load, Draw, Stats, Undo, Main Menu)
void handleContextMenuAction(GameSession& session, int option) {
    switch (option) {
    case 0: // Save current board state and turn count
        if (saveGame("savegame.txt", session.activeBoard, session.activeMoves)) {
            session.notify("Game Saved!");
        }
        break;
    case 1: // Load game state and reset turn context
        if (loadSaveGame(session.activeBoard, session.activeMoves)) {
            session.hasSelection = false;
            session.selectedRow = -1;
            session.selectedCol = -1;
            session.hasLastMove = false;
            session.isPromoting = false;
            startTurnTimer(clock1);
            session.notify("Game Loaded!");
        }
        break;
    case 2: // Log game statistics to file
        if (saveGameStats("game_stats.txt", session.groupName, session.activeMoves)) {
            session.notify("Stats Logged!");
        }
        break;
    case 3: // Revert to previous turn state
        if (undoMoveDirect(session)) {
            session.hasSelection = false;
            session.selectedRow = -1;
            session.selectedCol = -1;
            session.isPromoting = false;
            session.notify("Move Undone!");
        }
        else {
            session.notify("No moves to undo.");
        }
        break;
    case 4: // Return to Main Menu
        session.state = MenuState::MainMenu;
        session.hasSelection = false;
        session.selectedRow = -1;
        session.selectedCol = -1;
        session.isPromoting = false;
        session.notify("Returned to Menu");
        break;
    }
}

// Validates whether the tiles between target coordinates are empty
bool isPathClear(int fromRow, int fromCol, int toRow, int toCol, const char board[8][8]) {
    int dRow = toRow - fromRow;
    int dCol = toCol - fromCol;

    // Determine direction step vector
    int stepRow = (dRow == 0) ? 0 : (dRow > 0 ? 1 : -1);
    int stepCol = (dCol == 0) ? 0 : (dCol > 0 ? 1 : -1);

    int currRow = fromRow + stepRow;
    int currCol = fromCol + stepCol;

    // Traverse ray until destination square is reached
    while (currRow != toRow || currCol != toCol) {
        if (board[currRow][currCol] != '.') {
            return false; // Obstruction found
        }
        currRow += stepRow;
        currCol += stepCol;
    }
    return true;
}

// Validates L-shaped movement for Knights
bool isValidKnightMove(int fromRow, int fromCol, int toRow, int toCol) {
    int dRow = abs(toRow - fromRow);
    int dCol = abs(toCol - fromCol);
    return (dRow == 1 && dCol == 2) || (dRow == 2 && dCol == 1);
}

// Validates horizontal/vertical movement for Rooks
bool isValidRookMove(int fromRow, int fromCol, int toRow, int toCol, const char board[8][8]) {
    if (fromRow != toRow && fromCol != toCol) return false;
    return isPathClear(fromRow, fromCol, toRow, toCol, board);
}

// Validates diagonal movement for Bishops
bool isValidBishopMove(int fromRow, int fromCol, int toRow, int toCol, const char board[8][8]) {
    if (abs(toRow - fromRow) != abs(toCol - fromCol)) return false;
    return isPathClear(fromRow, fromCol, toRow, toCol, board);
}

// Validates combined straight and diagonal movement for Queens
bool isValidQueenMove(int fromRow, int fromCol, int toRow, int toCol, const char board[8][8]) {
    bool isStraight = (fromRow == toRow || fromCol == toCol);
    bool isDiagonal = (abs(toRow - fromRow) == abs(toCol - fromCol));
    if (!isStraight && !isDiagonal) return false;
    return isPathClear(fromRow, fromCol, toRow, toCol, board);
}

// Validates single-tile movement and castling conditions for Kings
bool isValidKingMove(int fromRow, int fromCol, int toRow, int toCol, PieceColour kingColor, const char board[8][8], const GameSession* session = nullptr) {
    int dRow = abs(toRow - fromRow);
    int dCol = abs(toCol - fromCol);

    // Standard single-step movement
    if (dRow <= 1 && dCol <= 1 && !(dRow == 0 && dCol == 0)) {
        return true;
    }

    // Castling Logic
    if (dRow == 0 && dCol == 2 && session) {
        int homeRow = (kingColor == PieceColour::white) ? 7 : 0;
        if (fromRow != homeRow || fromCol != 4) return false;

        PieceColour enemySide = (kingColor == PieceColour::white) ? PieceColour::black : PieceColour::white;
        if (isSquareAttacked(homeRow, 4, enemySide, board)) return false; // Cannot castle out of check

        // Kingside Castling
        if (toCol == 6) {
            bool kingMoved = (kingColor == PieceColour::white) ? session->whiteKingMoved : session->blackKingMoved;
            bool rookMoved = (kingColor == PieceColour::white) ? session->whiteRookHMoved : session->blackRookHMoved;
            char expectedRook = (kingColor == PieceColour::white) ? 'R' : 'r';

            if (kingMoved || rookMoved) return false;
            if (board[homeRow][7] != expectedRook) return false;
            if (board[homeRow][5] != '.' || board[homeRow][6] != '.') return false;

            // Ensure path is not under attack
            if (isSquareAttacked(homeRow, 5, enemySide, board)) return false;
            if (isSquareAttacked(homeRow, 6, enemySide, board)) return false;

            return true;
        }

        // Queenside Castling
        if (toCol == 2) {
            bool kingMoved = (kingColor == PieceColour::white) ? session->whiteKingMoved : session->blackKingMoved;
            bool rookMoved = (kingColor == PieceColour::white) ? session->whiteRookAMoved : session->blackRookAMoved;
            char expectedRook = (kingColor == PieceColour::white) ? 'R' : 'r';

            if (kingMoved || rookMoved) return false;
            if (board[homeRow][0] != expectedRook) return false;
            if (board[homeRow][1] != '.' || board[homeRow][2] != '.' || board[homeRow][3] != '.') return false;

            // Ensure path is not under attack
            if (isSquareAttacked(homeRow, 3, enemySide, board)) return false;
            if (isSquareAttacked(homeRow, 2, enemySide, board)) return false;

            return true;
        }
    }

    return false;
}

// Validates Pawn forward pushes, double-step openers, standard captures, and en passant
bool isValidPawnMove(int fromRow, int fromCol, int toRow, int toCol, PieceColour color, const char board[8][8], const GameSession* session) {
    int step = (color == PieceColour::white) ? -1 : 1;
    int startRow = (color == PieceColour::white) ? 6 : 1;
    int dRow = toRow - fromRow;
    int dCol = toCol - fromCol;

    // Single-step forward
    if (dCol == 0 && dRow == step) {
        return board[toRow][toCol] == '.';
    }

    // Double-step initial push
    if (dCol == 0 && dRow == 2 * step && fromRow == startRow) {
        return board[fromRow + step][fromCol] == '.' && board[toRow][toCol] == '.';
    }

    // Diagonal captures & En Passant
    if (abs(dCol) == 1 && dRow == step) {
        // Standard capture
        if (board[toRow][toCol] != '.') {
            return true;
        }

        // En Passant condition check
        if (session && session->hasLastMove) {
            char lastPiece = board[session->lastToRow][session->lastToCol];
            bool wasPawn = (tolower(lastPiece) == 'p');
            bool wasDoubleStep = (abs(session->lastToRow - session->lastFromRow) == 2);
            bool isAdjacent = (session->lastToRow == fromRow && session->lastToCol == toCol);

            if (wasPawn && wasDoubleStep && isAdjacent) {
                return true;
            }
        }
    }

    return false;
}

// Locates the King's board coordinates for a given color
bool findKing(PieceColour kingColor, const char board[8][8], int& outRow, int& outCol) {
    char targetKing = (kingColor == PieceColour::white) ? 'K' : 'k';
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (board[r][c] == targetKing) {
                outRow = r;
                outCol = c;
                return true;
            }
        }
    }
    return false;
}

// Checks if a specific board square is targeted by any opposing piece
bool isSquareAttacked(int targetRow, int targetCol, PieceColour attackingSide, const char board[8][8]) {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (getPieceColour(r, c, board) != attackingSide) continue;

            PieceType type = getPieceType(r, c, board);
            bool canAttack = false;

            switch (type) {
            case PieceType::pawn: {
                int step = (attackingSide == PieceColour::white) ? -1 : 1;
                if (abs(targetCol - c) == 1 && (targetRow - r) == step) {
                    canAttack = true;
                }
                break;
            }
            case PieceType::knight:
                canAttack = isValidKnightMove(r, c, targetRow, targetCol);
                break;
            case PieceType::bishop:
                canAttack = isValidBishopMove(r, c, targetRow, targetCol, board);
                break;
            case PieceType::rook:
                canAttack = isValidRookMove(r, c, targetRow, targetCol, board);
                break;
            case PieceType::queen:
                canAttack = isValidQueenMove(r, c, targetRow, targetCol, board);
                break;
            case PieceType::king:
                canAttack = (abs(targetRow - r) <= 1 && abs(targetCol - c) <= 1);
                break;
            default:
                break;
            }

            if (canAttack) return true;
        }
    }
    return false;
}

// Determines if the specified player's King is under attack
bool isKingInCheck(PieceColour kingColor, const char board[8][8]) {
    int kRow = -1, kCol = -1;
    if (!findKing(kingColor, board, kRow, kCol)) return false;

    PieceColour enemyColor = (kingColor == PieceColour::white) ? PieceColour::black : PieceColour::white;
    return isSquareAttacked(kRow, kCol, enemyColor, board);
}

// Evaluates comprehensive move validity (geometry, turn order, boundaries, self-check safety)
bool isMoveValid(int fromRow, int fromCol, int toRow, int toCol, const char board[8][8], int turn, const GameSession* session) {
    // Bounds check
    if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8) return false;
    if (toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8) return false;
    if (fromRow == toRow && fromCol == toCol) return false;

    PieceColour sourceColour = getPieceColour(fromRow, fromCol, board);
    PieceColour targetColour = getPieceColour(toRow, toCol, board);

    // Turn order validation
    bool isWhiteTurn = (turn % 2 == 1);
    if (isWhiteTurn && sourceColour != PieceColour::white) return false;
    if (!isWhiteTurn && sourceColour != PieceColour::black) return false;

    // Friendly piece collision check
    if (sourceColour == targetColour) return false;

    PieceType type = getPieceType(fromRow, fromCol, board);
    bool geometryPass = false;

    // Piece-specific geometry validation
    switch (type) {
    case PieceType::pawn:   geometryPass = isValidPawnMove(fromRow, fromCol, toRow, toCol, sourceColour, board, session); break;
    case PieceType::knight: geometryPass = isValidKnightMove(fromRow, fromCol, toRow, toCol); break;
    case PieceType::bishop: geometryPass = isValidBishopMove(fromRow, fromCol, toRow, toCol, board); break;
    case PieceType::rook:   geometryPass = isValidRookMove(fromRow, fromCol, toRow, toCol, board); break;
    case PieceType::queen:  geometryPass = isValidQueenMove(fromRow, fromCol, toRow, toCol, board); break;
    case PieceType::king:   geometryPass = isValidKingMove(fromRow, fromCol, toRow, toCol, sourceColour, board, session); break;
    default: return false;
    }

    if (!geometryPass) return false;

    // Simulate move on a temporary board to verify self-check safety
    char tempBoard[8][8];
    memcpy(tempBoard, board, sizeof(tempBoard));
    tempBoard[toRow][toCol] = tempBoard[fromRow][fromCol];
    tempBoard[fromRow][fromCol] = '.';

    // Handle En Passant capture cleanup on temp board
    if (type == PieceType::pawn && abs(toCol - fromCol) == 1 && board[toRow][toCol] == '.') {
        tempBoard[fromRow][toCol] = '.';
    }

    // Verify move does not leave friendly King in check
    if (isKingInCheck(sourceColour, tempBoard)) {
        return false;
    }

    return true;
}

// Draws overlay indicators for legal move targets and capture rings
void renderMoveHints(RenderWindow& window, const GameSession& session) {
    if (!session.hasSelection || session.isGameOver || session.isPromoting) return;

    const float tileSize = WINDOW_SIZE / 8.f;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (isMoveValid(session.selectedRow, session.selectedCol, r, c,
                session.activeBoard, session.activeMoves, &session)) {

                float centerX = c * tileSize + (tileSize / 2.f);
                float centerY = r * tileSize + (tileSize / 2.f);

                bool isCapture = (session.activeBoard[r][c] != '.');
                char selectedPiece = session.activeBoard[session.selectedRow][session.selectedCol];
                if (tolower(selectedPiece) == 'p' && abs(c - session.selectedCol) == 1 && session.activeBoard[r][c] == '.') {
                    isCapture = true; // En Passant capture hint
                }

                if (isCapture) {
                    // Draw outer ring for captures
                    float ringRadius = tileSize * 0.42f;
                    CircleShape ring(ringRadius);
                    ring.setOrigin({ ringRadius, ringRadius });
                    ring.setPosition({ centerX, centerY });
                    ring.setFillColor(Color::Transparent);
                    ring.setOutlineColor(hintCaptureRingColor);
                    ring.setOutlineThickness(tileSize * 0.08f);
                    window.draw(ring);
                }
                else {
                    // Draw centered dot for standard moves
                    float dotRadius = tileSize * 0.16f;
                    CircleShape dot(dotRadius);
                    dot.setOrigin({ dotRadius, dotRadius });
                    dot.setPosition({ centerX, centerY });
                    dot.setFillColor(hintDotColor);
                    window.draw(dot);
                }
            }
        }
    }
}

// Checks if the player has any valid legal moves available (Checkmate/Stalemate determination)
bool hasAnyLegalMoves(PieceColour playerColor, const GameSession& session) {
    int turn = (playerColor == PieceColour::white) ? 1 : 2;

    for (int r1 = 0; r1 < 8; r1++) {
        for (int c1 = 0; c1 < 8; c1++) {
            if (getPieceColour(r1, c1, session.activeBoard) != playerColor) continue;

            for (int r2 = 0; r2 < 8; r2++) {
                for (int c2 = 0; c2 < 8; c2++) {
                    if (isMoveValid(r1, c1, r2, c2, session.activeBoard, turn, &session)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// Handles piece selection, movement execution, special rules, and game state resolution
void handleInGamePieceMove(GameSession& session, Vector2f mousePos) {
    if (session.isGameOver || session.isPromoting) return;

    const float tileSize = WINDOW_SIZE / 8.f;
    int col = static_cast<int>(mousePos.x / tileSize);
    int row = static_cast<int>(mousePos.y / tileSize);

    if (row < 0 || row >= 8 || col < 0 || col >= 8) return;

    bool isWhiteTurn = (session.activeMoves % 2 == 1);
    PieceColour currentTurnColour = isWhiteTurn ? PieceColour::white : PieceColour::black;
    PieceColour clickedColour = getPieceColour(row, col, session.activeBoard);

    if (!session.hasSelection) {
        // Initial piece selection
        if (clickedColour == currentTurnColour) {
            session.selectedRow = row;
            session.selectedCol = col;
            session.hasSelection = true;
        }
        else if (clickedColour != PieceColour::empty) {
            session.notify(isWhiteTurn ? "White's turn!" : "Black's turn!");
        }
    }
    else {
        if (session.selectedRow == row && session.selectedCol == col) {
            // Deselect tile on second click
            session.hasSelection = false;
        }
        else {
            PieceColour sourceColour = getPieceColour(session.selectedRow, session.selectedCol, session.activeBoard);
            PieceColour targetColour = clickedColour;

            if (targetColour == sourceColour) {
                // Switch selection to another friendly piece
                session.selectedRow = row;
                session.selectedCol = col;
                session.hasSelection = true;
            }
            else {
                // Execute move if legal
                if (isMoveValid(session.selectedRow, session.selectedCol, row, col, session.activeBoard, session.activeMoves, &session)) {
                    char piece = session.activeBoard[session.selectedRow][session.selectedCol];
                    char captured = session.activeBoard[row][col];

                    // En Passant tile cleanup
                    if (tolower(piece) == 'p' && abs(col - session.selectedCol) == 1 && captured == '.') {
                        captured = session.activeBoard[session.selectedRow][col];
                        session.activeBoard[session.selectedRow][col] = '.';
                    }

                    // Castling Rook Relocation
                    if (tolower(piece) == 'k' && abs(col - session.selectedCol) == 2) {
                        int homeRow = session.selectedRow;
                        if (col == 6) {
                            session.activeBoard[homeRow][5] = session.activeBoard[homeRow][7];
                            session.activeBoard[homeRow][7] = '.';
                        }
                        else if (col == 2) {
                            session.activeBoard[homeRow][3] = session.activeBoard[homeRow][0];
                            session.activeBoard[homeRow][0] = '.';
                        }
                    }

                    // Update Castling Tracking Flags
                    if (piece == 'K') session.whiteKingMoved = true;
                    if (piece == 'k') session.blackKingMoved = true;
                    if (session.selectedRow == 7 && session.selectedCol == 0) session.whiteRookAMoved = true;
                    if (session.selectedRow == 7 && session.selectedCol == 7) session.whiteRookHMoved = true;
                    if (session.selectedRow == 0 && session.selectedCol == 0) session.blackRookAMoved = true;
                    if (session.selectedRow == 0 && session.selectedCol == 7) session.blackRookHMoved = true;

                    if (row == 7 && col == 0) session.whiteRookAMoved = true;
                    if (row == 7 && col == 7) session.whiteRookHMoved = true;
                    if (row == 0 && col == 0) session.blackRookAMoved = true;
                    if (row == 0 && col == 7) session.blackRookHMoved = true;

                    session.activeBoard[row][col] = piece;
                    session.activeBoard[session.selectedRow][session.selectedCol] = '.';

                    // Intercept Pawn Promotion
                    bool isPromoWhite = (piece == 'P' && row == 0);
                    bool isPromoBlack = (piece == 'p' && row == 7);

                    if (isPromoWhite || isPromoBlack) {
                        session.isPromoting = true;
                        session.promoRow = row;
                        session.promoCol = col;
                        session.promoFromRow = session.selectedRow;
                        session.promoFromCol = session.selectedCol;
                        session.promoCaptured = captured;
                        session.hasSelection = false;
                        return;
                    }

                    completeMove(piece, session.selectedRow, session.selectedCol, row, col, captured, session.activeMoves, session);
                    session.hasSelection = false;

                    // Draw Check: Insufficient Chess
                    if (isInsufficientChess(session.activeBoard)) {
                        session.isGameOver = true;
                        session.notify("Draw: Insufficient Chess!");
                        saveGameStats("game_stats.txt", "Draw (Chess)", session.activeMoves);
                        return;
                    }

                    // Evaluate Checkmate/Stalemate
                    PieceColour opponentColour = isWhiteTurn ? PieceColour::black : PieceColour::white;
                    bool inCheck = isKingInCheck(opponentColour, session.activeBoard);
                    bool canMove = hasAnyLegalMoves(opponentColour, session);

                    if (!canMove) {
                        session.isGameOver = true;
                        if (inCheck) {
                            string winner = (isWhiteTurn ? "White wins by Checkmate!" : "Black wins by Checkmate!");
                            session.notify(winner);
                            saveGameStats("game_stats.txt", (isWhiteTurn ? "White" : "Black"), session.activeMoves);
                        }
                        else {
                            session.notify("Draw by Stalemate!");
                            saveGameStats("game_stats.txt", "Draw", session.activeMoves);
                        }
                    }
                    else if (inCheck) {
                        session.notify("Check!");
                    }
                }
                else {
                    session.notify("Illegal move!");
                }
            }
        }
    }
}

// Directs mouse input based on current application menu state
void handleMouseClicks(RenderWindow& window, GameSession& session, const ChessUI& ui, Vector2f mousePos, Mouse::Button button) {
    // Open in-game context menu on right click
    if (button == Mouse::Button::Right && session.state == MenuState::InGame && !session.isPromoting) {
        session.isContextMenuOpen = true;
        session.contextMenuPos = mousePos;
        return;
    }

    if (button != Mouse::Button::Left) return;

    // Process UI button interactions across application states
    switch (session.state) {
    case MenuState::GroupNameInput:
        if (ui.btnConfirmGroup.isHovered(mousePos) && !session.groupName.empty()) {
            session.state = MenuState::MainMenu;
            session.notify("Welcome, Team " + session.groupName + "!");
        }
        break;

    case MenuState::MainMenu:
        if (ui.btnStart.isHovered(mousePos)) {
            session.resetToNewGame();
        }
        else if (ui.btnLoad.isHovered(mousePos)) {
            session.hasSaveData = loadSaveGame(session.savedBoard, session.savedMoves);
            session.state = MenuState::LoadGamePreview;
        }
        else if (ui.btnHistory.isHovered(mousePos)) {
            session.state = MenuState::MoveHistory;
        }
        else if (ui.btnExit.isHovered(mousePos)) {
            window.close();
        }
        break;

    case MenuState::LoadGamePreview:
        if (ui.btnBackFromLoad.isHovered(mousePos)) {
            session.state = MenuState::MainMenu;
        }
        else if (ui.btnPlaySaved.isHovered(mousePos)) {
            session.loadSavedToActive();
        }
        break;

    case MenuState::MoveHistory:
        if (ui.btnBackFromHist.isHovered(mousePos)) {
            session.state = MenuState::MainMenu;
        }
        break;

    case MenuState::InGame:
        if (session.isPromoting) {
            ui.handlePromotionClick(session, mousePos);
        }
        else if (session.isContextMenuOpen) {
            int option = ui.getContextMenuOption(mousePos, session.contextMenuPos);
            if (option >= 0) {
                handleContextMenuAction(session, option);
            }
            session.isContextMenuOpen = false;
        }
        else {
            handleInGamePieceMove(session, mousePos);
        }
        break;
    }
}

// Master rendering function
void renderApp(RenderWindow& window, const GameSession& session, const ChessUI& ui, Vector2f mousePos) {
    window.clear(bgDark);

    if (session.state != MenuState::InGame) {
        ui.renderTopBorder(window);
    }

    switch (session.state) {
    case MenuState::GroupNameInput: {
        bool showCursor = (static_cast<int>(session.cursorClock.getElapsedTime().asSeconds() * 2) % 2 == 0);
        ui.renderGroupNameInput(window, menuFont, session.groupName, showCursor, mousePos);
        break;
    }
    case MenuState::MainMenu:
        ui.renderMainMenu(window, menuFont, session.groupName, mousePos);
        break;
    case MenuState::LoadGamePreview:
        ui.renderLoadGamePreview(window, menuFont, session.hasSaveData, session.savedBoard, mousePos);
        break;
    case MenuState::MoveHistory: {
        vector<string> stats = loadStatsLines();
        ui.renderMoveHistory(window, menuFont, stats, mousePos);
        break;
    }
    case MenuState::InGame:
        ui.renderInGame(window, menuFont, session);
        break;
    }

    ui.renderToastNotification(window, menuFont, session.notificationMsg, session.notificationClock.getElapsedTime().asSeconds());

    window.display();
}

// Main Function
int main() {
    loadAssets();

    RenderWindow window(VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "UTAR Chess");
    window.setFramerateLimit(60);

    GameSession session;
    ChessUI ui;

    // Event and Render Loop
    while (window.isOpen()) {
        Vector2i mousePixel = Mouse::getPosition(window);
        Vector2f mousePos = window.mapPixelToCoords(mousePixel);

        while (const auto event = window.pollEvent()) {
            if (event->is<Event::Closed>()) {
                window.close();
            }
            else if (const auto* resized = event->getIf<Event::Resized>()) {
                updateWindowView(window, resized);
            }
            else if (session.state == MenuState::GroupNameInput) {
                if (const auto* textEvent = event->getIf<Event::TextEntered>()) {
                    handleTextInput(session, textEvent);
                }
            }

            if (const auto* click = event->getIf<Event::MouseButtonPressed>()) {
                handleMouseClicks(window, session, ui, mousePos, click->button);
            }
        }

        renderApp(window, session, ui, mousePos);
    }

    return 0;
}