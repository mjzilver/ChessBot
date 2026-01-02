#include "ChessBoard.h"

#include <cmath>
#include <cstdint>
#include <random>

#include "../Utils/ctz.h"

// board functions
void ChessBoard::resetBoard() {
    emptyBoard();

    // set pawns
    for (int i = 0; i < 8; ++i) {
        setPiece(i, 1, PAWN, BLACK);
        setPiece(i, 6, PAWN, WHITE);
    }

    // set rooks
    setPiece(0, 0, ROOK, BLACK);
    setPiece(7, 0, ROOK, BLACK);
    setPiece(0, 7, ROOK, WHITE);
    setPiece(7, 7, ROOK, WHITE);

    // set knights
    setPiece(1, 0, KNIGHT, BLACK);
    setPiece(6, 0, KNIGHT, BLACK);
    setPiece(1, 7, KNIGHT, WHITE);
    setPiece(6, 7, KNIGHT, WHITE);

    // set bishops
    setPiece(2, 0, BISHOP, BLACK);
    setPiece(5, 0, BISHOP, BLACK);
    setPiece(2, 7, BISHOP, WHITE);
    setPiece(5, 7, BISHOP, WHITE);

    // set queens
    setPiece(3, 0, QUEEN, BLACK);
    setPiece(3, 7, QUEEN, WHITE);

    // set kings
    setPiece(4, 0, KING, BLACK);
    setPiece(4, 7, KING, WHITE);

    gameOver = false;
}

void ChessBoard::emptyBoard() {
    whitePieces = 0;
    blackPieces = 0;
    pieces[PAWN] = 0;
    pieces[ROOK] = 0;
    pieces[KNIGHT] = 0;
    pieces[BISHOP] = 0;
    pieces[QUEEN] = 0;
    pieces[KING] = 0;
}

char ChessBoard::getPieceSymbol(const int x, const int y) const { return pieceTypeToSymbol(getPieceTypeAt(x, y)); }

// piece functions
void ChessBoard::setPiece(const int x, const int y, const PieceType pieceType, const bool isWhite) {
    uint64_t piece = 1ULL << (x + y * 8);
    if (isWhite) {
        whitePieces |= piece;
    } else {
        blackPieces |= piece;
    }
    pieces[pieceType] |= piece;
}

void ChessBoard::removePiece(const int x, const int y, const PieceType pieceType, const bool isWhite) {
    uint64_t piece = 1ULL << (x + y * 8);
    if (isWhite) {
        whitePieces &= ~piece;
    } else {
        blackPieces &= ~piece;
    }
    pieces[pieceType] &= ~piece;
}

bool ChessBoard::isPieceAt(const int x, const int y) const {
    uint64_t piece = 1ULL << (x + y * 8);
    return (whitePieces & piece) || (blackPieces & piece);
}

bool ChessBoard::isPieceAt(const int x, const int y, const bool isWhite) const {
    uint64_t piece = 1ULL << (x + y * 8);
    return isWhite ? (whitePieces & piece) : (blackPieces & piece);
}

bool ChessBoard::isPieceAt(const int x, const int y, const PieceType pieceType) const {
    uint64_t piece = 1ULL << (x + y * 8);
    return pieces[pieceType] & piece;
}

bool ChessBoard::getPieceColor(const int x, const int y) const {
    uint64_t piece = 1ULL << (x + y * 8);
    return whitePieces & piece;
}

bool ChessBoard::removePieceAt(const int x, const int y) {
    if (isPieceAt(x, y)) {
        uint64_t piece = 1ULL << (x + y * 8);
        whitePieces &= ~piece;
        blackPieces &= ~piece;
        for (int i = 0; i < 6; ++i) {
            pieces[i] &= ~piece;
        }
        return true;
    }
    return false;
}

PieceType ChessBoard::getPieceTypeAt(const int x, const int y) const {
    if (isPieceAt(x, y, PAWN)) return PAWN;
    if (isPieceAt(x, y, ROOK)) return ROOK;
    if (isPieceAt(x, y, KNIGHT)) return KNIGHT;
    if (isPieceAt(x, y, BISHOP)) return BISHOP;
    if (isPieceAt(x, y, QUEEN)) return QUEEN;
    if (isPieceAt(x, y, KING)) return KING;
    return EMPTY;
}

bool ChessBoard::movePiece(const int x, const int y, const int newX, const int newY) {
    if (isPieceAt(x, y)) {
        if (isValidAttack(x, y, newX, newY) && isPieceAt(newX, newY, !getPieceColor(x, y))) {
            auto piece = getPieceTypeAt(x, y);
            removePieceAt(newX, newY);
            setPiece(newX, newY, piece, getPieceColor(x, y));
            removePieceAt(x, y);

            return true;
        } else if (isValidMove(x, y, newX, newY)) {
            auto piece = getPieceTypeAt(x, y);
            setPiece(newX, newY, piece, getPieceColor(x, y));
            removePieceAt(x, y);

            return true;
        }
    }
    return false;
}

void ChessBoard::undoMove(const int x, const int y, const int newX, const int newY, const PieceType capturedPiece) {
    bool color = getPieceColor(newX, newY);
    auto piece = getPieceTypeAt(newX, newY);
    setPiece(x, y, piece, color);
    removePieceAt(newX, newY);
    if (capturedPiece != EMPTY) {
        setPiece(newX, newY, capturedPiece, !color);
    }
}

uint64_t ChessBoard::getValidMoves(const int x, const int y) const {
    uint64_t validMoves = 0;
    int64_t emptySquares = ~getBoard();

    // loop over all empty squares and check if the piece can move there
    while (emptySquares) {
        int index = ctz(emptySquares);
        int newX = index % 8;
        int newY = index / 8;

        if (isValidMove(x, y, newX, newY)) {
            validMoves |= 1ULL << index;
        }

        emptySquares &= emptySquares - 1;
    }

    return validMoves;
}

