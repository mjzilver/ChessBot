#pragma once

#include <cstdint>
#include <mutex>

#include "PieceType.h"

class ChessBoard {
private:
    bool gameOver = false;

    uint64_t whitePieces = 0;
    uint64_t blackPieces = 0;
    uint64_t pieces[6] = {0, 0, 0, 0, 0, 0};

    uint64_t zobristTable[12][64];
    uint64_t zobristSideToMove;

public:
    ChessBoard() {
        resetBoard();
        initializeZobristTable();
    }

    std::mutex mtx;

    ChessBoard(ChessBoard&& other) noexcept : whitePieces(other.whitePieces), blackPieces(other.blackPieces) {
        for (int i = 0; i < 6; i++) pieces[i] = other.pieces[i];
        zobristSideToMove = other.zobristSideToMove;
        for (int i = 0; i < 12; i++)
            for (int j = 0; j < 64; j++) zobristTable[i][j] = other.zobristTable[i][j];
    }

    ChessBoard& operator=(ChessBoard&& other) noexcept {
        if (this != &other) {
            whitePieces = other.whitePieces;
            blackPieces = other.blackPieces;
            for (int i = 0; i < 6; i++) pieces[i] = other.pieces[i];
            zobristSideToMove = other.zobristSideToMove;
            for (int i = 0; i < 12; i++)
                for (int j = 0; j < 64; j++) zobristTable[i][j] = other.zobristTable[i][j];
        }
        return *this;
    }

    ChessBoard(const ChessBoard&) = delete;

    ChessBoard clone() const {
        ChessBoard copy;
        copy.whitePieces = whitePieces;
        copy.blackPieces = blackPieces;
        for (int i = 0; i < 6; i++) copy.pieces[i] = pieces[i];
        copy.zobristSideToMove = zobristSideToMove;
        for (int i = 0; i < 12; i++)
            for (int j = 0; j < 64; j++) copy.zobristTable[i][j] = zobristTable[i][j];
        return copy;
    }

    static const bool WHITE = true;
    static const bool BLACK = false;

    // board functions
    void resetBoard();
    void emptyBoard();
    uint64_t getBoard() const { return whitePieces | blackPieces; }
    uint64_t getPiecesBitmap(bool isWhite) const { return isWhite ? whitePieces : blackPieces; }

    // Zobrist hashing
    uint64_t getBoardHash(bool isWhiteTurn) const;
    void initializeZobristTable();

    // piece functions
    uint64_t getPieceLocation(int x, int y) const { return 1ULL << (x + y * 8); }
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
    uint64_t getValidAttacks(int x, int y) const;
    bool isValidMove(int x, int y, int newX, int newY) const;
    bool isPathClear(int startX, int startY, int endX, int endY) const;
    bool isValidAttack(int x, int y, int newX, int newY) const;
};