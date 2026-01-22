#include "AI.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../Chess/ChessBoard.h"
#include "../Utils/bits.h"
#include "PieceSqTable.h"

void AI::makeMove(ChessBoard* board, bool isWhite) {
    startTime = std::chrono::steady_clock::now();
    searchRootIsWhite = isWhite;

    ChessBoard boardCopy;
    {
        std::lock_guard<std::mutex> lock(board->mtx);
        boardCopy = std::move(board->clone());
    }

    Move bestMove = findBestMove(&boardCopy, isWhite);

    board->mtx.lock();
    if (!board->movePiece(bestMove.fromX, bestMove.fromY, bestMove.toX, bestMove.toY)) {
        std::cout << "AI move failed, this should not happen" << std::endl;
    }
    board->mtx.unlock();

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Time to find best move " << duration.count() << " milliseconds\n";
}

Move AI::findBestMove(const ChessBoard* const board, bool isWhite) {
    float bestScore = -10000.0f;
    Move bestMove{};

    auto moves = generateMoves(board, isWhite);

    std::cout << "Cache hit count: " << cacheHitCount << std::endl;
    std::cout << "Moves evaluated: " << evaluatedMoves << std::endl;
    evaluatedMoves = 0;
    cacheHitCount = 0;

    if (moves.empty()) return bestMove;

    std::mutex mutex;
    std::atomic<int> remainingTasks(moves.size());
    std::condition_variable cv;
    std::mutex cvMutex;

    for (const auto& move : moves) {
        threadpool.submit([&, move]() {
            ChessBoard newBoard = board->clone();

            newBoard.movePiece(move.fromX, move.fromY, move.toX, move.toY);

            bool nextIsWhite = !isWhite;
            float score = minimax(&newBoard, maxDepth - 1, -1e9f, 1e9f, nextIsWhite);

            {
                std::lock_guard<std::mutex> lock(mutex);
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
            }

            if (--remainingTasks == 0) {
                std::lock_guard<std::mutex> lock(cvMutex);
                cv.notify_one();
            }
        });
    }

    {
        std::unique_lock<std::mutex> lock(cvMutex);
        cv.wait(lock, [&]() { return remainingTasks == 0; });
    }

    threadpool.join();

    std::cout << "Best score: " << bestScore << std::endl;

    return bestMove;
}

std::vector<Move> AI::generateMoves(const ChessBoard* const board, bool isWhite) {
    uint64_t boardHash = board->getBoardHash(isWhite);

    {
        std::lock_guard<std::mutex> lock(moveCacheMutex);
        auto it = moveCache.find(boardHash);
        if (it != moveCache.end()) {
            cacheHitCount++;
            return std::vector<Move>(it->second);
        }
    }

    auto availableMoves = std::vector<Move>();
    uint64_t boardPieces = board->getColorBitboard(isWhite);

    while (boardPieces) {
        int index = ctz(boardPieces);
        int x = index % 8;
        int y = index / 8;

        auto moves = board->getValidMoves(x, y);

        while (moves) {
            int moveIndex = ctz(moves);
            int newX = moveIndex % 8;
            int newY = moveIndex / 8;

            Move move = {x, y, newX, newY, 0};

            availableMoves.push_back(move);

            moves &= moves - 1;
        }

        boardPieces &= boardPieces - 1;
    }

    {
        std::lock_guard<std::mutex> lock(moveCacheMutex);
        moveCache[boardHash] = availableMoves;
    }
    return availableMoves;
}

float AI::minimax(ChessBoard* const board, int depth, float alpha, float beta, bool isWhiteToMove) {
    if (depth == 0) {
        return evaluatePosition(board);
    }

    auto currentTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
    if (duration.count() > timeLimit) {
        return evaluatePosition(board);
    }

    const bool maximizingPlayer = (isWhiteToMove == searchRootIsWhite);
    float bestScore = maximizingPlayer ? -1e9f : 1e9f;

    auto moves = generateMoves(board, isWhiteToMove);

    for (const auto& move : moves) {
        evaluatedMoves++;

        auto captured = board->getPieceTypeAt(move.toX, move.toY);
        board->movePiece(move.fromX, move.fromY, move.toX, move.toY);

        float score = minimax(board, depth - 1, alpha, beta, !isWhiteToMove);

        board->undoMove(move.fromX, move.fromY, move.toX, move.toY, captured);

        if (maximizingPlayer) {
            if (score > bestScore) bestScore = score;
            if (score > alpha) alpha = score;
        } else {
            if (score < bestScore) bestScore = score;
            if (score < beta) beta = score;
        }

        if (beta <= alpha) {
            break;  // alpha-beta cutoff
        }
    }

    return bestScore;
}

float AI::evaluatePosition(const ChessBoard* const board) const {
    auto evalColor = [&](bool isWhite) -> float {
        float score = 0.0f;

        uint64_t pawns = board->getPieceBitboard(PAWN, isWhite);
        uint64_t rooks = board->getPieceBitboard(ROOK, isWhite);
        uint64_t knights = board->getPieceBitboard(KNIGHT, isWhite);
        uint64_t bishops = board->getPieceBitboard(BISHOP, isWhite);
        uint64_t queens = board->getPieceBitboard(QUEEN, isWhite);
        uint64_t kings = board->getPieceBitboard(KING, isWhite);

        auto accumulatePieceScore = [&](uint64_t bits, PieceType piece) {
            while (bits) {
                int idx = ctz(bits);
                int x = idx & 7;
                int y = idx >> 3;

                score += getValueForPiece(piece);
                score += piecePositionScore(x, y, piece, isWhite);

                bits &= bits - 1;
            }
        };

        accumulatePieceScore(pawns, PAWN);
        accumulatePieceScore(rooks, ROOK);
        accumulatePieceScore(knights, KNIGHT);
        accumulatePieceScore(bishops, BISHOP);
        accumulatePieceScore(queens, QUEEN);
        accumulatePieceScore(kings, KING);

        return score;
    };

    float whiteScore = evalColor(ChessBoard::WHITE);
    float blackScore = evalColor(ChessBoard::BLACK);

    float score = whiteScore - blackScore;
    return searchRootIsWhite ? score : -score;
}

int AI::piecePositionScore(int x, int y, const PieceType type, bool isWhite) const {
    int idx = y * 8 + x;

    if (!isWhite) {
        idx = 63 - idx;
    }

    switch (type) {
        case PAWN:
            return PAWN_TABLE[idx];
        case KNIGHT:
            return KNIGHT_TABLE[idx];
        case BISHOP:
            return BISHOP_TABLE[idx];
        case ROOK:
            return ROOK_TABLE[idx];
        case QUEEN:
            return QUEEN_TABLE[idx];
        case KING:
            return KING_TABLE[idx];
        default:
            return 0;
    }
}

float AI::getValueForPiece(const PieceType piece) const {
    switch (piece) {
        case PAWN:
            return 100;
        case KNIGHT:
            return 320;
        case BISHOP:
            return 330;
        case ROOK:
            return 500;
        case QUEEN:
            return 900;
        case KING:
            return 20000;
        default:
            return 0;
    }
}