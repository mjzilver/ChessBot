#pragma once

#include <array>
#include <cstdint>
#include <cwchar>

#include "../Utils/bits.h"
#include "ChessBoard.h"

enum Direction { N, S, E, W, NE, NW, SE, SW };

struct AttackTables {
    std::array<uint64_t, 64> knight;
    std::array<uint64_t, 64> king;
    std::array<std::array<uint64_t, 64>, 2> pawn;
    std::array<std::array<uint64_t, 8>, 64> rays;
};

constexpr bool onBoard(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }

constexpr AttackTables genAttackTables() {
    AttackTables t{};

    for (int sq = 0; sq < 64; ++sq) {
        int x = sq % 8;
        int y = sq / 8;

        // Knight tables
        const int kdx[8] = {1, 2, 2, 1, -1, -2, -2, -1};
        const int kdy[8] = {2, 1, -1, -2, -2, -1, 1, 2};

        for (int i = 0; i < 8; ++i) {
            int nx = x + kdx[i];
            int ny = y + kdy[i];
            if (onBoard(nx, ny)) t.knight[sq] |= 1ULL << (nx + ny * 8);
        }

        // King tables
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx;
                int ny = y + dy;
                if (onBoard(nx, ny)) t.king[sq] |= 1ULL << (nx + ny * 8);
            }
        }
        // Pawn tables
        if (y > 0) {
            if (x > 0) t.pawn[ChessBoard::WHITE][sq] |= 1ULL << ((x - 1) + (y - 1) * 8);
            if (x < 7) t.pawn[ChessBoard::WHITE][sq] |= 1ULL << ((x + 1) + (y - 1) * 8);
        }

        if (y < 7) {
            if (x > 0) t.pawn[ChessBoard::BLACK][sq] |= 1ULL << ((x - 1) + (y + 1) * 8);
            if (x < 7) t.pawn[ChessBoard::BLACK][sq] |= 1ULL << ((x + 1) + (y + 1) * 8);
        }

        // Ray tables (per direction)
        auto gen = [&](int dx, int dy, int dir) {
            int nx = x + dx;
            int ny = y + dy;
            while (onBoard(nx, ny)) {
                t.rays[sq][dir] |= 1ULL << (nx + ny * 8);
                nx += dx;
                ny += dy;
            }
        };

        gen(0, -1, N);
        gen(0, 1, S);
        gen(1, 0, E);
        gen(-1, 0, W);
        gen(1, -1, NE);
        gen(-1, -1, NW);
        gen(1, 1, SE);
        gen(-1, 1, SW);
    }

    return t;
}

inline constexpr AttackTables ATTACK_TABLES = genAttackTables();

inline uint64_t getRayMoves(int sq, Direction dir, uint64_t occupied) {
    uint64_t ray = ATTACK_TABLES.rays[sq][dir];
    uint64_t blockers = ray & occupied;

    if (!blockers) return ray;

    // Find closest blocker
    int blockerSq = (dir == N || dir == W || dir == NW || dir == NE) ? msb(blockers) : ctz(blockers);

    // Remove everything beyond blocker
    return ray ^ ATTACK_TABLES.rays[blockerSq][dir];
}

inline uint64_t rookMoves(int sq, uint64_t occupied) {
    return getRayMoves(sq, N, occupied) | getRayMoves(sq, S, occupied) | getRayMoves(sq, E, occupied) |
           getRayMoves(sq, W, occupied);
}

inline uint64_t bishopMoves(int sq, uint64_t occupied) {
    return getRayMoves(sq, NE, occupied) | getRayMoves(sq, NW, occupied) | getRayMoves(sq, SE, occupied) |
           getRayMoves(sq, SW, occupied);
}

inline uint64_t queenMoves(int sq, uint64_t occupied) { return bishopMoves(sq, occupied) | rookMoves(sq, occupied); }