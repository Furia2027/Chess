#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cctype>

using namespace std;
using namespace sf;

// Dimensions & Constants
const unsigned int WINDOW_SIZE = 640;
const int TILE_SIZE = 80;
const float MENU_WIDTH = 150.f;
const float OPTION_HEIGHT = 35.f;
const int TOTAL_OPTIONS = 4;

// Color Palette
const Color lightSquare(220, 220, 180);
const Color darkSquare(120, 145, 80);
const Color highlightColor(245, 245, 0, 180);
const Color bgDark(34, 38, 30);
const Color panelBg(45, 52, 40, 245);
const Color btnNormal(52, 62, 46);
const Color btnHover(120, 145, 80);
const Color btnBorder(160, 185, 120);

// Default Starting Board
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

// Global Assets
Texture pieceTextures[128];
Font menuFont;

// Piece Color & Type Definitions
enum class PieceColour { white, black, empty, invalid };
enum class PieceType { pawn, rook, knight, bishop, queen, king, empty, invalid };

// UI View States
enum class MenuState {
    GroupNameInput,
    MainMenu,
    LoadGamePreview,
    MoveHistory,
    InGame
};

// UI Button Helper
struct MenuButton {
    FloatRect rect;
    string label;

    bool isHovered(Vector2f mousePos) const {
        return rect.contains(mousePos);
    }

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

// ============================================================================
// FILE I/O & DATA OPERATIONS
// ============================================================================

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

bool loadSaveGame(char board[8][8], int& moves) {
    ifstream file("savegame.txt");
    if (!file.is_open()) return false;
    file >> moves;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) file >> board[r][c];
    }
    return true;
}

bool saveGameStats(const string& filename, const string& winner, int moves) {
    ofstream file(filename, ios::app);
    if (!file.is_open()) return false;
    file << "Winner: " << winner << " | Total Moves: " << moves << endl;
    return true;
}

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

// ============================================================================
// CHESS PIECE EVALUATION HELPERS
// ============================================================================

PieceColour getPieceColour(int row, int col, const char board[8][8]) {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return PieceColour::invalid;
    char piece = board[row][col];
    if (piece == '.') return PieceColour::empty;
    if (isupper(piece)) return PieceColour::white;
    if (islower(piece)) return PieceColour::black;
    return PieceColour::invalid;
}

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

// Chessboard Renderer
void renderChessBoard(RenderWindow& window, const char board[8][8], int selRow = -1, int selCol = -1, bool hasSel = false) {
    const float tileSize = WINDOW_SIZE / 8.f;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            RectangleShape tile({ tileSize, tileSize });
            tile.setPosition({ c * tileSize, r * tileSize });
            tile.setFillColor(((r + c) % 2 == 0) ? lightSquare : darkSquare);
            window.draw(tile);

            if (hasSel && r == selRow && c == selCol) {
                RectangleShape highlight({ tileSize, tileSize });
                highlight.setPosition({ c * tileSize, r * tileSize });
                highlight.setFillColor(highlightColor);
                window.draw(highlight);
            }

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
}

// ============================================================================
// REUSABLE UI COMPONENT
// ============================================================================

class ChessUI {
public:
    // Action and Navigation Buttons
    MenuButton btnStart;
    MenuButton btnLoad;
    MenuButton btnHistory;
    MenuButton btnExit;
    MenuButton btnBackFromLoad;
    MenuButton btnPlaySaved;
    MenuButton btnBackFromHist;
    MenuButton btnConfirmGroup;

