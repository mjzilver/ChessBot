#include "AI/AI.h"
#include "Chess/ChessBoard.h"
#include "Config/Config.h"
#include "UI/ConsoleDisplay.h"
#include "UI/GDisplay.h"
#include "UI/IDisplay.h"

int main() {
    Config& config = Config::getInstance();

    AI ai(config.difficulty, config.timeLimit);
    IDisplay* display = nullptr;
    ChessBoard board;
    board.initializeZobristTable();

    if (config.useGui) {
        display = new GDisplay(&ai);
    } else {
        display = new ConsoleDisplay(&ai);
    }

    display->drawLoop(board);

    delete display;

    return 0;
}