#include <iostream>
#include "Board.h"
using namespace std;

Board::Board() {
    setupBoard();
}

void Board::setupBoard() {
    char initialBoard[8][8] = {
        {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'},
        {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
        {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
        {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'}
    };

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            board[i][j] = initialBoard[i][j];
        }
    }
}

void Board::displayBoard() {
    cout << endl;

    for (int i = 0; i < 8; i++) {
        cout << 8 - i << " ";

        for (int j = 0; j < 8; j++) {
            cout << board[i][j] << " ";
        }

        cout << endl;
    }

    cout << "  a b c d e f g h" << endl;
}

void Board::movePiece(int startRow, int startCol, int endRow, int endCol) {
    board[endRow][endCol] = board[startRow][startCol];
    board[startRow][startCol] = '.';
}
void Board::movePiece(std::string from, std::string to) {
    int startCol = from[0] - 'a';
    int startRow = 8 - (from[1] - '0');

    int endCol = to[0] - 'a';
    int endRow = 8 - (to[1] - '0');

    movePiece(startRow, startCol, endRow, endCol);
}
