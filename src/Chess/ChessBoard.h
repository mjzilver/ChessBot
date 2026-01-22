#pragma once

#include <cstdint>
#include <mutex>

#include "PieceType.h"

class ChessBoard {
public:
    ChessBoard() {
        resetBoard();
        // initializeZobristTable();
    }

    std::mutex mtx;

    ChessBoard(ChessBoard&& other) noexcept;

    ChessBoard& operator=(ChessBoard&& other) noexcept;

    ChessBoard(const ChessBoard&) = delete;

    ChessBoard clone() const;

    static const bool WHITE = true;
    static const bool BLACK = false;

    // board functions
    void resetBoard();
    void emptyBoard();
    uint64_t getBoard() const { return whitePieces | blackPieces; }
    uint64_t getColorBitboard(bool isWhite) const;
    uint64_t getPieceBitboard(PieceType pieceType, bool isWhite) const;

    // Zobrist hashing
    uint64_t getBoardHash(bool isWhiteTurn) const;
    void initializeZobristTable();

    // piece functions
    char getPieceSymbol(int x, int y) const;
    void setPiece(int x, int y, const PieceType pieceType, bool isWhite);
    void removePiece(int x, int y, const PieceType pieceType, bool isWhite);
    bool isPieceAt(int x, int y) const;
    bool isPieceAt(int x, int y, bool isWhite) const;
    bool isPieceAt(int x, int y, const PieceType pieceType) const;
    bool isSquareEmpty(int x, int y) const { return !isPieceAt(x, y); }
    bool isSquareTaken(int x, int y) const { return isPieceAt(x, y); }
    bool getPieceColor(int x, int y) const;
    bool removePieceAt(int x, int y);
    PieceType getPieceTypeAt(int x, int y) const;
    bool movePiece(int x, int y, int newX, int newY);
    void undoMove(int x, int y, int newX, int newY, const PieceType capturedPiece);
    uint64_t getValidMoves(int x, int y) const;
    bool isValidMove(int x, int y, int newX, int newY) const;
    bool isValidAttack(int x, int y, int newX, int newY) const;

private:
    bool gameOver = false;

    uint64_t whitePieces = 0;
    uint64_t blackPieces = 0;
    uint64_t pieces[6] = {0, 0, 0, 0, 0, 0};

    uint64_t zobristTable[12][64];
    uint64_t zobristSideToMove;
    void copyFrom(const ChessBoard& other);
};