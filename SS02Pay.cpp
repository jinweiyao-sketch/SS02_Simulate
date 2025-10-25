#include "SS02Pay.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <stdexcept>

SlotSS02::SlotSS02(bool cascade, float game_cost, std::string game_type)
    : SlotBase(GameConfig{
        .board_height = 5,
        .board_width = 6,
        .symbols = {0, 1, 2, 3, 4, 5, 6, 7, 8},
        .min_match_size = 8,
        .cascade = cascade,
        .game_cost = game_cost,
        .game_type = game_type,
        .pay_table = {}
    }), 
      fg_trigger_probability_(0.005),      // Initialize SS02 free game trigger probability (0.4%)
      fg_retrigger_probability_(0.03),     // Initialize SS02 free game retrigger probability (4%)
      volatility_type_("low")             // Initialize volatility type to high
{
    init_ss02_pay_table();
}

void SlotSS02::init_ss02_pay_table() {
    // Initialize the complete SS02 pay table as specified in the reference
    config_.pay_table = {
        {0, {{8, 200.0f}, {9, 200.0f}, {10, 500.0f}, {11, 500.0f}, {12, 1000.0f}, {13, 1000.0f}, {14, 1000.0f}, {15, 1000.0f}, {16, 1000.0f}, {17, 1000.0f}, {18, 1000.0f}, {19, 1000.0f}, {20, 1000.0f}, {21, 1000.0f}, {22, 1000.0f}, {23, 1000.0f}, {24, 1000.0f}, {25, 1000.0f}, {26, 1000.0f}, {27, 1000.0f}, {28, 1000.0f}, {29, 1000.0f}, {30, 1000.0f}}},
        {1, {{8, 50.0f}, {9, 50.0f}, {10, 200.0f}, {11, 200.0f}, {12, 500.0f}, {13, 500.0f}, {14, 500.0f}, {15, 500.0f}, {16, 500.0f}, {17, 500.0f}, {18, 500.0f}, {19, 500.0f}, {20, 500.0f}, {21, 500.0f}, {22, 500.0f}, {23, 500.0f}, {24, 500.0f}, {25, 500.0f}, {26, 500.0f}, {27, 500.0f}, {28, 500.0f}, {29, 500.0f}, {30, 500.0f}}},
        {2, {{8, 40.0f}, {9, 40.0f}, {10, 100.0f}, {11, 100.0f}, {12, 300.0f}, {13, 300.0f}, {14, 300.0f}, {15, 300.0f}, {16, 300.0f}, {17, 300.0f}, {18, 300.0f}, {19, 300.0f}, {20, 300.0f}, {21, 300.0f}, {22, 300.0f}, {23, 300.0f}, {24, 300.0f}, {25, 300.0f}, {26, 300.0f}, {27, 300.0f}, {28, 300.0f}, {29, 300.0f}, {30, 300.0f}}},
        {3, {{8, 30.0f}, {9, 30.0f}, {10, 40.0f}, {11, 40.0f}, {12, 240.0f}, {13, 240.0f}, {14, 240.0f}, {15, 240.0f}, {16, 240.0f}, {17, 240.0f}, {18, 240.0f}, {19, 240.0f}, {20, 240.0f}, {21, 240.0f}, {22, 240.0f}, {23, 240.0f}, {24, 240.0f}, {25, 240.0f}, {26, 240.0f}, {27, 240.0f}, {28, 240.0f}, {29, 240.0f}, {30, 240.0f}}},
        {4, {{8, 20.0f}, {9, 20.0f}, {10, 30.0f}, {11, 30.0f}, {12, 200.0f}, {13, 200.0f}, {14, 200.0f}, {15, 200.0f}, {16, 200.0f}, {17, 200.0f}, {18, 200.0f}, {19, 200.0f}, {20, 200.0f}, {21, 200.0f}, {22, 200.0f}, {23, 200.0f}, {24, 200.0f}, {25, 200.0f}, {26, 200.0f}, {27, 200.0f}, {28, 200.0f}, {29, 200.0f}, {30, 200.0f}}},
        {5, {{8, 16.0f}, {9, 16.0f}, {10, 24.0f}, {11, 24.0f}, {12, 160.0f}, {13, 160.0f}, {14, 160.0f}, {15, 160.0f}, {16, 160.0f}, {17, 160.0f}, {18, 160.0f}, {19, 160.0f}, {20, 160.0f}, {21, 160.0f}, {22, 160.0f}, {23, 160.0f}, {24, 160.0f}, {25, 160.0f}, {26, 160.0f}, {27, 160.0f}, {28, 160.0f}, {29, 160.0f}, {30, 160.0f}}},
        {6, {{8, 10.0f}, {9, 10.0f}, {10, 20.0f}, {11, 20.0f}, {12, 100.0f}, {13, 100.0f}, {14, 100.0f}, {15, 100.0f}, {16, 100.0f}, {17, 100.0f}, {18, 100.0f}, {19, 100.0f}, {20, 100.0f}, {21, 100.0f}, {22, 100.0f}, {23, 100.0f}, {24, 100.0f}, {25, 100.0f}, {26, 100.0f}, {27, 100.0f}, {28, 100.0f}, {29, 100.0f}, {30, 100.0f}}},
        {7, {{8, 8.0f}, {9, 8.0f}, {10, 18.0f}, {11, 18.0f}, {12, 80.0f}, {13, 80.0f}, {14, 80.0f}, {15, 80.0f}, {16, 80.0f}, {17, 80.0f}, {18, 80.0f}, {19, 80.0f}, {20, 80.0f}, {21, 80.0f}, {22, 80.0f}, {23, 80.0f}, {24, 80.0f}, {25, 80.0f}, {26, 80.0f}, {27, 80.0f}, {28, 80.0f}, {29, 80.0f}, {30, 80.0f}}},
        {8, {{8, 5.0f}, {9, 5.0f}, {10, 15.0f}, {11, 15.0f}, {12, 40.0f}, {13, 40.0f}, {14, 40.0f}, {15, 40.0f}, {16, 40.0f}, {17, 40.0f}, {18, 40.0f}, {19, 40.0f}, {20, 40.0f}, {21, 40.0f}, {22, 40.0f}, {23, 40.0f}, {24, 40.0f}, {25, 40.0f}, {26, 40.0f}, {27, 40.0f}, {28, 40.0f}, {29, 40.0f}, {30, 40.0f}}}
    };
}

