#pragma once

#include "../Chess/ChessBoard.h"

class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual void drawBoard(const ChessBoard& board) = 0;
    virtual void drawLoop(ChessBoard& board) = 0;
    virtual void handleInput(ChessBoard& board) = 0;
};