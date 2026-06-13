#ifndef BOARD_H
#define BOARD_H
#include <string>

class Board {
private:
    char board[8][8];

public:
    Board();
    void setupBoard();
    void displayBoard();
    void movePiece(int startRow, int startCol, int endRow, int endCol);
    void movePiece(std::string from, std::string to);
};

#endif