    ChessUI() {
        const float btnWidth = 320.f;
        const float btnHeight = 46.f;
        const float btnX = (WINDOW_SIZE - btnWidth) / 2.f;

        btnStart = { { { btnX, 230.f }, { btnWidth, btnHeight } }, "1. Start New Game" };
        btnLoad = { { { btnX, 290.f }, { btnWidth, btnHeight } }, "2. Load Game" };
        btnHistory = { { { btnX, 350.f }, { btnWidth, btnHeight } }, "3. Display Move History" };
        btnExit = { { { btnX, 410.f }, { btnWidth, btnHeight } }, "4. Exit" };

        btnBackFromLoad = { { { (WINDOW_SIZE - 200.f) / 2.f, 560.f }, { 200.f, 40.f } }, "Back to Menu" };
        btnPlaySaved = { { { (WINDOW_SIZE - 200.f) / 2.f, 510.f }, { 200.f, 40.f } }, "Play Game" };
        btnBackFromHist = { { { (WINDOW_SIZE - 200.f) / 2.f, 560.f }, { 200.f, 40.f } }, "Back to Menu" };
        btnConfirmGroup = { { { (WINDOW_SIZE - 180.f) / 2.f, 380.f }, { 180.f, 44.f } }, "Confirm" };
    }

    // Top decorative checkered border
    void renderTopBorder(RenderWindow& window) const {
        const float checkSize = 20.f;
        for (unsigned int i = 0; i < WINDOW_SIZE / (unsigned int)checkSize; ++i) {
            RectangleShape chk({ checkSize, 6.f });
            chk.setPosition({ (float)i * checkSize, 0.f });
            chk.setFillColor((i % 2 == 0) ? lightSquare : darkSquare);
            window.draw(chk);
        }
    }

    // Screen: Group Name Input
    void renderGroupNameInput(RenderWindow& window, Font& font, const string& groupName, bool showCursor, Vector2f mousePos) const {
        RectangleShape card({ 440.f, 320.f });
        card.setPosition({ (WINDOW_SIZE - 440.f) / 2.f, 160.f });
        card.setFillColor(panelBg);
        card.setOutlineColor(darkSquare);
        card.setOutlineThickness(2.f);
        window.draw(card);

        Text header(font, "WELCOME TO CHESS SYSTEM", 22);
        header.setFillColor(lightSquare);
        FloatRect hb = header.getLocalBounds();
        header.setOrigin({ hb.position.x + hb.size.x / 2.f, hb.position.y + hb.size.y / 2.f });
        header.setPosition({ WINDOW_SIZE / 2.f, 205.f });
        window.draw(header);

        Text prompt(font, "Enter Group / Team Name to Start:", 15);
        prompt.setFillColor(Color(200, 200, 200));
        FloatRect pb = prompt.getLocalBounds();
        prompt.setOrigin({ pb.position.x + pb.size.x / 2.f, pb.position.y + pb.size.y / 2.f });
        prompt.setPosition({ WINDOW_SIZE / 2.f, 255.f });
        window.draw(prompt);

        RectangleShape inputBox({ 320.f, 44.f });
        inputBox.setPosition({ (WINDOW_SIZE - 320.f) / 2.f, 295.f });
        inputBox.setFillColor(Color(25, 28, 22));
        inputBox.setOutlineColor(highlightColor);
        inputBox.setOutlineThickness(1.5f);
        window.draw(inputBox);

        string displayText = groupName;
        if (showCursor) displayText += "_";

        Text inputText(font, displayText, 18);
        inputText.setFillColor(Color::White);
        inputText.setPosition({ (WINDOW_SIZE - 300.f) / 2.f, 307.f });
        window.draw(inputText);

        btnConfirmGroup.draw(window, font, mousePos);
    }

