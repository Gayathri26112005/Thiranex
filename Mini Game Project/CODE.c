#include <iostream>
using namespace std;

char board[3][3];

void initializeBoard() {
    char num = '1';

    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            board[i][j] = num++;
        }
    }
}

void displayBoard() {
    cout << "\n";
    for(int i = 0; i < 3; i++) {
        cout << " ";
        for(int j = 0; j < 3; j++) {
            cout << board[i][j];
            if(j < 2)
                cout << " | ";
        }
        cout << endl;

        if(i < 2)
            cout << "---|---|---" << endl;
    }
    cout << "\n";
}

bool checkWin() {
    // Rows
    for(int i = 0; i < 3; i++) {
        if(board[i][0] == board[i][1] &&
           board[i][1] == board[i][2])
            return true;
    }

    // Columns
    for(int i = 0; i < 3; i++) {
        if(board[0][i] == board[1][i] &&
           board[1][i] == board[2][i])
            return true;
    }

    // Diagonals
    if(board[0][0] == board[1][1] &&
       board[1][1] == board[2][2])
        return true;

    if(board[0][2] == board[1][1] &&
       board[1][1] == board[2][0])
        return true;

    return false;
}

bool isDraw() {
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            if(board[i][j] != 'X' && board[i][j] != 'O')
                return false;
        }
    }
    return true;
}

void makeMove(int position, char symbol) {
    int row = (position - 1) / 3;
    int col = (position - 1) % 3;

    board[row][col] = symbol;
}

bool validMove(int position) {
    if(position < 1 || position > 9)
        return false;

    int row = (position - 1) / 3;
    int col = (position - 1) % 3;

    if(board[row][col] == 'X' || board[row][col] == 'O')
        return false;

    return true;
}

int main() {
    char playAgain;

    do {
        initializeBoard();

        int player = 1;
        int move;
        char symbol;

        while(true) {
            displayBoard();

            if(player == 1)
                symbol = 'X';
            else
                symbol = 'O';

            cout << "Player " << player
                 << " (" << symbol << ") Enter position (1-9): ";
            cin >> move;

            if(!validMove(move)) {
                cout << "Invalid move! Try again.\n";
                continue;
            }

            makeMove(move, symbol);

            if(checkWin()) {
                displayBoard();
                cout << "Player " << player
                     << " wins!\n";
                break;
            }

            if(isDraw()) {
                displayBoard();
                cout << "Game Draw!\n";
                break;
            }

            player = (player == 1) ? 2 : 1;
        }

        cout << "\nPlay Again? (Y/N): ";
        cin >> playAgain;

    } while(playAgain == 'Y' || playAgain == 'y');

    cout << "\nThank you for playing!\n";

    return 0;
}