Board SlotSS02::apply_gravity(const Board& board) {
    Board result = board;
    
    for (int col = 0; col < config_.board_width; ++col) {
        // Collect non-empty values
        std::vector<int> non_empty;
        for (int row = 0; row < config_.board_height; ++row) {
            if (result[row][col] != -1) {
                non_empty.push_back(result[row][col]);
            }
        }
        
        // Clear column and place -1s at top, non-empty values at bottom
        for (int row = 0; row < config_.board_height; ++row) {
            result[row][col] = -1;
        }
        
        // Place non-empty values at the bottom of the column
        int start_row = config_.board_height - static_cast<int>(non_empty.size());
        for (size_t i = 0; i < non_empty.size(); ++i) {
            result[start_row + static_cast<int>(i)][col] = non_empty[i];
        }
    }    
    return result;
}


std::pair<MatchPatterns, bool> SlotSS02::find_matches(const Board& board) {
    MatchPatterns match_patterns;
    bool has_match = false;

    for (int symbol : config_.symbols) {
        std::vector<std::pair<int, int>> positions;

        // Count all positions of this symbol (cluster matching - anywhere on board)
        for (int row = 0; row < config_.board_height; ++row) {
            for (int col = 0; col < config_.board_width; ++col) {
                if (board[row][col] == symbol) {
                    positions.emplace_back(row, col);
                }
            }
        }

        // Check if we have enough for a match (8+ symbols for SS02)
        if (static_cast<int>(positions.size()) >= config_.min_match_size) {
            match_patterns[symbol] = positions;
            has_match = true;
        }
    }

    return {match_patterns, has_match};
}

Board SlotSS02::refill(const Board& current_board, int current_stop, const std::vector<Board>& script) {
    Board result = current_board;
    
    // Use the next board for refilling, but don't increment the stop counter
    int next_stop = current_stop + 1;
    
    const Board& next_board = script[next_stop];

    // Refill positions marked with -1 using values from the next board
    for (int row = 0; row < static_cast<int>(result.size()); ++row) {
        for (int col = 0; col < static_cast<int>(result[row].size()); ++col) {
            if (result[row][col] == -1) {
                // Make sure the next_board has the same dimensions
                if (row < static_cast<int>(next_board.size()) && 
                    col < static_cast<int>(next_board[row].size())) {
                    result[row][col] = next_board[row][col];
                }
            }
        }
    }

    return result; // Return the refilled board
}

