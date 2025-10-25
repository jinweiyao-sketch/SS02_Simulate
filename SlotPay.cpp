#include "SlotPay.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <stdexcept>

SlotBase:: SlotBase(const GameConfig& config)
    : config_(config), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}

Board SlotBase::eliminate_matches(const Board& board, const MatchPatterns& patterns) {
    Board result = board;
    for (const auto& [symbol, positions] : patterns) {
        for (const auto& [row, col] : positions) {
            result[row][col] = -1;
        }
    }
    return result;
}


float SlotBase::get_score(const MatchPatterns& patterns) {
    float total_score = 0.0f;
    
    for (const auto& [symbol, positions] : patterns) {
        if (!positions.empty()) {
            int count = static_cast<int>(positions.size());
            
            if (config_.pay_table.count(symbol) && config_.pay_table.at(symbol).count(count)) {
                total_score += config_.pay_table.at(symbol).at(count);
            } else {
                total_score += symbol * count;
            }
        }
    }    
    return total_score;
}

bool SlotBase::is_terminal(const Board& board) {
    auto [_, has_match] = find_matches(board);
    return !has_match;
}

void SlotBase::printBoard(const Board& board) {
    for (const auto& row : board) {
        std::cout << "  ";
        for (int symbol : row) {
            std::cout << symbol << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

