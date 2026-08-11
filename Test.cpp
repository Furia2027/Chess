#include <iostream>
#include <string>
#include <vector>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// ANSI Escape Codes
#define RESET             "\033[0m"
#define BOLD              "\033[1m"
#define COLOR_WHITE_PIECE "\033[1;36m"
#define COLOR_BLACK_PIECE "\033[1;31m"
#define BG_LIGHT          "\033[47m"
#define BG_DARK           "\033[100m"
#define TEXT_LIGHT        "\033[30m"
#define TEXT_DARK         "\033[97m"

enum Color { NONE, WHITE, BLACK };

struct Piece {
    char symbol;
    Color color;
};

struct GameState {
    Piece board[8][8];
    Color currentTurn;
    int moveCount;
};

void displayChessGameUI(GameState& game) {
    // 1. Console Initialization & Screen Reset
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    // Clears terminal buffer so rendering starts from line 1 (row 0)
    system("cls");
#endif

    // 2. Game State Initialization
    game.currentTurn = WHITE;
    game.moveCount = 1;

    char backRow[] = { 'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R' };
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (r == 0)      game.board[r][c] = { static_cast<char>(tolower(backRow[c])), BLACK };
            else if (r == 1) game.board[r][c] = { 'p', BLACK };
            else if (r == 6) game.board[r][c] = { 'P', WHITE };
            else if (r == 7) game.board[r][c] = { backRow[c], WHITE };
            else             game.board[r][c] = { '.', NONE };
        }
    }

    // 3. UI Rendering: Header Banner
    cout << "┌──────────────────────────────────────────┐\n";
    cout << "│              CONSOLE CHESS               │\n";
    cout << "└──────────────────────────────────────────┘\n\n";

    // Top Column Coordinates
    cout << "     a   b   c   d   e   f   g   h   \n";
    cout << "   ┌───┬───┬───┬───┬───┬───┬───┬───┐\n";

    for (int r = 0; r < 8; ++r) {
        // Left Rank Number
        cout << " " << (8 - r) << " │";

        // Board Tiles
        for (int c = 0; c < 8; ++c) {
            bool isLightSquare = (r + c) % 2 == 0;
            string bg = isLightSquare ? BG_LIGHT : BG_DARK;
            Piece p = game.board[r][c];
            string pieceColor = (p.color == WHITE) ? COLOR_WHITE_PIECE : COLOR_BLACK_PIECE;

            cout << bg;
            if (p.symbol == '.') {
                cout << (isLightSquare ? TEXT_LIGHT : TEXT_DARK) << " . " << RESET;
            }
            else {
                cout << " " << pieceColor << BOLD << p.symbol << RESET << bg << " " << RESET;
            }
            cout << "│";
        }

        // Right Rank Number & Sidebar Status Panel
        cout << " " << (8 - r);
        if (r == 1) cout << "   " << BOLD << "MATCH STATUS" << RESET;
        if (r == 2) cout << "   Turn : " << (game.currentTurn == WHITE ? COLOR_WHITE_PIECE "WHITE" RESET : COLOR_BLACK_PIECE "BLACK" RESET);
        if (r == 3) cout << "   Moves: " << game.moveCount;
        if (r == 5) cout << "   " << COLOR_WHITE_PIECE << "P/R/N/B/Q/K" << RESET << " = White";
        if (r == 6) cout << "   " << COLOR_BLACK_PIECE << "p/r/n/b/q/k" << RESET << " = Black";
        cout << "\n";

        if (r < 7) {
            cout << "   ├───┼───┼───┼───┼───┼───┼───┼───┤\n";
        }
    }

    // Bottom Column Coordinates & Prompt
    cout << "   └───┴───┴───┴───┴───┴───┴───┴───┘\n";
    cout << "     a   b   c   d   e   f   g   h   \n\n";
    cout << BOLD << "Enter Move (e.g., 'e2 e4') or 'undo': " << RESET;
}

int main() {
    GameState game;
    displayChessGameUI(game);
    return 0;
}