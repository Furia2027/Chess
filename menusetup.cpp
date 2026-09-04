#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <cstdint>

using namespace std;
using namespace sf;

// Dimensions & Color Palette (matching FileName.cpp)
const unsigned int WINDOW_SIZE = 640;
const Color lightSquare(220, 220, 180);      // Light Green / Cream
const Color darkSquare(120, 145, 80);        // Dark Green
const Color highlightColor(245, 245, 0, 220);// Highlight Yellow
const Color bgDark(34, 38, 30);              // Deep Theme Background
const Color panelBg(45, 52, 40, 245);        // Card / Container Background
const Color btnNormal(52, 62, 46);           // Normal Button Fill
const Color btnHover(120, 145, 80);          // Hover Button Fill (matches darkSquare)
const Color btnBorder(160, 185, 120);        // Button Border Accent

// Global Assets
Texture pieceTextures[128];
Font menuFont;

// UI View States
enum class MenuState {
    GroupNameInput,
    MainMenu,
    LoadGamePreview,
    MoveHistory
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

// Load piece textures & font
bool loadAssets() {
    if (!menuFont.openFromFile("arial.ttf")) {
        cout << "Warning: Could not load arial.ttf\n";
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

// Function to launch the game executable
void launchChessGame(RenderWindow& window, string& notificationMsg, sf::Clock& notificationClock) {
    const string candidatePaths[] = {
        "FileName.exe",
        "Chess.exe",
        "Debug\\Chess.exe",
        "x64\\Debug\\Chess.exe",
        "Release\\Chess.exe",
        "x64\\Release\\Chess.exe"
    };

    string targetPath = "";
    for (const auto& path : candidatePaths) {
        if (filesystem::exists(path)) {
            targetPath = path;
            break;
        }
    }

    if (targetPath.empty()) {
        notificationMsg = "Game exe not found!";
        notificationClock.restart();
        return;
    }

    notificationMsg = "Launching Game...";
    notificationClock.restart();

    // Temporarily hide menu window while playing game
    window.setVisible(false);
    string command = "\"" + targetPath + "\"";
    system(command.c_str());
    window.setVisible(true);

    notificationMsg = "Welcome Back!";
    notificationClock.restart();
}

// Load savegame file
bool loadSaveGame(char board[8][8], int& moves) {
    ifstream file("savegame.txt");
    if (!file.is_open()) return false;
    file >> moves;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) file >> board[r][c];
    }
    return true;
}

// Load match statistics lines
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

int main() {
    loadAssets();

    RenderWindow window(VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "UTAR Chess - System Menu");
    window.setFramerateLimit(60);

    MenuState state = MenuState::GroupNameInput;
    string groupName = "";

    // Saved board preview data
    char savedBoard[8][8];
    int savedMoves = 0;
    bool hasSaveData = false;

    // Notification toast tracking
    string notificationMsg = "";
    sf::Clock notificationClock;

    // Blinking cursor clock for text input
    sf::Clock cursorClock;

    // Main Menu Buttons
    const float btnWidth = 320.f;
    const float btnHeight = 46.f;
    const float btnX = (WINDOW_SIZE - btnWidth) / 2.f;

    MenuButton btnStart{ { { btnX, 230.f }, { btnWidth, btnHeight } }, "1. Start Game" };
    MenuButton btnLoad{ { { btnX, 290.f }, { btnWidth, btnHeight } }, "2. Load Game" };
    MenuButton btnHistory{ { { btnX, 350.f }, { btnWidth, btnHeight } }, "3. Display Move History" };
    MenuButton btnExit{ { { btnX, 410.f }, { btnWidth, btnHeight } }, "4. Exit" };

    // Sub-view Back buttons
    MenuButton btnBackFromLoad{ { { (WINDOW_SIZE - 200.f) / 2.f, 560.f }, { 200.f, 40.f } }, "Back to Menu" };
    MenuButton btnPlaySaved{ { { (WINDOW_SIZE - 200.f) / 2.f, 510.f }, { 200.f, 40.f } }, "Play Game" };
    MenuButton btnBackFromHist{ { { (WINDOW_SIZE - 200.f) / 2.f, 560.f }, { 200.f, 40.f } }, "Back to Menu" };

    // Group Name Confirmation Button
    MenuButton btnConfirmGroup{ { { (WINDOW_SIZE - 180.f) / 2.f, 380.f }, { 180.f, 44.f } }, "Confirm" };

    while (window.isOpen()) {
        Vector2i mousePixel = Mouse::getPosition(window);
        Vector2f mousePos = window.mapPixelToCoords(mousePixel);

        while (const auto event = window.pollEvent()) {
            if (event->is<Event::Closed>()) {
                window.close();
            }

            // Text Input for Group Name
            if (state == MenuState::GroupNameInput) {
                if (const auto* textEvent = event->getIf<Event::TextEntered>()) {
                    char32_t unicode = textEvent->unicode;
                    if (unicode == 8) { // Backspace
                        if (!groupName.empty()) groupName.pop_back();
                    }
                    else if (unicode == 13 || unicode == 10) { // Enter key
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

            // Mouse Clicks
            if (const auto* click = event->getIf<Event::MouseButtonPressed>()) {
                if (click->button == Mouse::Button::Left) {
                    if (state == MenuState::GroupNameInput) {
                        if (btnConfirmGroup.isHovered(mousePos) && !groupName.empty()) {
                            state = MenuState::MainMenu;
                            notificationMsg = "Welcome, Team " + groupName + "!";
                            notificationClock.restart();
                        }
                    }
                    else if (state == MenuState::MainMenu) {
                        if (btnStart.isHovered(mousePos)) {
                            launchChessGame(window, notificationMsg, notificationClock);
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
                            launchChessGame(window, notificationMsg, notificationClock);
                        }
                    }
                    else if (state == MenuState::MoveHistory) {
                        if (btnBackFromHist.isHovered(mousePos)) {
                            state = MenuState::MainMenu;
                        }
                    }
                }
            }
        }

        // --- RENDERING ---
        window.clear(bgDark);

        // Decorative checkered top border (using lightSquare & darkSquare theme)
        const float checkSize = 20.f;
        for (unsigned int i = 0; i < WINDOW_SIZE / (unsigned int)checkSize; ++i) {
            RectangleShape chk({ checkSize, 6.f });
            chk.setPosition({ (float)i * checkSize, 0.f });
            chk.setFillColor((i % 2 == 0) ? lightSquare : darkSquare);
            window.draw(chk);
        }

        // 1. STATE: GROUP NAME INPUT
        if (state == MenuState::GroupNameInput) {
            // Center Dialog Container Box
            RectangleShape card({ 440.f, 320.f });
            card.setPosition({ (WINDOW_SIZE - 440.f) / 2.f, 160.f });
            card.setFillColor(panelBg);
            card.setOutlineColor(darkSquare);
            card.setOutlineThickness(2.f);
            window.draw(card);

            // Title
            Text header(menuFont, "WELCOME TO CHESS SYSTEM", 22);
            header.setFillColor(lightSquare);
            FloatRect hb = header.getLocalBounds();
            header.setOrigin({ hb.position.x + hb.size.x / 2.f, hb.position.y + hb.size.y / 2.f });
            header.setPosition({ WINDOW_SIZE / 2.f, 205.f });
            window.draw(header);

            // Subtitle
            Text prompt(menuFont, "Enter Group / Team Name to Start:", 15);
            prompt.setFillColor(Color(200, 200, 200));
            FloatRect pb = prompt.getLocalBounds();
            prompt.setOrigin({ pb.position.x + pb.size.x / 2.f, pb.position.y + pb.size.y / 2.f });
            prompt.setPosition({ WINDOW_SIZE / 2.f, 255.f });
            window.draw(prompt);

            // Text Input Box
            RectangleShape inputBox({ 320.f, 44.f });
            inputBox.setPosition({ (WINDOW_SIZE - 320.f) / 2.f, 295.f });
            inputBox.setFillColor(Color(25, 28, 22));
            inputBox.setOutlineColor(highlightColor);
            inputBox.setOutlineThickness(1.5f);
            window.draw(inputBox);

            // Display Entered Name with Blinking Cursor
            string displayText = groupName;
            bool showCursor = (static_cast<int>(cursorClock.getElapsedTime().asSeconds() * 2) % 2 == 0);
            if (showCursor) displayText += "_";

            Text inputText(menuFont, displayText, 18);
            inputText.setFillColor(Color::White);
            FloatRect ib = inputText.getLocalBounds();
            inputText.setPosition({ (WINDOW_SIZE - 300.f) / 2.f, 307.f });
            window.draw(inputText);

            // Confirm Button
            btnConfirmGroup.draw(window, menuFont, mousePos);
        }

        // 2. STATE: MAIN MENU
        else if (state == MenuState::MainMenu) {
            // Header Area
            Text title(menuFont, "CHESS SYSTEM", 32);
            title.setFillColor(lightSquare);
            FloatRect tb = title.getLocalBounds();
            title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
            title.setPosition({ WINDOW_SIZE / 2.f, 75.f });
            window.draw(title);

            // Team Subtitle
            Text sub(menuFont, "Team: [" + groupName + "]", 18);
            sub.setFillColor(highlightColor);
            FloatRect sb = sub.getLocalBounds();
            sub.setOrigin({ sb.position.x + sb.size.x / 2.f, sb.position.y + sb.size.y / 2.f });
            sub.setPosition({ WINDOW_SIZE / 2.f, 120.f });
            window.draw(sub);

            // Decorative King Sprites on either side of Title (from assets)
            if (pieceTextures[(int)'K'].getSize().x > 0) {
                Sprite whiteKing(pieceTextures[(int)'K']);
                Vector2u sz = pieceTextures[(int)'K'].getSize();
                whiteKing.setScale({ 50.f / sz.x, 50.f / sz.y });
                whiteKing.setPosition({ 60.f, 55.f });
                window.draw(whiteKing);
            }
            if (pieceTextures[(int)'k'].getSize().x > 0) {
                Sprite blackKing(pieceTextures[(int)'k']);
                Vector2u sz = pieceTextures[(int)'k'].getSize();
                blackKing.setScale({ 50.f / sz.x, 50.f / sz.y });
                blackKing.setPosition({ WINDOW_SIZE - 110.f, 55.f });
                window.draw(blackKing);
            }

            // Divider Line
            RectangleShape line({ 420.f, 2.f });
            line.setPosition({ (WINDOW_SIZE - 420.f) / 2.f, 155.f });
            line.setFillColor(darkSquare);
            window.draw(line);

            // Draw Menu Buttons
            btnStart.draw(window, menuFont, mousePos);
            btnLoad.draw(window, menuFont, mousePos);
            btnHistory.draw(window, menuFont, mousePos);
            btnExit.draw(window, menuFont, mousePos);

            // Footer Info
            Text footer(menuFont, "Select an option to proceed", 13);
            footer.setFillColor(Color(140, 150, 130));
            FloatRect fb = footer.getLocalBounds();
            footer.setOrigin({ fb.position.x + fb.size.x / 2.f, fb.position.y + fb.size.y / 2.f });
            footer.setPosition({ WINDOW_SIZE / 2.f, 580.f });
            window.draw(footer);
        }

        // 3. STATE: LOAD GAME PREVIEW
        else if (state == MenuState::LoadGamePreview) {
            Text title(menuFont, "SAVED GAME PREVIEW", 24);
            title.setFillColor(lightSquare);
            FloatRect tb = title.getLocalBounds();
            title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
            title.setPosition({ WINDOW_SIZE / 2.f, 45.f });
            window.draw(title);

            if (hasSaveData) {
                Text info(menuFont, "File: savegame.txt  |  Total Moves: " + to_string(savedMoves), 15);
                info.setFillColor(highlightColor);
                FloatRect ib = info.getLocalBounds();
                info.setOrigin({ ib.position.x + ib.size.x / 2.f, ib.position.y + ib.size.y / 2.f });
                info.setPosition({ WINDOW_SIZE / 2.f, 80.f });
                window.draw(info);

                // Render 8x8 Mini Chessboard
                const float tileSize = 42.f;
                const float startX = (WINDOW_SIZE - tileSize * 8.f) / 2.f;
                const float startY = 115.f;

                // Board border
                RectangleShape boardBorder({ tileSize * 8.f + 6.f, tileSize * 8.f + 6.f });
                boardBorder.setPosition({ startX - 3.f, startY - 3.f });
                boardBorder.setFillColor(Color::Transparent);
                boardBorder.setOutlineColor(darkSquare);
                boardBorder.setOutlineThickness(3.f);
                window.draw(boardBorder);

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
            else {
                Text noSave(menuFont, "No savegame.txt found or file could not be read.", 16);
                noSave.setFillColor(Color(230, 100, 100));
                FloatRect nb = noSave.getLocalBounds();
                noSave.setOrigin({ nb.position.x + nb.size.x / 2.f, nb.position.y + nb.size.y / 2.f });
                noSave.setPosition({ WINDOW_SIZE / 2.f, 250.f });
                window.draw(noSave);
            }

            btnBackFromLoad.draw(window, menuFont, mousePos);
        }

        // 4. STATE: MOVE HISTORY / STATS
        else if (state == MenuState::MoveHistory) {
            Text title(menuFont, "MATCH STATISTICS & HISTORY", 24);
            title.setFillColor(lightSquare);
            FloatRect tb = title.getLocalBounds();
            title.setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
            title.setPosition({ WINDOW_SIZE / 2.f, 45.f });
            window.draw(title);

            // Container card
            RectangleShape card({ 520.f, 420.f });
            card.setPosition({ (WINDOW_SIZE - 520.f) / 2.f, 90.f });
            card.setFillColor(panelBg);
            card.setOutlineColor(darkSquare);
            card.setOutlineThickness(2.f);
            window.draw(card);

            vector<string> stats = loadStatsLines();
            if (stats.empty()) {
                Text noStats(menuFont, "No match statistics recorded yet in game_stats.txt", 16);
                noStats.setFillColor(Color(180, 180, 180));
                FloatRect nsb = noStats.getLocalBounds();
                noStats.setOrigin({ nsb.position.x + nsb.size.x / 2.f, nsb.position.y + nsb.size.y / 2.f });
                noStats.setPosition({ WINDOW_SIZE / 2.f, 260.f });
                window.draw(noStats);
            }
            else {
                // Show up to the last 10 records
                int startIdx = max(0, (int)stats.size() - 10);
                float lineY = 115.f;
                for (size_t i = startIdx; i < stats.size(); ++i) {
                    Text entry(menuFont, to_string(i + 1) + ". " + stats[i], 15);
                    entry.setFillColor(lightSquare);
                    entry.setPosition({ 80.f, lineY });
                    window.draw(entry);
                    lineY += 34.f;
                }
            }

            btnBackFromHist.draw(window, menuFont, mousePos);
        }

        // Toast Notification Banner (matching FileName.cpp fade duration & style)
        float fadeDuration = 1.6f;
        float elapsed = notificationClock.getElapsedTime().asSeconds();
        if (elapsed < fadeDuration && !notificationMsg.empty()) {
            float alphaRatio = 1.0f - (elapsed / fadeDuration);
            uint8_t alpha = static_cast<uint8_t>(255 * alphaRatio);
            uint8_t bgAlpha = static_cast<uint8_t>(220 * alphaRatio);

            RectangleShape toastBox({ 300.f, 46.f });
            toastBox.setPosition({ (WINDOW_SIZE - 300.f) / 2.f, (WINDOW_SIZE - 46.f) / 2.f });
            toastBox.setFillColor(Color(20, 20, 20, bgAlpha));
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