    // Screen: Main Menu
    void renderMainMenu(RenderWindow& window, Font& font, const string& groupName, Vector2f mousePos) const {
        Text title(font, "CHESS SYSTEM", 32);
        title.setFillColor(lightSquare);
        FloatRect tb = title.getLocalBounds();
        title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
        title.setPosition({ WINDOW_SIZE / 2.f, 75.f });
        window.draw(title);

        Text sub(font, "Team: [" + groupName + "]", 18);
        sub.setFillColor(highlightColor);
        FloatRect sb = sub.getLocalBounds();
        sub.setOrigin({ sb.position.x + sb.size.x / 2.f, sb.position.y + sb.size.y / 2.f });
        sub.setPosition({ WINDOW_SIZE / 2.f, 118.f });
        window.draw(sub);

        // Alert Box under Team Name
        const float alertWidth = 460.f;
        const float alertHeight = 40.f;
        const float alertX = (WINDOW_SIZE - alertWidth) / 2.f;
        const float alertY = 158.f;

        RectangleShape alertBox({ alertWidth, alertHeight });
        alertBox.setPosition({ alertX, alertY });
        alertBox.setFillColor(Color(40, 48, 35, 230));
        alertBox.setOutlineColor(highlightColor);
        alertBox.setOutlineThickness(1.5f);
        window.draw(alertBox);

        // Accent strip on the left side of the alert box
        RectangleShape accentStrip({ 4.f, alertHeight });
        accentStrip.setPosition({ alertX, alertY });
        accentStrip.setFillColor(highlightColor);
        window.draw(accentStrip);

        Text alertText(font, "[!] Right-click during game for menu options (Save/Load/Menu)", 13);
        alertText.setFillColor(Color(240, 240, 220));
        FloatRect ab = alertText.getLocalBounds();
        alertText.setOrigin({ ab.position.x + ab.size.x / 2.f, ab.position.y + ab.size.y / 2.f });
        alertText.setPosition({ alertX + alertWidth / 2.f + 4.f, alertY + alertHeight / 2.f });
        window.draw(alertText);

        btnStart.draw(window, font, mousePos);
        btnLoad.draw(window, font, mousePos);
        btnHistory.draw(window, font, mousePos);
        btnExit.draw(window, font, mousePos);
    }

    // Screen: Saved Game Preview
    void renderLoadGamePreview(RenderWindow& window, Font& font, bool hasSaveData, const char savedBoard[8][8], Vector2f mousePos) const {
        Text title(font, "SAVED GAME PREVIEW", 24);
        title.setFillColor(lightSquare);
        FloatRect tb = title.getLocalBounds();
        title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
        title.setPosition({ WINDOW_SIZE / 2.f, 45.f });
        window.draw(title);

        if (hasSaveData) {
            const float tileSize = 42.f;
            const float startX = (WINDOW_SIZE - tileSize * 8.f) / 2.f;
            const float startY = 115.f;

            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    RectangleShape tile({ tileSize, tileSize });
                    tile.setPosition({ startX + c * tileSize, startY + r * tileSize });
                    tile.setFillColor(((r + c) % 2 == 0) ? lightSquare : darkSquare);
                    window.draw(tile);

                    char piece = savedBoard[r][c];
                    if (piece != '.' && pieceTextures[(int)piece].getSize().x > 0) {
                        Sprite sprite(pieceTextures[(int)piece]);
                        Vector2u sz = pieceTextures[(int)piece].getSize();
                        sprite.setScale({ tileSize / sz.x, tileSize / sz.y });
                        sprite.setPosition({ startX + c * tileSize, startY + r * tileSize });
                        window.draw(sprite);
                    }
                }
            }
            btnPlaySaved.draw(window, font, mousePos);
        }
        btnBackFromLoad.draw(window, font, mousePos);
    }

    // Screen: Match Statistics & History
    void renderMoveHistory(RenderWindow& window, Font& font, const vector<string>& stats, Vector2f mousePos) const {
        Text title(font, "MATCH STATISTICS", 24);
        title.setFillColor(lightSquare);
        FloatRect tb = title.getLocalBounds();
        title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
        title.setPosition({ WINDOW_SIZE / 2.f, 45.f });
        window.draw(title);

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

    // Right-Click Context Menu Overlay
    void renderContextMenu(RenderWindow& window, Font& font, Vector2f pos) const {
        string optionLabels[TOTAL_OPTIONS] = { "Save Game", "Load Game", "Log Stats", "Back to Menu" };

        RectangleShape menuBox({ MENU_WIDTH, OPTION_HEIGHT * TOTAL_OPTIONS });
        menuBox.setPosition(pos);
        menuBox.setFillColor(Color(40, 40, 40, 240));
        menuBox.setOutlineColor(Color::White);
        menuBox.setOutlineThickness(1.f);
        window.draw(menuBox);

        for (int i = 0; i < TOTAL_OPTIONS; i++) {
            Text optionText(font, optionLabels[i], 14);
            optionText.setFillColor(Color::White);
            optionText.setPosition({ pos.x + 10.f, pos.y + (i * OPTION_HEIGHT) + 8.f });
            window.draw(optionText);
        }
    }

    // Context Menu Option Hit Test (returns 0: Save, 1: Load, 2: Log Stats, 3: Back to Menu, or -1 if outside)
    int getContextMenuOption(Vector2f mousePos, Vector2f contextMenuPos) const {
        if (mousePos.x >= contextMenuPos.x && mousePos.x <= contextMenuPos.x + MENU_WIDTH &&
            mousePos.y >= contextMenuPos.y && mousePos.y <= contextMenuPos.y + (OPTION_HEIGHT * TOTAL_OPTIONS)) {
            return static_cast<int>((mousePos.y - contextMenuPos.y) / OPTION_HEIGHT);
        }
        return -1;
    }

    // Screen: In-Game (Chessboard + Context Menu if open)
    void renderInGame(RenderWindow& window, Font& font, const char board[8][8],
        int selectedRow, int selectedCol, bool hasSelection,
        bool isContextMenuOpen, Vector2f contextMenuPos) const {
        renderChessBoard(window, board, selectedRow, selectedCol, hasSelection);

        if (isContextMenuOpen) {
            renderContextMenu(window, font, contextMenuPos);
        }
    }

    // Temporary Notification Overlay Toast
    void renderToastNotification(RenderWindow& window, Font& font, const string& msg, float elapsed, float fadeDuration = 1.6f) const {
        if (elapsed >= fadeDuration || msg.empty()) return;

        float alphaRatio = 1.0f - (elapsed / fadeDuration);
        uint8_t alpha = static_cast<uint8_t>(255 * alphaRatio);

        RectangleShape toastBox({ 300.f, 46.f });
        toastBox.setPosition({ (WINDOW_SIZE - 300.f) / 2.f, (WINDOW_SIZE - 46.f) / 2.f });
        toastBox.setFillColor(Color(20, 20, 20, static_cast<uint8_t>(220 * alphaRatio)));
        toastBox.setOutlineColor(Color(255, 255, 255, alpha));
        toastBox.setOutlineThickness(1.5f);
        window.draw(toastBox);

        Text toastText(font, msg, 16);
        toastText.setFillColor(Color(255, 255, 255, alpha));
        FloatRect tb = toastText.getLocalBounds();
        toastText.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
        toastText.setPosition({ WINDOW_SIZE / 2.f, WINDOW_SIZE / 2.f });
        window.draw(toastText);
    }
};

