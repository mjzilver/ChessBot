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
#include "../Utils/ctz.h"
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

    std::cout << "Number of moves: " << moves.size() << std::endl;
    std::cout << "Cache hit count: " << cacheHitCount << std::endl;
    cacheHitCount = 0;

    if (moves.empty()) return bestMove;

    std::mutex mutex;
    std::atomic<int> remainingTasks(moves.size());
    std::condition_variable cv;
    std::mutex cvMutex;

    for (const auto& move : moves) {
        threadpool.submit([&, move]() {
            ChessBoard newBoard = board->clone();

            auto captured = newBoard.getPieceTypeAt(move.toX, move.toY);
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
    uint64_t boardPieces = board->getPiecesBitmap(isWhite);

    while (boardPieces) {
        int index = ctz(boardPieces);
        int x = index % 8;
        int y = index / 8;

        auto attacks = board->getValidAttacks(x, y);
        auto moves = board->getValidMoves(x, y);

        while (attacks) {
            int attackIndex = ctz(attacks);
            int newX = attackIndex % 8;
            int newY = attackIndex / 8;

            Move move = {x, y, newX, newY, 0};

            availableMoves.push_back(move);

            attacks &= attacks - 1;
        }

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
    float whiteScore = 0.0f;
    float blackScore = 0.0f;

    auto evaluatePieces = [&](uint64_t pieces, bool isWhite) {
        float score = 0.0f;
        while (pieces) {
            int index = ctz(pieces);
            int x = index % 8;
            int y = index / 8;

            auto piece = board->getPieceTypeAt(x, y);

            score += getValueForPiece(piece);
            score += piecePositionScore(x, y, piece, isWhite);

            pieces &= pieces - 1;
        }
        return score;
    };

    whiteScore = evaluatePieces(board->getPiecesBitmap(ChessBoard::WHITE), ChessBoard::WHITE);
    blackScore = evaluatePieces(board->getPiecesBitmap(ChessBoard::BLACK), ChessBoard::BLACK);

    float score = whiteScore - blackScore;
    return searchRootIsWhite ? score : -score;
}

int AI::piecePositionScore(int x, int y, const PieceType type, bool isWhite) const {
    int i = isWhite ? 0 : 1;
    switch (type) {
        case PAWN:
            return PAWN_TABLE[i][y * 8 + x];
        case KNIGHT:
            return KNIGHT_TABLE[i][y * 8 + x];
        case BISHOP:
            return BISHOP_TABLE[i][y * 8 + x];
        case ROOK:
            return ROOK_TABLE[i][y * 8 + x];
        case QUEEN:
            return QUEEN_TABLE[i][y * 8 + x];
        case KING:
            return KING_TABLE[i][y * 8 + x];
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