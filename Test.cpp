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

    return 0;
}