// ============================================================================
// GAME SESSION DATA STATE
// ============================================================================

struct GameSession {
    MenuState state = MenuState::GroupNameInput;
    string groupName = "";

    // Active Board State
    char activeBoard[8][8];
    int activeMoves = 0;

    // Selection Tracking
    int selectedRow = -1;
    int selectedCol = -1;
    bool hasSelection = false;

    // Saved Game Cache (for Preview)
    char savedBoard[8][8];
    int savedMoves = 0;
    bool hasSaveData = false;

    // Context Menu State
    bool isContextMenuOpen = false;
    Vector2f contextMenuPos{ 0.f, 0.f };

    // Toast Notification Trackers
    string notificationMsg = "";
    sf::Clock notificationClock;
    sf::Clock cursorClock;

    void notify(const string& msg) {
        notificationMsg = msg;
        notificationClock.restart();
    }

    void resetToNewGame() {
        memcpy(activeBoard, INITIAL_BOARD, sizeof(INITIAL_BOARD));
        activeMoves = 0;
        hasSelection = false;
        selectedRow = -1;
        selectedCol = -1;
        isContextMenuOpen = false;
        state = MenuState::InGame;
    }

    void loadSavedToActive() {
        memcpy(activeBoard, savedBoard, sizeof(savedBoard));
        activeMoves = savedMoves;
        hasSelection = false;
        selectedRow = -1;
        selectedCol = -1;
        isContextMenuOpen = false;
        state = MenuState::InGame;
    }
};

// ============================================================================
// EVENT & INPUT PROCESSING METHODS
// ============================================================================

// Window viewport aspect ratio adjustment on resize
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

