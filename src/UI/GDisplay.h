#pragma once

#include <SFML/Graphics.hpp>
#include <map>

#include "../AI/AI.h"
#include "../Chess/ChessBoard.h"
#include "IDisplay.h"

class GDisplay : public IDisplay {
public:
    GDisplay(AI *ai);
    ~GDisplay();

    void drawBoard(const ChessBoard &board) override;
    void drawLoop(ChessBoard &board) override;
    void handleInput(ChessBoard &board) override;

    struct SelectionPiece {
        int x, y = 0;
        char symbol = EMPTY_SYMBOL;
        bool isWhite = true;

        void clear() {
            x = y = 0;
            symbol = EMPTY_SYMBOL;
        }

        bool isEmpty() const { return symbol == EMPTY_SYMBOL; }

        bool operator==(const SelectionPiece &other) const {
            return x == other.x && y == other.y && symbol == other.symbol && isWhite == other.isWhite;
        }
    };

private:
    sf::RenderWindow window;
    int squareSize;
    int margin;
    std::map<char, sf::Texture> pieceTextures;
    sf::Font font;
    SelectionPiece selectedPiece;
    std::atomic<bool> isCurrentPlayerWhite{true};
    std::atomic<bool> isAIThreadRunning{false};

    AI *ai;

    // Load textures
    void loadPieceTextures(std::map<char, sf::Texture> &pieceTextures) const;

    // Drawing functions
    void drawSquare(int x, int y, const sf::Color &color);
    void drawCircleOutline(int x, int y, float circleFraction, const sf::Color &borderColor);
    void drawCircle(int x, int y, float circleFraction, const sf::Color &color);

    // Event handlers for selecting and moving pieces
    void handleEvent(sf::Event &event, ChessBoard &board);
    void handleMouseClick(sf::Event::MouseButtonEvent &mouse, ChessBoard &board);
    void handleValidChessboardClick(int colIndex, int rowIndex, ChessBoard &board);
};
