#include "GDisplay.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>

#include "../AI/AI.h"

GDisplay::GDisplay(AI* ai) : squareSize(60), margin(25) {
    loadPieceTextures(pieceTextures);
    if (!font.loadFromFile("./resources/OpenSans-Regular.ttf")) {
        throw std::runtime_error("Error loading font");
    }

    window.create(sf::VideoMode(500, 500), "Chess Game", sf::Style::Titlebar | sf::Style::Close);

    window.setView(window.getDefaultView());
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
    window.setMouseCursorVisible(true);
    window.setActive(true);
    window.clear();
    window.display();

    this->ai = ai;
}

GDisplay::~GDisplay() { pieceTextures.clear(); }

void GDisplay::drawLoop(ChessBoard& board) {
    while (window.isOpen()) {
        handleInput(board);

        if (!isCurrentPlayerWhite && !isAIThreadRunning) {
            isAIThreadRunning = true;
            std::thread aiThread([&]() {
                ai->makeMove(&board, false);

                isCurrentPlayerWhite = true;
                isAIThreadRunning = false;
            });

            aiThread.detach();
        }

        ChessBoard boardCopy;
        {
            std::lock_guard<std::mutex> lock(board.mtx);
            boardCopy = board.clone();
        }

        drawBoard(boardCopy);
    }
}

void GDisplay::handleInput(ChessBoard& board) {
    sf::Event event;
    while (window.pollEvent(event)) {
        handleEvent(event, board);
    }
}

void GDisplay::drawBoard(const ChessBoard& board) {
    window.clear();

    // Draw ranks
    for (int i = 0; i < 8; ++i) {
        sf::Text rankLabel(std::to_string(8 - i), font, 20);
        rankLabel.setPosition(5, i * squareSize + (margin));
        rankLabel.setFillColor(sf::Color::White);
        window.draw(rankLabel);
    }

    // Draw files
    for (int i = 0; i < 8; ++i) {
        sf::Text fileLabel(std::string(1, 'A' + i), font, 20);
        fileLabel.setPosition(i * squareSize + (margin * 2), squareSize * 8);
        fileLabel.setFillColor(sf::Color::White);
        window.draw(fileLabel);
    }

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            if ((y + x) % 2 == 0) {
                drawSquare(x, y, sf::Color(230, 230, 230));
            } else {
                drawSquare(x, y, sf::Color(75, 75, 75));
            }

            // if selected piece is not null, draw possible moves
            if (!selectedPiece.isEmpty()) {
                if (board.isSquareTaken(x, y) && board.isValidAttack(selectedPiece.x, selectedPiece.y, x, y)) {
                    drawSquare(x, y, sf::Color(255, 0, 0, 100));
                } else if (board.isValidMove(selectedPiece.x, selectedPiece.y, x, y)) {
                    drawCircle(x, y, 0.5, sf::Color(0, 255, 0, 100));
                }
            }

            // draw piece on square
            char piece = board.getPieceSymbol(x, y);
            bool isWhite = board.isPieceAt(x, y, true);

            if (piece == EMPTY_SYMBOL) continue;

            char pieceSymbol = isWhite ? toupper(piece) : tolower(piece);

            auto it = pieceTextures.find(pieceSymbol);
            if (it != pieceTextures.end()) {
                // draw a circle around the selected piece
                SelectionPiece currentPiece = {x, y, pieceSymbol, isWhite};

                if (currentPiece == selectedPiece) {
                    drawCircleOutline(x, y, 0.8, sf::Color(0, 0, 255, 100));
                }

                sf::Sprite pieceSprite(it->second);
                pieceSprite.setPosition(x * squareSize + margin, y * squareSize);
                window.draw(pieceSprite);
            } else {
                std::cerr << "Texture not found for piece " << piece << std::endl;
            }
        }
    }

    window.display();
}

void GDisplay::loadPieceTextures(std::map<char, sf::Texture>& pieceTextures) const {
    const std::string imagePath = "./resources/images/";

    // Define the pieces and their corresponding filenames
    // Capital letters are white pieces, lowercase letters are black pieces
    std::map<char, std::string> pieceFilenames = {{'P', "WP.png"}, {'N', "WN.png"}, {'B', "WB.png"}, {'R', "WR.png"},
                                                  {'Q', "WQ.png"}, {'K', "WK.png"}, {'p', "BP.png"}, {'n', "BN.png"},
                                                  {'b', "BB.png"}, {'r', "BR.png"}, {'q', "BQ.png"}, {'k', "BK.png"}};

    for (const auto& entry : pieceFilenames) {
        char piece = entry.first;
        const std::string& filename = entry.second;

        sf::Texture texture;
        if (texture.loadFromFile(imagePath + filename)) {
            pieceTextures[piece] = texture;
        } else {
            throw std::runtime_error("Error loading texture for piece " + std::string(1, piece));
        }
    }
}

void GDisplay::drawCircleOutline(int x, int y, float circleFraction, const sf::Color& borderColor) {
    const float radius = circleFraction * (squareSize / 2);
    const float offset = (squareSize * (1 - circleFraction)) / 2;

    sf::CircleShape border(radius);
    border.setPosition(x * squareSize + margin + offset, y * squareSize + offset);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineThickness(4);
    border.setOutlineColor(borderColor);
    window.draw(border);
}

void GDisplay::drawCircle(int x, int y, float circleFraction, const sf::Color& color) {
    const float radius = circleFraction * (squareSize / 2);
    const float offset = (squareSize * (1 - circleFraction)) / 2;

    sf::CircleShape circle(radius);
    circle.setPosition(x * squareSize + margin + offset, y * squareSize + offset);
    circle.setFillColor(color);
    window.draw(circle);
}

void GDisplay::drawSquare(int x, int y, const sf::Color& color) {
    sf::RectangleShape square(sf::Vector2f(squareSize, squareSize));
    square.setPosition(x * squareSize + margin, y * squareSize);
    square.setFillColor(color);
    window.draw(square);
}

void GDisplay::handleEvent(sf::Event& event, ChessBoard& board) {
    switch (event.type) {
        case sf::Event::Closed:
            window.close();
            break;
        case sf::Event::MouseButtonPressed:
            handleMouseClick(event.mouseButton, board);
            break;
        default:
            break;
    }
}

void GDisplay::handleMouseClick(sf::Event::MouseButtonEvent& mouse, ChessBoard& board) {
    if (mouse.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f viewCoordinates = window.mapPixelToCoords(mousePos);

        int colIndex = static_cast<int>((viewCoordinates.x - margin) / squareSize);
        int rowIndex = static_cast<int>(viewCoordinates.y / squareSize);

        handleValidChessboardClick(colIndex, rowIndex, board);
    } else if (mouse.button == sf::Mouse::Right) {
        selectedPiece.clear();
    }
}

void GDisplay::handleValidChessboardClick(int colIndex, int rowIndex, ChessBoard& board) {
    SelectionPiece clickedPiece = {colIndex, rowIndex, board.getPieceSymbol(colIndex, rowIndex),
                                   board.isPieceAt(colIndex, rowIndex, true)};

    if (!selectedPiece.isEmpty() && isCurrentPlayerWhite && selectedPiece.isWhite) {
        if (board.movePiece(selectedPiece.x, selectedPiece.y, colIndex, rowIndex)) {
            selectedPiece.clear();
            isCurrentPlayerWhite = !isCurrentPlayerWhite;
            return;
        }
    }

    selectedPiece = clickedPiece;
}
