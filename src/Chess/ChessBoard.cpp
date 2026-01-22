#include "ChessBoard.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <random>

#include "./AttackTables.h"
/*
 * Bitboard usage:
 *  - index = x + y * 8   get the index
 *  - 1ULL << index       sets a bit at the square index
 *  - (board & mask)      checks if a square is occupied
 *  - board |= mask       adds a piece
 *  - board &= ~mask      removes a piece
 *
 * Bit shifts are used to generate moves.
 *
 *  - North:              >> 8
 *  - South:              << 8
 *  - East:               << 1
 *  - West:               >> 1
 */

ChessBoard::ChessBoard(ChessBoard&& other) noexcept { copyFrom(other); }

ChessBoard& ChessBoard::operator=(ChessBoard&& other) noexcept {
    if (this != &other) {
        copyFrom(other);
    }
    return *this;
}

ChessBoard ChessBoard::clone() const {
    ChessBoard copy;
    copy.copyFrom(*this);
    return copy;
}

void ChessBoard::copyFrom(const ChessBoard& other) {
    whitePieces = other.whitePieces;
    blackPieces = other.blackPieces;
    for (int i = 0; i < 6; ++i) pieces[i] = other.pieces[i];
    zobristSideToMove = other.zobristSideToMove;

    for (int i = 0; i < 12; ++i) {
        for (int j = 0; j < 64; ++j) {
            zobristTable[i][j] = other.zobristTable[i][j];
        }
    }
}

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

char ChessBoard::getPieceSymbol(int x, int y) const { return pieceTypeToSymbol(getPieceTypeAt(x, y)); }

void ChessBoard::setPiece(int x, int y, const PieceType pieceType, bool isWhite) {
    uint64_t piece = 1ULL << (x + y * 8);
    if (isWhite) {
        whitePieces |= piece;
    } else {
        blackPieces |= piece;
    }
    pieces[pieceType] |= piece;
}

void ChessBoard::removePiece(int x, int y, const PieceType pieceType, bool isWhite) {
    uint64_t piece = 1ULL << (x + y * 8);
    if (isWhite) {
        whitePieces &= ~piece;
    } else {
        blackPieces &= ~piece;
    }
    pieces[pieceType] &= ~piece;
}

bool ChessBoard::isPieceAt(int x, int y) const {
    if (!onBoard(x, y)) return false;
    uint64_t piece = 1ULL << (x + y * 8);
    return (whitePieces & piece) || (blackPieces & piece);
}

bool ChessBoard::isPieceAt(int x, int y, bool isWhite) const {
    if (!onBoard(x, y)) return false;
    uint64_t piece = 1ULL << (x + y * 8);
    return isWhite ? (whitePieces & piece) : (blackPieces & piece);
}

bool ChessBoard::isPieceAt(int x, int y, const PieceType pieceType) const {
    if (!onBoard(x, y)) return false;
    uint64_t piece = 1ULL << (x + y * 8);
    return pieces[pieceType] & piece;
}

bool ChessBoard::getPieceColor(int x, int y) const {
    uint64_t piece = 1ULL << (x + y * 8);
    return (whitePieces & piece) != 0;
}

bool ChessBoard::removePieceAt(int x, int y) {
    if (!onBoard(x, y)) return false;
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

PieceType ChessBoard::getPieceTypeAt(int x, int y) const {
    if (!onBoard(x, y)) return EMPTY;

    if (isPieceAt(x, y, PAWN)) return PAWN;
    if (isPieceAt(x, y, ROOK)) return ROOK;
    if (isPieceAt(x, y, KNIGHT)) return KNIGHT;
    if (isPieceAt(x, y, BISHOP)) return BISHOP;
    if (isPieceAt(x, y, QUEEN)) return QUEEN;
    if (isPieceAt(x, y, KING)) return KING;
    return EMPTY;
}

bool ChessBoard::movePiece(int x, int y, int newX, int newY) {
    if (!onBoard(x, y) || !onBoard(newX, newY)) return false;
    if (!isPieceAt(x, y)) return false;

    auto piece = getPieceTypeAt(x, y);

    // Use getValidMoves to validate
    uint64_t valid = getValidMoves(x, y);
    uint64_t target = 1ULL << (newX + newY * 8);

    if (!(valid & target)) return false;

    // Make move
    removePieceAt(newX, newY);
    setPiece(newX, newY, piece, getPieceColor(x, y));
    removePieceAt(x, y);

    return true;
}

void ChessBoard::undoMove(int x, int y, int newX, int newY, const PieceType capturedPiece) {
    if (!onBoard(x, y) || !onBoard(newX, newY)) return;

    bool color = getPieceColor(newX, newY);
    auto piece = getPieceTypeAt(newX, newY);

    setPiece(x, y, piece, color);
    removePieceAt(newX, newY);

    if (capturedPiece != EMPTY) {
        setPiece(newX, newY, capturedPiece, !color);
    }
}

inline uint64_t pawnPushes(int from, bool white, uint64_t occ) {
    uint64_t fromBB = 1ULL << from;
    uint64_t moves = 0;

    if (white) {
        uint64_t one = fromBB >> 8;
        if (!(one & occ)) {
            moves |= one;

            if ((from >> 3) == 6) {
                uint64_t two = fromBB >> 16;
                if (!(two & occ)) moves |= two;
            }
        }
    } else {
        uint64_t one = fromBB << 8;
        if (!(one & occ)) {
            moves |= one;

            if ((from >> 3) == 1) {
                uint64_t two = fromBB << 16;
                if (!(two & occ)) moves |= two;
            }
        }
    }

    return moves;
}

uint64_t ChessBoard::getValidMoves(int x, int y) const {
    if (!onBoard(x, y) || !isPieceAt(x, y)) return 0;

    bool white = getPieceColor(x, y);
    uint64_t own = white ? whitePieces : blackPieces;
    uint64_t occ = whitePieces | blackPieces;
    uint64_t from = x + (y << 3);

    switch (getPieceTypeAt(x, y)) {
        case KNIGHT:
            return ATTACK_TABLES.knight[from] & ~own;

        case KING:
            return ATTACK_TABLES.king[from] & ~own;

        case PAWN: {
            uint64_t enemy = occ ^ own;
            return pawnPushes(from, white, occ) | (ATTACK_TABLES.pawn[white][from] & enemy);
        }

        case ROOK:
            return rookMoves(from, occ) & ~own;

        case BISHOP:
            return bishopMoves(from, occ) & ~own;

        case QUEEN:
            return (rookMoves(from, occ) | bishopMoves(from, occ)) & ~own;

        default:
            return 0;
    }
}

bool ChessBoard::isValidMove(int x, int y, int newX, int newY) const {
    if (!onBoard(x, y) || !onBoard(newX, newY)) return false;

    uint64_t moveMask = 1ULL << (newX + newY * 8);
    return (getValidMoves(x, y) & moveMask) != 0;
}

bool ChessBoard::isValidAttack(int x, int y, int newX, int newY) const {
    if (!onBoard(x, y) || !onBoard(newX, newY)) return false;
    if (!isPieceAt(newX, newY)) return false;

    return isValidMove(x, y, newX, newY) && (getPieceColor(x, y) != getPieceColor(newX, newY));
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

uint64_t ChessBoard::getBoardHash(bool isWhiteTurn) const {
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

    if (!isWhiteTurn) {
        hash ^= zobristSideToMove;
    }

    return hash;
}
