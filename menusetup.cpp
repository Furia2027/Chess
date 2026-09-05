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
const int TOTAL_OPTIONS = 3;

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
void renderChessBoard(RenderWindow& window, char board[8][8], int selRow = -1, int selCol = -1, bool hasSel = false) {
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
// MAIN APPLICATION
// ============================================================================

int main() {
    loadAssets();

    RenderWindow window(VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "UTAR Chess - Unified System");
    window.setFramerateLimit(60);

    MenuState state = MenuState::GroupNameInput;
    string groupName = "";

    // Board States
    char activeBoard[8][8];
    int activeMoves = 0;

    // Selection Tracking
    int selectedRow = -1;
    int selectedCol = -1;
    bool hasSelection = false;

    // Saved Game Cache
    char savedBoard[8][8];
    int savedMoves = 0;
    bool hasSaveData = false;

    // Right-Click Context Menu State
    bool isContextMenuOpen = false;
    Vector2f contextMenuPos(0.f, 0.f);

    // Toast Notification Trackers
    string notificationMsg = "";
    sf::Clock notificationClock;
    sf::Clock cursorClock;

    // Main UI Buttons
    const float btnWidth = 320.f;
    const float btnHeight = 46.f;
    const float btnX = (WINDOW_SIZE - btnWidth) / 2.f;

    MenuButton btnStart{ { { btnX, 230.f }, { btnWidth, btnHeight } }, "1. Start New Game" };
    MenuButton btnLoad{ { { btnX, 290.f }, { btnWidth, btnHeight } }, "2. Load Game" };
    MenuButton btnHistory{ { { btnX, 350.f }, { btnWidth, btnHeight } }, "3. Display Move History" };
    MenuButton btnExit{ { { btnX, 410.f }, { btnWidth, btnHeight } }, "4. Exit" };

    MenuButton btnBackFromLoad{ { { (WINDOW_SIZE - 200.f) / 2.f, 560.f }, { 200.f, 40.f } }, "Back to Menu" };
    MenuButton btnPlaySaved{ { { (WINDOW_SIZE - 200.f) / 2.f, 510.f }, { 200.f, 40.f } }, "Play Game" };
    MenuButton btnBackFromHist{ { { (WINDOW_SIZE - 200.f) / 2.f, 560.f }, { 200.f, 40.f } }, "Back to Menu" };
    MenuButton btnConfirmGroup{ { { (WINDOW_SIZE - 180.f) / 2.f, 380.f }, { 180.f, 44.f } }, "Confirm" };

    MenuButton btnReturnMenu{ { { 10.f, 10.f }, { 110.f, 32.f } }, "< Menu" };

    while (window.isOpen()) {
        Vector2i mousePixel = Mouse::getPosition(window);
        Vector2f mousePos = window.mapPixelToCoords(mousePixel);

        while (const auto event = window.pollEvent()) {
            if (event->is<Event::Closed>()) {
                window.close();
            }

            // Aspect ratio scaling on resize
            if (const auto* resized = event->getIf<Event::Resized>()) {
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

            // Keyboard Text Input for Group Name
            if (state == MenuState::GroupNameInput) {
                if (const auto* textEvent = event->getIf<Event::TextEntered>()) {
                    char32_t unicode = textEvent->unicode;
                    if (unicode == 8) {
                        if (!groupName.empty()) groupName.pop_back();
                    }
                    else if (unicode == 13 || unicode == 10) {
                        if (!groupName.empty()) {
                            state = MenuState::MainMenu;
                            notificationMsg = "Welcome, Team " + groupName + "!";
                            notificationClock.restart();
                        }
                    }
                    else if (unicode >= 32 && unicode < 127 && groupName.size() < 18) {
                        groupName += static_cast<char>(unicode);
                    }
                }
            }

            // Mouse Click Handling
            if (const auto* click = event->getIf<Event::MouseButtonPressed>()) {
                if (click->button == Mouse::Button::Right && state == MenuState::InGame) {
                    isContextMenuOpen = true;
                    contextMenuPos = mousePos;
                }
                else if (click->button == Mouse::Button::Left) {
                    if (state == MenuState::GroupNameInput) {
                        if (btnConfirmGroup.isHovered(mousePos) && !groupName.empty()) {
                            state = MenuState::MainMenu;
                            notificationMsg = "Welcome, Team " + groupName + "!";
                            notificationClock.restart();
                        }
                    }
                    else if (state == MenuState::MainMenu) {
                        if (btnStart.isHovered(mousePos)) {
                            memcpy(activeBoard, INITIAL_BOARD, sizeof(INITIAL_BOARD));
                            activeMoves = 0;
                            hasSelection = false;
                            isContextMenuOpen = false;
                            state = MenuState::InGame;
                        }
                        else if (btnLoad.isHovered(mousePos)) {
                            hasSaveData = loadSaveGame(savedBoard, savedMoves);
                            state = MenuState::LoadGamePreview;
                        }
                        else if (btnHistory.isHovered(mousePos)) {
                            state = MenuState::MoveHistory;
                        }
                        else if (btnExit.isHovered(mousePos)) {
                            window.close();
                        }
                    }
                    else if (state == MenuState::LoadGamePreview) {
                        if (btnBackFromLoad.isHovered(mousePos)) {
                            state = MenuState::MainMenu;
                        }
                        else if (btnPlaySaved.isHovered(mousePos)) {
                            memcpy(activeBoard, savedBoard, sizeof(savedBoard));
                            activeMoves = savedMoves;
                            hasSelection = false;
                            isContextMenuOpen = false;
                            state = MenuState::InGame;
                        }
                    }
                    else if (state == MenuState::MoveHistory) {
                        if (btnBackFromHist.isHovered(mousePos)) {
                            state = MenuState::MainMenu;
                        }
                    }
                    else if (state == MenuState::InGame) {
                        // Priority 1: Handle Context Menu Clicking
                        if (isContextMenuOpen) {
                            if (mousePos.x >= contextMenuPos.x && mousePos.x <= contextMenuPos.x + MENU_WIDTH &&
                                mousePos.y >= contextMenuPos.y && mousePos.y <= contextMenuPos.y + (OPTION_HEIGHT * TOTAL_OPTIONS)) {

                                int option = static_cast<int>((mousePos.y - contextMenuPos.y) / OPTION_HEIGHT);
                                switch (option) {
                                case 0:
                                    if (saveGame("savegame.txt", activeBoard, activeMoves)) {
                                        notificationMsg = "Game Saved!";
                                        notificationClock.restart();
                                    }
                                    break;
                                case 1:
                                    if (loadSaveGame(activeBoard, activeMoves)) {
                                        notificationMsg = "Game Loaded!";
                                        notificationClock.restart();
                                    }
                                    break;
                                case 2:
                                    if (saveGameStats("game_stats.txt", groupName, activeMoves)) {
                                        notificationMsg = "Stats Logged!";
                                        notificationClock.restart();
                                    }
                                    break;
                                }
                            }
                            isContextMenuOpen = false;
                        }
                        // Priority 2: In-Game Return Button
                        else if (btnReturnMenu.isHovered(mousePos)) {
                            state = MenuState::MainMenu;
                            hasSelection = false;
                        }
                        // Priority 3: Board Interactions
                        else {
                            const float tileSize = WINDOW_SIZE / 8.f;
                            int col = static_cast<int>(mousePos.x / tileSize);
                            int row = static_cast<int>(mousePos.y / tileSize);

                            if (row >= 0 && row < 8 && col >= 0 && col < 8) {
                                PieceColour targetColour = getPieceColour(row, col, activeBoard);

                                if (!hasSelection) {
                                    if (targetColour != PieceColour::empty && targetColour != PieceColour::invalid) {
                                        selectedRow = row;
                                        selectedCol = col;
                                        hasSelection = true;
                                    }
                                }
                                else {
                                    if (selectedRow == row && selectedCol == col) {
                                        hasSelection = false; // Deselect
                                    }
                                    else {
                                        PieceColour sourceColour = getPieceColour(selectedRow, selectedCol, activeBoard);
                                        // Friendly fire check: Prevent capturing own piece
                                        if (sourceColour != targetColour) {
                                            activeBoard[row][col] = activeBoard[selectedRow][selectedCol];
                                            activeBoard[selectedRow][selectedCol] = '.';
                                            activeMoves++;
                                        }
                                        hasSelection = false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- RENDERING PIPELINE ---
        window.clear(bgDark);

        if (state != MenuState::InGame) {
            const float checkSize = 20.f;
            for (unsigned int i = 0; i < WINDOW_SIZE / (unsigned int)checkSize; ++i) {
                RectangleShape chk({ checkSize, 6.f });
                chk.setPosition({ (float)i * checkSize, 0.f });
                chk.setFillColor((i % 2 == 0) ? lightSquare : darkSquare);
                window.draw(chk);
            }
        }

        if (state == MenuState::GroupNameInput) {
            RectangleShape card({ 440.f, 320.f });
            card.setPosition({ (WINDOW_SIZE - 440.f) / 2.f, 160.f });
            card.setFillColor(panelBg);
            card.setOutlineColor(darkSquare);
            card.setOutlineThickness(2.f);
            window.draw(card);

            Text header(menuFont, "WELCOME TO CHESS SYSTEM", 22);
            header.setFillColor(lightSquare);
            FloatRect hb = header.getLocalBounds();
            header.setOrigin({ hb.position.x + hb.size.x / 2.f, hb.position.y + hb.size.y / 2.f });
            header.setPosition({ WINDOW_SIZE / 2.f, 205.f });
            window.draw(header);

            Text prompt(menuFont, "Enter Group / Team Name to Start:", 15);
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
            if (static_cast<int>(cursorClock.getElapsedTime().asSeconds() * 2) % 2 == 0) displayText += "_";

            Text inputText(menuFont, displayText, 18);
            inputText.setFillColor(Color::White);
            inputText.setPosition({ (WINDOW_SIZE - 300.f) / 2.f, 307.f });
            window.draw(inputText);

            btnConfirmGroup.draw(window, menuFont, mousePos);
        }
        else if (state == MenuState::MainMenu) {
            Text title(menuFont, "CHESS SYSTEM", 32);
            title.setFillColor(lightSquare);
            FloatRect tb = title.getLocalBounds();
            title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
            title.setPosition({ WINDOW_SIZE / 2.f, 75.f });
            window.draw(title);

            Text sub(menuFont, "Team: [" + groupName + "]", 18);
            sub.setFillColor(highlightColor);
            FloatRect sb = sub.getLocalBounds();
            sub.setOrigin({ sb.position.x + sb.size.x / 2.f, sb.position.y + sb.size.y / 2.f });
            sub.setPosition({ WINDOW_SIZE / 2.f, 120.f });
            window.draw(sub);

            btnStart.draw(window, menuFont, mousePos);
            btnLoad.draw(window, menuFont, mousePos);
            btnHistory.draw(window, menuFont, mousePos);
            btnExit.draw(window, menuFont, mousePos);
        }
        else if (state == MenuState::LoadGamePreview) {
            Text title(menuFont, "SAVED GAME PREVIEW", 24);
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
                btnPlaySaved.draw(window, menuFont, mousePos);
            }
            btnBackFromLoad.draw(window, menuFont, mousePos);
        }
        else if (state == MenuState::MoveHistory) {
            Text title(menuFont, "MATCH STATISTICS", 24);
            title.setFillColor(lightSquare);
            FloatRect tb = title.getLocalBounds();
            title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
            title.setPosition({ WINDOW_SIZE / 2.f, 45.f });
            window.draw(title);

            vector<string> stats = loadStatsLines();
            float lineY = 115.f;
            for (size_t i = max(0, (int)stats.size() - 10); i < stats.size(); ++i) {
                Text entry(menuFont, to_string(i + 1) + ". " + stats[i], 15);
                entry.setFillColor(lightSquare);
                entry.setPosition({ 80.f, lineY });
                window.draw(entry);
                lineY += 34.f;
            }
            btnBackFromHist.draw(window, menuFont, mousePos);
        }
        else if (state == MenuState::InGame) {
            renderChessBoard(window, activeBoard, selectedRow, selectedCol, hasSelection);
            btnReturnMenu.draw(window, menuFont, mousePos);

            // Right-Click Overlay Context Menu
            if (isContextMenuOpen) {
                string optionLabels[TOTAL_OPTIONS] = { "Save Game", "Load Game", "Log Stats" };

                RectangleShape menuBox({ MENU_WIDTH, OPTION_HEIGHT * TOTAL_OPTIONS });
                menuBox.setPosition(contextMenuPos);
                menuBox.setFillColor(Color(40, 40, 40, 240));
                menuBox.setOutlineColor(Color::White);
                menuBox.setOutlineThickness(1.f);
                window.draw(menuBox);

                for (int i = 0; i < TOTAL_OPTIONS; i++) {
                    Text optionText(menuFont, optionLabels[i], 14);
                    optionText.setFillColor(Color::White);
                    optionText.setPosition({ contextMenuPos.x + 10.f, contextMenuPos.y + (i * OPTION_HEIGHT) + 8.f });
                    window.draw(optionText);
                }
            }
        }

        // Notification Overlay Toast
        float elapsed = notificationClock.getElapsedTime().asSeconds();
        if (elapsed < 1.6f && !notificationMsg.empty()) {
            float alphaRatio = 1.0f - (elapsed / 1.6f);
            uint8_t alpha = static_cast<uint8_t>(255 * alphaRatio);

            RectangleShape toastBox({ 300.f, 46.f });
            toastBox.setPosition({ (WINDOW_SIZE - 300.f) / 2.f, (WINDOW_SIZE - 46.f) / 2.f });
            toastBox.setFillColor(Color(20, 20, 20, static_cast<uint8_t>(220 * alphaRatio)));
            toastBox.setOutlineColor(Color(255, 255, 255, alpha));
            toastBox.setOutlineThickness(1.5f);
            window.draw(toastBox);

            Text toastText(menuFont, notificationMsg, 16);
            toastText.setFillColor(Color(255, 255, 255, alpha));
            FloatRect tb = toastText.getLocalBounds();
            toastText.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
            toastText.setPosition({ WINDOW_SIZE / 2.f, WINDOW_SIZE / 2.f });
            window.draw(toastText);
        }

        window.display();
    }

    return 0;
}