#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "../Chess/ChessBoard.h"
#include "../Chess/PieceType.h"
#include "../Thread/ThreadPool.h"

struct Move {
    int fromX, fromY;
    int toX, toY;
    float score;
};

class AI {
public:
    AI(int maxDepth, int timeLimit) : maxDepth(maxDepth), timeLimit(timeLimit) {}

    AI(const AI&) = delete;
    AI& operator=(const AI&) = delete;

    void makeMove(ChessBoard* board, bool isWhite);

    ~AI() { threadpool.join(); }

private:
    const int maxDepth;
    const int timeLimit;
    std::atomic<int> cacheHitCount{0};
    bool searchRootIsWhite;

    ThreadPool threadpool;

    // move cache
    std::unordered_map<uint64_t, std::vector<Move>> moveCache;
    std::mutex moveCacheMutex;

    std::chrono::steady_clock::time_point startTime;

    // evals
    Move findBestMove(const ChessBoard* const board, bool isWhite);
    float evaluatePosition(const ChessBoard* const board) const;
    int piecePositionScore(int x, int y, const PieceType type, bool isWhite) const;
    float getValueForPiece(const PieceType piece) const;
    float minimax(ChessBoard* const board, int depth, float alpha, float beta, bool isWhiteToMove);

    // move generation
    std::vector<Move> generateMoves(const ChessBoard* const board, bool isWhite);
};