uint64_t ChessBoard::getValidAttacks(const int x, const int y) const {
    uint64_t validAttacks = 0;
    bool color = getPieceColor(x, y);
    int64_t enemyPieces = getPiecesBitmap(!color);

    // loop over all enemy pieces and check if the piece can attack them
    while (enemyPieces) {
        int index = ctz(enemyPieces);
        int enemyX = index % 8;
        int enemyY = index / 8;

        if (isValidAttack(x, y, enemyX, enemyY)) {
            validAttacks |= 1ULL << index;
        }

        enemyPieces &= enemyPieces - 1;
    }

    return validAttacks;
}

bool ChessBoard::isValidMove(const int x, const int y, const int newX, const int newY) const {
    if (isPieceAt(x, y)) {
        auto piece = getPieceTypeAt(x, y);
        auto pieceColor = getPieceColor(x, y);
        // Can't move to a square occupied by a piece of the same color
        if (isPieceAt(newX, newY, pieceColor)) {
            return false;
        }

        bool isWhite = isPieceAt(x, y, true);
        // Direction of movement for white and black pieces
        int direction = isWhite ? -1 : 1;

        switch (piece) {
            case PAWN:
                if (newX == x && newY == y + direction) {
                    if (!isPieceAt(x, y + direction)) {
                        return true;
                    }
                } else if (newX == x && newY == y + 2 * direction && y == (isWhite ? 6 : 1)) {
                    if (!isPieceAt(x, y + direction) && !isPieceAt(x, y + 2 * direction)) {
                        return true;
                    }
                }
                break;
            case KNIGHT:
                // knight can move in an L shape
                return ((newX == x + 1 && newY == y + 2) || (newX == x + 2 && newY == y + 1) ||
                        (newX == x + 2 && newY == y - 1) || (newX == x + 1 && newY == y - 2) ||
                        (newX == x - 1 && newY == y - 2) || (newX == x - 2 && newY == y - 1) ||
                        (newX == x - 2 && newY == y + 1) || (newX == x - 1 && newY == y + 2));
                break;
            case QUEEN:
                if (newX == x || newY == y || newX - x == newY - y || newX - x == y - newY) {
                    // Check for blocking pieces in the path of the queen
                    if (!isPathClear(x, y, newX, newY)) {
                        return false;
                    }
                    return true;
                }
                break;
            case ROOK:
                // rook can move any number of spaces horizontally or vertically
                if (newX == x || newY == y) {
                    if (!isPathClear(x, y, newX, newY)) {
                        return false;
                    }
                    return true;
                }
                break;
            case BISHOP:
                // bishop can move any number of spaces diagonally
                if (newX - x == newY - y || newX - x == y - newY) {
                    if (!isPathClear(x, y, newX, newY)) {
                        return false;
                    }
                    return true;
                }
                break;
            case KING:
                // King can move 1 space in any direction
                if (std::abs(newX - x) <= 1 && std::abs(newY - y) <= 1) {
                    return true;
                }
                break;
            case EMPTY:
                return false;
        }
    }
    return false;
}

bool ChessBoard::isPathClear(const int startX, const int startY, const int endX, const int endY) const {
    // Check if the path from start to end is clear (no pieces in the way)
    int deltaX = (endX > startX) ? 1 : ((endX < startX) ? -1 : 0);
    int deltaY = (endY > startY) ? 1 : ((endY < startY) ? -1 : 0);

    int currentX = startX + deltaX;
    int currentY = startY + deltaY;

    while (currentX != endX || currentY != endY) {
        if (isPieceAt(currentX, currentY)) {
            return false;  // There is a piece in the path
        }
        currentX += deltaX;
        currentY += deltaY;
    }

    return true;  // The path is clear
}

bool ChessBoard::isValidAttack(const int x, const int y, const int newX, const int newY) const {
    if (isPieceAt(x, y)) {
        auto piece = getPieceTypeAt(x, y);
        // Can't attack pieces of the same color
        if (isPieceAt(x, y, true) == isPieceAt(newX, newY, true)) {
            return false;
        }

        bool isWhite = isPieceAt(x, y, true);
        // Direction of movement for white and black pieces
        int direction = isWhite ? -1 : 1;

        switch (piece) {
            case PAWN:
                // Pawn attacks diagonally
                if ((newX == x + 1 || newX == x - 1) && newY == y + direction) {
                    return true;
                }
                break;
            default:
                // For other pieces, use the isValidMove function
                return isValidMove(x, y, newX, newY);
        }
    }
    return false;
}

void ChessBoard::initializeZobristTable() {
    std::mt19937_64 rng(123456);  // Fixed seed for reproducibility
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());

    for (int piece = 0; piece < 12; ++piece) {
        for (int square = 0; square < 64; ++square) {
            zobristTable[piece][square] = dist(rng);
        }
    }

    zobristSideToMove = dist(rng);
}

uint64_t ChessBoard::getBoardHash(const bool isWhiteTurn) const {
    uint64_t hash = 0;

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            PieceType piece = getPieceTypeAt(x, y);
            if (piece != EMPTY) {
                int pieceIndex = getPieceColor(x, y) == WHITE ? piece : piece + 6;
                int squareIndex = y * 8 + x;
                hash ^= zobristTable[pieceIndex][squareIndex];
            }
        }
    }

    // XOR the side to move (flip if black's turn)
    if (!isWhiteTurn) {
        hash ^= zobristSideToMove;
    }

    return hash;
}