std::tuple<Board, float, int, std::vector<MatchPatterns>, bool> SlotSS02::steps(const std::vector<Board>& script, int special_multipliers) {
    if (script.empty()) {
        return {Board{}, 0.0f, 0, std::vector<MatchPatterns>{}, true};
    }
    
    Board current_board = script[0];  // Use the first board from the script

    int total_score = 0;
    int actual_stop = 0;
    bool all_cascade_match = true;  // Track if all boards match throughout processing
    
    // Record all patterns found during processing
    std::vector<MatchPatterns> all_patterns;

    while (!is_terminal(current_board) && actual_stop < static_cast<int>(script.size())-1) { 
        auto [patterns, has_match] = find_matches(current_board);
        // Record the patterns for this step
        all_patterns.push_back(patterns);
        
        float step_score = get_score(patterns);
        total_score += step_score;
    
        current_board = eliminate_matches(current_board, patterns);
        current_board = apply_gravity(current_board);

        Board next_board = script[actual_stop + 1];

        // Check if current_board's non -1 entries match next_board's corresponding entries
        bool step_cascade_match = true;
        for (size_t row = 0; row < current_board.size(); ++row) {
            for (size_t col = 0; col < current_board[row].size(); ++col) {
                if (current_board[row][col] != -1) {
                    if (current_board[row][col] != next_board[row][col]) {
                        step_cascade_match = false;
                    }
                }
            }
        }
        if (!step_cascade_match) {
            all_cascade_match = false;
        }
        current_board = next_board;
        actual_stop++;
    }

    // Apply multiplier to total score (only for free games)
    if (config_.game_type == "free") {
        // Count the number of multiplier symbols in the last board
        int multiplier_count = 0;
        for (int row = 0; row < config_.board_height; ++row) {
            for (int col = 0; col < config_.board_width; ++col) {
                if (current_board[row][col] == MULTIPLIER) {
                    multiplier_count++;
                }
            }
        }
        // Apply multiplier to total score
        if (multiplier_count > 0) {
            total_score *= multiplier_count * special_multipliers;
        }
    }
    
    return {current_board, total_score, actual_stop+1, all_patterns, all_cascade_match};
}

// ============================================================================
// MULTIPLIER TABLE DEFINITIONS
// Edit these tables to customize high and low volatility multiplier distributions
// ============================================================================

// HIGH VOLATILITY MULTIPLIER TABLE
const nlohmann::json SlotSS02::HIGH_VOLATILITY_TABLE = R"({
  "free": [
    {
      "id": 1,
      "multiplier": [
        102,
        103,
        105,
        110,
        120,
        130,
        150,
        200
      ],
      "weight": [
        10,
        5,
        5,
        8,
        0,
        0,
        0,
        0
      ]
    },
    {
      "id": 2,
      "multiplier": [
        102,
        103,
        105,
        110,
        120,
        130,
        150,
        200
      ],
      "weight": [
        0,
        0,
        2,
        0,
        3,
        2,
        9,
        1
      ]
    }
  ]
})"_json;

// LOW VOLATILITY MULTIPLIER TABLE
const nlohmann::json SlotSS02::LOW_VOLATILITY_TABLE = R"({
  "free": [
    {
      "id": 1,
      "multiplier": [
        102,
        103,
        105,
        110,
        120,
        130,
        150,
        200
      ],
      "weight": [
        10,
        5,
        5,
        0,
        0,
        0,
        0,
        0
      ]
    },
    {
      "id": 2,
      "multiplier": [
        102,
        103,
        105,
        110,
        120,
        130,
        150,
        200
      ],
      "weight": [
        1,
        1,
        1,
        0,
        2,
        2,
        2,
        1
      ]
    }
  ]
})"_json;

nlohmann::json SlotSS02::get_multiplier_table(const std::string& volatility_type) {
    if (volatility_type == "high") {
        return HIGH_VOLATILITY_TABLE;
    } else if (volatility_type == "low") {
        return LOW_VOLATILITY_TABLE;
    }
    // Default to high volatility
    return HIGH_VOLATILITY_TABLE;
}