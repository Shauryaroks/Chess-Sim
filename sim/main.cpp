#include <iostream>
#include "Board.h"
using namespace std;

int main() {
    Board chessBoard;
    string from, to;
    cout << "Welcome to Chess!" << endl;
    chessBoard.displayBoard();
    cout << endl << "Make your move." << endl;
    cout << "Enter your move, for example e2 e4: ";
    cin >> from >> to;
    
    chessBoard.movePiece(from, to);
    chessBoard.displayBoard();
    return 0;
}