// Group name keyboard typing handler
void handleTextInput(GameSession& session, const Event::TextEntered* textEvent) {
    char32_t unicode = textEvent->unicode;
    if (unicode == 8) { // Backspace
        if (!session.groupName.empty()) session.groupName.pop_back();
    }
    else if (unicode == 13 || unicode == 10) { // Enter key confirms
        if (!session.groupName.empty()) {
            session.state = MenuState::MainMenu;
            session.notify("Welcome, Team " + session.groupName + "!");
        }
    }
    else if (unicode >= 32 && unicode < 127 && session.groupName.size() < 18) {
        session.groupName += static_cast<char>(unicode);
    }
}

// Right-click context menu action dispatcher
void handleContextMenuAction(GameSession& session, int option) {
    switch (option) {
    case 0: // Save Game
        if (saveGame("savegame.txt", session.activeBoard, session.activeMoves)) {
            session.notify("Game Saved!");
        }
        break;
    case 1: // Load Game
        if (loadSaveGame(session.activeBoard, session.activeMoves)) {
            session.hasSelection = false;
            session.selectedRow = -1;
            session.selectedCol = -1;
            session.notify("Game Loaded!");
        }
        break;
    case 2: // Log Stats
        if (saveGameStats("game_stats.txt", session.groupName, session.activeMoves)) {
            session.notify("Stats Logged!");
        }
        break;
    case 3: // Back to Menu
        session.state = MenuState::MainMenu;
        session.hasSelection = false;
        session.selectedRow = -1;
        session.selectedCol = -1;
        session.notify("Returned to Menu");
        break;
    }
}

// Chessboard click and piece movement logic
void handleInGamePieceMove(GameSession& session, Vector2f mousePos) {
    const float tileSize = WINDOW_SIZE / 8.f;
    int col = static_cast<int>(mousePos.x / tileSize);
    int row = static_cast<int>(mousePos.y / tileSize);

    if (row < 0 || row >= 8 || col < 0 || col >= 8) return;

    PieceColour targetColour = getPieceColour(row, col, session.activeBoard);

    if (!session.hasSelection) {
        if (targetColour != PieceColour::empty && targetColour != PieceColour::invalid) {
            session.selectedRow = row;
            session.selectedCol = col;
            session.hasSelection = true;
        }
    }
    else {
        if (session.selectedRow == row && session.selectedCol == col) {
            session.hasSelection = false; // Deselect on clicking same tile
        }
        else {
            PieceColour sourceColour = getPieceColour(session.selectedRow, session.selectedCol, session.activeBoard);
            // Friendly fire check: Prevent capturing own piece
            if (sourceColour != targetColour) {
                session.activeBoard[row][col] = session.activeBoard[session.selectedRow][session.selectedCol];
                session.activeBoard[session.selectedRow][session.selectedCol] = '.';
                session.activeMoves++;
            }
            session.hasSelection = false;
        }
    }
}

// Master mouse click event router
void handleMouseClicks(RenderWindow& window, GameSession& session, const ChessUI& ui, Vector2f mousePos, Mouse::Button button) {
    // Right click inside gameplay opens the context menu
    if (button == Mouse::Button::Right && session.state == MenuState::InGame) {
        session.isContextMenuOpen = true;
        session.contextMenuPos = mousePos;
        return;
    }

    if (button != Mouse::Button::Left) return;

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
        if (session.isContextMenuOpen) {
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

// Master rendering pipeline method
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
        ui.renderInGame(window, menuFont, session.activeBoard,
            session.selectedRow, session.selectedCol, session.hasSelection,
            session.isContextMenuOpen, session.contextMenuPos);
        break;
    }

    // Toast Notification Overlay
    ui.renderToastNotification(window, menuFont, session.notificationMsg, session.notificationClock.getElapsedTime().asSeconds());

    window.display();
}

// ============================================================================
// MAIN APPLICATION
// ============================================================================

int main() {
    loadAssets();

    RenderWindow window(VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "UTAR Chess - Unified System");
    window.setFramerateLimit(60);

    GameSession session;
    ChessUI ui;

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