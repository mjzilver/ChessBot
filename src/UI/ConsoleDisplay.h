#pragma once

#include "../AI/AI.h"
#include "IDisplay.h"

class ConsoleDisplay : public IDisplay {
public:
    ConsoleDisplay(AI* ai) : IDisplay(), ai(ai) { }
    void drawBoard(const ChessBoard& board) override;
    std::string receiveInput();
    void drawLoop(ChessBoard& board) override;
    bool makeMove(ChessBoard& board, const std::string& moveInput);
    void handleInput(ChessBoard& board) override;

private:
    AI* ai;
    bool isCurrentPlayerWhite = true;
};