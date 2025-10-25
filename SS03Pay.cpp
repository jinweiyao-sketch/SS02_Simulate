#include "SS03Pay.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>
#include <array>

SlotSS03::SlotSS03(bool cascade, float game_cost, std::string game_type)
    : SlotBase(GameConfig{
        .board_height = 5,
        .board_width = 5,
        .symbols = {0, 1, 2, 3, 4, 5, 6, 7, 8},
        .min_match_size = 3,  // SS03 requires 3+ symbols
        .cascade = cascade,
        .game_cost = game_cost,
        .game_type = game_type,
        .pay_table = {}
    }) {
    init_ss03_pay_table();
}

void SlotSS03::init_ss03_pay_table() {
    // Initialize the SS03 pay table with new values
    config_.pay_table = {
        {1, {{3, 10.0f}, {4, 25.0f}, {5, 50.0f}}},
        {2, {{3, 8.0f}, {4, 20.0f}, {5, 40.0f}}},
        {3, {{3, 6.0f}, {4, 15.0f}, {5, 30.0f}}},
        {4, {{3, 5.0f}, {4, 10.0f}, {5, 15.0f}}},
        {5, {{3, 3.0f}, {4, 5.0f}, {5, 12.0f}}},
        {6, {{3, 3.0f}, {4, 5.0f}, {5, 12.0f}}},
        {7, {{3, 2.0f}, {4, 4.0f}, {5, 10.0f}}},
        {8, {{3, 1.0f}, {4, 3.0f}, {5, 6.0f}}},
        {9, {{3, 1.0f}, {4, 3.0f}, {5, 6.0f}}}
    };
}

// Helper method to check if a cell is a padding cell
bool SlotSS03::is_padding_cell(int row, int col) const {
    if (col < -1 || col >= config_.board_width) return true;
    return row >= COLUMN_HEIGHTS[col];
}

// Initialize board with padding cells
void SlotSS03::init_board_padding(Board& board) const {
    for (int col = 0; col < config_.board_width; ++col) {
        for (int row = COLUMN_HEIGHTS[col]; row < config_.board_height; ++row) {
            board[row][col] = PADDING_CELL;
        }
    }
}

// Create a properly padded board
Board SlotSS03::create_padded_board() const {
    Board board(config_.board_height, std::vector<int>(config_.board_width, -1));
    init_board_padding(board);
    return board;
}

// Validate that padding cells are only in correct positions
bool SlotSS03::is_valid_padding(const Board& board) const {
    // Check that board has correct dimensions
    if (board.size() != static_cast<size_t>(config_.board_height) || 
        board[0].size() != static_cast<size_t>(config_.board_width)) {
        return false;
    }
    
    // Check each position on the board
    for (int row = 0; row < config_.board_height; ++row) {
        for (int col = 0; col < config_.board_width; ++col) {
            bool should_be_padding = is_padding_cell(row, col);
            bool is_padding = (board[row][col] == PADDING_CELL);
            
            // Padding cells should only be where expected, and vice versa
            if (should_be_padding != is_padding) {
                return false;
            }
        }
    }
    
    return true;
}

bool SlotSS03::is_valid_board(const Board& board, bool throw_on_error) const {
    // 1. Validate board dimensions
    if (static_cast<int>(board.size()) != config_.board_height) {
        if (throw_on_error) {
            throw BoardValidationError("Invalid board: wrong height. Expected " + 
                                      std::to_string(config_.board_height) + 
                                      ", got " + std::to_string(board.size()));
        }
        return false;
    }
    for (size_t i = 0; i < board.size(); ++i) {
        if (static_cast<int>(board[i].size()) != config_.board_width) {
            if (throw_on_error) {
                throw BoardValidationError("Invalid board: wrong width at row " + 
                                          std::to_string(i) + ". Expected " + 
                                          std::to_string(config_.board_width) + 
                                          ", got " + std::to_string(board[i].size()));
            }
            return false;
        }
    }
    
    // 2. Validate padding cells
    if (!is_valid_padding(board)) {
        if (throw_on_error) {
            throw BoardValidationError("Invalid board: incorrect padding");
        }
        return false;
    }
    
    // 3. Validate cells and count golden tiles per column
    std::array<int, 5> golden_tile_counts = {0, 0, 0, 0, 0};
    bool is_free_game = (config_.game_type == "free");
    std::vector<std::pair<int, int>> non_golden_in_col2;  // Track non-golden tiles in column 2 for free game
    
    for (int row = 0; row < config_.board_height; ++row) {
        for (int col = 0; col < config_.board_width; ++col) {
            if (is_padding_cell(row, col)) continue;
            
            int cell_value = board[row][col];
            bool is_golden = (cell_value >= 100 && cell_value < 200);
            bool is_first_or_last_row = (row == 0 || row == config_.board_height - 1);
            
            // Allow empty cells and scatter
            if (cell_value == -1 || cell_value == SCATTER) continue;
            
            // Count golden tiles
            if (is_golden) {
                golden_tile_counts[col]++;
            }
            
            // Free game: column 2 must have ALL golden tiles - collect violations
            if (is_free_game && col == 2 && !is_golden) {
                non_golden_in_col2.emplace_back(row, col);
            }
            
            // Golden tiles not allowed in first/last rows (except column 2 in free game)
            if (is_golden && is_first_or_last_row && !(is_free_game && col == 2)) {
                if (throw_on_error) {
                    throw BoardValidationError("Invalid board: gold tile appears in first or last row at (" + 
                                              std::to_string(row) + "," + std::to_string(col) + ")");
                }
                return false;
            }
            
            // Validate symbol value
            if (cell_value == WILD || cell_value == SCATTER) continue;
            
            bool is_valid_symbol = false;
            int base_symbol = is_golden ? cell_value - 100 : cell_value;
            for (int symbol : config_.symbols) {
                if (base_symbol == symbol) {
                    is_valid_symbol = true;
                    break;
                }
            }
            
            if (!is_valid_symbol) {
                if (throw_on_error) {
                    throw BoardValidationError("Invalid board: invalid symbol " + 
                                              std::to_string(cell_value) + 
                                              " at position (" + std::to_string(row) + 
                                              "," + std::to_string(col) + ")");
                }
                return false;
            }
        }
    }
    
    // Check for non-golden tiles in column 2 for free game
    if (!non_golden_in_col2.empty()) {
        if (throw_on_error) {
            std::string positions;
            for (size_t i = 0; i < non_golden_in_col2.size(); ++i) {
                positions += "(" + std::to_string(non_golden_in_col2[i].first) + "," + 
                            std::to_string(non_golden_in_col2[i].second) + ")";
                if (i < non_golden_in_col2.size() - 1) positions += ", ";
            }
            throw BoardValidationError("Invalid board: In free game, non-gold tile(s) in 3rd column at " + positions);
        }
        return false;
    }
    
    // 4. Validate golden tile counts per column (max 2, except column 2 in free game)
    for (int col = 0; col < config_.board_width; ++col) {
        if (is_free_game && col == 2) continue;  // Column 2 in free game already validated
        
        if (golden_tile_counts[col] > 2) {
            if (throw_on_error) {
                throw BoardValidationError("Invalid board: More than 2 golden tiles (" + 
                                          std::to_string(golden_tile_counts[col]) + 
                                          ") in column " + std::to_string(col));
            }
            return false;
        }
    }
    return true;
}


// Ways to Win: Dynamic Programming approach
//they should treat 105 and 5 the same.

std::pair<MatchPatterns, bool> SlotSS03::find_matches(const Board& board) {
    MatchPatterns match_patterns;
    bool has_match = false;

    // DP table: dp[symbol] = positions for valid winning lines
    std::unordered_map<int, MatchPattern> dp;
    
    // Initialize column 0: collect all symbols in first column
    for (int row = 0; row < COLUMN_HEIGHTS[0]; ++row) {
        int symbol = board[row][0];
        if (symbol > 0 && symbol < 100) {  // Valid symbol (not empty, padding, or special values >= 100)
            dp[symbol].emplace_back(row, 0);
        }
        else if (symbol >= 100 && symbol < 200)  // treat gold tile symbol as normal symbol 
        {
            dp[symbol-100].emplace_back(row,0);
        }    
    }
    
    // Process columns 1-4
    for (int col = 1; col < config_.board_width; ++col) {
        std::unordered_map<int, MatchPattern> next_dp;
        
        if (col <= 2) {
            // Columns 1-2: symbol must exist in previous column to continue
            for (const auto& [symbol, prev_positions] : dp) {
                // Check if symbol or WILD exists in current column and collect ALL occurrences
                std::vector<std::pair<int, int>> current_positions;
                for (int row = 0; row < COLUMN_HEIGHTS[col]; ++row) {
                    int current_symbol = board[row][col];
                    // Match if exact symbol or WILD (and col >= 1 for WILD restriction)
                    if (current_symbol == symbol || (current_symbol - 100 == symbol) || (current_symbol == WILD && col >= 1)) {
                        current_positions.emplace_back(row, col);
                    }
                }
                // If symbol found in current column, add ALL occurrences
                if (!current_positions.empty()) {
                    next_dp[symbol] = prev_positions;
                    for (const auto& pos : current_positions) {
                        next_dp[symbol].emplace_back(pos);
                    }
                }
            }
        } else {
            // Columns 3-4: extend existing chains if symbol exists, preserve 3+ column matches even if they die
            for (const auto& [symbol, prev_positions] : dp) {
                // If chain already has 3+ positions, preserve it as a valid match
                if (static_cast<int>(prev_positions.size()) >= config_.min_match_size) {
                    match_patterns[symbol] = prev_positions;
                    has_match = true;
                }
                
                // Try to extend the chain by collecting ALL occurrences
                std::vector<std::pair<int, int>> current_positions;
                for (int row = 0; row < COLUMN_HEIGHTS[col]; ++row) {
                    int current_symbol = board[row][col];
                    // Match if exact symbol or WILD
                    if (current_symbol == symbol || (current_symbol - 100 == symbol) || current_symbol == WILD) {
                        current_positions.emplace_back(row, col);
                    }
                }
                // If symbol found in current column, add ALL occurrences
                if (!current_positions.empty()) {
                    next_dp[symbol] = prev_positions;
                    for (const auto& pos : current_positions) {
                        next_dp[symbol].emplace_back(pos);
                    }
                }
                // If symbol not found in this column, chain dies but may have been preserved above
            }
        }
        
        dp = std::move(next_dp); //change ownership to avoid copies
    }
    
    // Collect any remaining valid matches (including those that survived to the end)
    for (const auto& [symbol, positions] : dp) {
        if (static_cast<int>(positions.size()) >= config_.min_match_size) {
            match_patterns[symbol] = positions;  // This will overwrite shorter versions if longer exists
            has_match = true;
        }
    }
    return {match_patterns, has_match};
}

// Override eliminate_matches for SS03: golden tiles become WILD, regular symbols become -1
Board SlotSS03::eliminate_matches(const Board& board, const MatchPatterns& patterns) {
    Board result = board;
    
    for (const auto& [symbol, positions] : patterns) {
        for (const auto& [row, col] : positions) {
            int original_value = board[row][col];
            
            // Check if it's a golden tile (100-200)
            if (original_value >= 100 && original_value < 200) {
                // Golden tile matched -> becomes WILD
                result[row][col] = WILD;
            } else {
                // Regular symbol matched -> becomes empty
                result[row][col] = -1;
            }
        }
    }
    
    return result;
}

// Override gravity to respect column heights
Board SlotSS03::apply_gravity(const Board& board) {
    Board result = board;
    
    for (int col = 0; col < config_.board_width; ++col) {
        int column_height = COLUMN_HEIGHTS[col];
        
        // Collect all non-empty values in this column (excluding padding)
        std::vector<int> non_empty;
        for (int row = 0; row < column_height; ++row) {
            if (result[row][col] != -1 && result[row][col] != PADDING_CELL) {
                non_empty.push_back(result[row][col]);
            }
        }
        
        // Clear the playable area of this column
        for (int row = 0; row < column_height; ++row) {
            result[row][col] = -1;
        }
        
        // Place blocks at bottom of this column
        int start_row = column_height - static_cast<int>(non_empty.size());
        for (size_t i = 0; i < non_empty.size(); ++i) {
            result[start_row + static_cast<int>(i)][col] = non_empty[i];
        }
        
        // Ensure padding cells remain as padding
        for (int row = column_height; row < config_.board_height; ++row) {
            result[row][col] = PADDING_CELL;
        }
    }
    
    return result;
}

Board SlotSS03::refill(const Board& current_board, int current_stop, const std::vector<Board>& script) {
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

// Override scoring to calculate "ways to win"
float SlotSS03::get_score(const MatchPatterns& patterns) {
    float total_score = 0.0f;

    for (const auto& [symbol, positions] : patterns) {
        if (positions.empty()) continue;
        
        // 1) Create list: count of symbol in each column [col0, col1, col2, col3, col4]
        std::array<int, 5> column_counts = {0, 0, 0, 0, 0};
        
        for (const auto& [row, col] : positions) {
            column_counts[col]++;
        }
        
        // 2) Find consecutive columns starting from column 0
        int consecutive_columns = 0;
        for (int col = 0; col < 5; ++col) {
            if (column_counts[col] > 0) {
                consecutive_columns++;  // Increment count of consecutive columns
            } else {
                break;  // Stop at first gap
            }
        }
        
        // Must be at least 3 consecutive columns for a win
        if (consecutive_columns < 3) continue;
        
        // 3) Ways = product of counts in each column up to stopping column
        int ways = 1;
        for (int col = 0; col < consecutive_columns; ++col) {
            ways *= column_counts[col];
        }
        
        // Get payout from table (use consecutive_columns as "kind")
        float payout = 0.0f;
        if (config_.pay_table.count(symbol) && config_.pay_table[symbol].count(consecutive_columns)) {
            payout = config_.pay_table[symbol][consecutive_columns];
        }
        
        // Symbol score = payout * ways
        total_score += payout * ways;
    }

    // Return base score without multiplier (multiplier handled externally)
    return total_score;
}


// Override step method without combo tracking (handled externally)
std::pair<Board, float> SlotSS03::step(const Board& board) {
    // 1. FIND MATCHES
    auto [match_patterns, has_match] = find_matches(board);
    
    // 2. NO MATCHES CASE
    if (!has_match) {
        return {board, 0.0f};
    }
    
    // 3. PROCESS MATCHES - get base score (no multiplier applied)
    float board_score = get_score(match_patterns);
    
    // 4. ELIMINATE MATCHES (uses SS03's override: golden tiles->WILD, regular->-1)
    Board result_board = eliminate_matches(board, match_patterns);

    // 5. APPLY GRAVITY (if cascade enabled)
    if (config_.cascade) {
        result_board = apply_gravity(result_board);
    }

    return {result_board, board_score};
}

// Steps method that completes the script
std::tuple<Board, float, int, std::vector<MatchPatterns>, bool> SlotSS03::steps(const std::vector<Board>& script) {
    if (script.empty()) {
        return {Board{}, 0.0f, 0, std::vector<MatchPatterns>{}, true};
    }
    
    Board current_board = script[0];  // Use the first board from the script

    float total_score = 0.0f;
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
    
        // Eliminate matches (uses SS03's override: golden tiles->WILD, regular->-1)
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
    
    return {current_board, total_score, actual_stop+1, all_patterns, all_cascade_match};
}

// Test function for SS03 Oracle
#ifdef SS03_TEST_MAIN
int main() {
    std::cout << "=== TESTING SS03Pay Class ===" << std::endl;

    try {
        // Test initialization
        SlotSS03 ss03(true, 20.0f);
        std::cout << "Board size: " << ss03.get_board_height() << "x" << ss03.get_board_width() << std::endl;
        std::cout << "Symbols: ";
        for (int symbol : ss03.get_symbols()) {
            std::cout << symbol << " ";
        }
        std::cout << std::endl;
        std::cout << "Min match size: " << ss03.get_min_match_size() << std::endl;
        std::cout << "Cascade: " << std::boolalpha << ss03.get_config().cascade << std::endl;
        std::cout << "Game cost: " << ss03.get_config().game_cost << std::endl;
        std::cout << std::endl;

        // Test create padded board
        std::cout << "Testing create_padded_board():" << std::endl;
        Board padded_board = ss03.create_padded_board();
        SlotBase::printBoard(padded_board);
        std::cout << "Is valid padding: " << std::boolalpha << ss03.is_valid_padding(padded_board) << std::endl;
        std::cout << std::endl;
        
        // Test with no matches (symbols 1-8 only)
        Board board1 = {
            {1, 2, 3, 4, 5},
            {6, 107, 8, 1, 2},
            {2, 3, 108, 5, 3},
            {7, 8, 1, 2, 4},
            {-2, 5, 6, 7, -2}
        };

        std::cout << "Testing board with no matches:" << std::endl;
        SlotBase::printBoard(board1);
        std::cout << "Is valid board: " << std::boolalpha << ss03.is_valid_board(board1) << std::endl;
        std::cout << "Is valid padding: " << std::boolalpha << ss03.is_valid_padding(board1) << std::endl;
        auto [result1, score1] = ss03.step(board1);
        std::cout << "Score: " << score1 << " (should be 0)" << std::endl;
        std::cout << "Is terminal: " << ss03.is_terminal(result1) << std::endl;
        std::cout << std::endl;

        // Test board validation
        std::cout << "=== Testing is_valid_board() ===" << std::endl;
        
        // Valid padding
        std::cout << "\n1. Valid board with correct padding:" << std::endl;
        SlotBase::printBoard(board1);
        std::cout << "Is valid padding: " << std::boolalpha << ss03.is_valid_padding(board1) << std::endl;
        try {
            bool is_valid = ss03.is_valid_board(board1, true);
            std::cout << "Is valid board: " << std::boolalpha << is_valid << std::endl;
        } catch (const BoardValidationError& e) {
            std::cout << "Is valid board: false" << std::endl;
            std::cout << "  Error: " << e.what() << std::endl;
        }
        
        // Missing padding
        std::cout << "\n2. Invalid board - missing padding:" << std::endl;
        Board missing_padding = {
            {1, 2, 3, 4, 5},
            {6, 7, 8, 1, 2},
            {2, 3, 4, 5, 3},
            {7, 8, 1, 2, 4},
            {1, 2, 3, 4, 5}  // Should have -2 in positions (4,0) and (4,4)
        };
        SlotBase::printBoard(missing_padding);
        try {
            bool is_valid = ss03.is_valid_board(missing_padding, true);
            std::cout << "Is valid board: " << std::boolalpha << is_valid << std::endl;
        } catch (const BoardValidationError& e) {
            std::cout << "Is valid board: false" << std::endl;
            std::cout << "  Error: " << e.what() << std::endl;
        }

        // Invalid symbol
        std::cout << "\n3. Invalid board - invalid symbol (99):" << std::endl;
        Board invalid_symbol = {
            {1, 2, 3, 4, 5},
            {6, 7, 8, 1, 2},
            {2, 3, 99, 5, 3},  // 99 is not a valid symbol
            {7, 8, 1, 2, 4},
            {-2, 1, 2, 3, -2}
        };
        SlotBase::printBoard(invalid_symbol);
        try {
            bool is_valid = ss03.is_valid_board(invalid_symbol, true);
            std::cout << "Is valid board: " << std::boolalpha << is_valid << std::endl;
        } catch (const BoardValidationError& e) {
            std::cout << "Is valid board: false" << std::endl;
            std::cout << "  Error: " << e.what() << std::endl;
        }

        // Gold tile in first row (invalid)
        std::cout << "\n4. Invalid board - gold tile in first row:" << std::endl;
        Board gold_first_row = {
            {105, 2, 3, 4, 5},  // Golden tile in first row - invalid
            {6, 7, 8, 1, 2},
            {2, 3, 4, 5, 3},
            {7, 8, 1, 2, 4},
            {-2, 1, 2, 3, -2}
        };
        SlotBase::printBoard(gold_first_row);
        try {
            bool is_valid = ss03.is_valid_board(gold_first_row, true);
            std::cout << "Is valid board: " << std::boolalpha << is_valid << std::endl;
        } catch (const BoardValidationError& e) {
            std::cout << "Is valid board: false" << std::endl;
            std::cout << "  Error: " << e.what() << std::endl;
        }

        // Test free game with all golden tiles in 3rd column (valid)
        std::cout << "\n5. Free game - all golden tiles in 3rd column (valid):" << std::endl;
        SlotSS03 ss03_free(true, 20.0f, "free");  // Create free game instance
        Board free_game_board = {
            {1, 2, 105, 4, 5},  // First row - golden tile in column 2
            {6, 7, 108, 1, 2},  // Golden tile in column 2
            {2, 3, 104, 5, 3},  // Golden tile in column 2
            {7, 8, 101, 2, 4},  // Golden tile in column 2
            {-2, 5, 106, 7, -2}  // Last row - golden tile 106 in column 2
        };
        SlotBase::printBoard(free_game_board);
        std::cout << "Game type: " << ss03_free.get_config().game_type << std::endl;
        try {
            bool is_valid = ss03_free.is_valid_board(free_game_board, true);
            std::cout << "Is valid board: " << std::boolalpha << is_valid << std::endl;
        } catch (const BoardValidationError& e) {
            std::cout << "Is valid board: false" << std::endl;
            std::cout << "  Error: " << e.what() << std::endl;
        }

        // Test free game with non-golden tile in 3rd column (invalid)
        std::cout << "\n6. Free game - non-golden tile in 3rd column (invalid):" << std::endl;
        Board free_game_invalid = {
            {1, 2, 5, 4, 5},  // First row - regular tile in column 2
            {6, 7, 108, 1, 2},  // Golden tile in column 2
            {2, 3, 5, 5, 3},  // Regular tile 5 in column 2 row 2 - should be invalid for free game
            {7, 8, 101, 2, 4},
            {-2, 5, 5, 7, -2}
        };
        SlotBase::printBoard(free_game_invalid);
        std::cout << "Game type: " << ss03_free.get_config().game_type << std::endl;
        try {
            bool is_valid = ss03_free.is_valid_board(free_game_invalid, true);
            std::cout << "Is valid board: " << std::boolalpha << is_valid << std::endl;
        } catch (const BoardValidationError& e) {
            std::cout << "Is valid board: false" << std::endl;
            std::cout << "  Error: " << e.what() << std::endl;
        }

        // Test base game with 3 golden tiles in 2nd column (invalid - too many)
        std::cout << "\n7. Base game - 3 golden tiles in 2nd column (invalid - max 2):" << std::endl;
        Board base_game_gold = {
            {1, 2, 5, 4, 5},  // First row - no golden tiles
            {6, 107, 8, 1, 2},  // Golden tile 107 in column 1 (2nd column)
            {2, 103, 4, 5, 3},  // Golden tile 103 in column 1
            {7, 102, 1, 2, 4},  // Golden tile 102 in column 1
            {-2, 5, 6, 7, -2}  // Last row - column 1 has regular 5
        };
        SlotBase::printBoard(base_game_gold);
        std::cout << "Game type: " << ss03.get_config().game_type << std::endl;
        try {
            bool is_valid = ss03.is_valid_board(base_game_gold, true);
            std::cout << "Is valid board: " << std::boolalpha << is_valid << std::endl;
        } catch (const BoardValidationError& e) {
            std::cout << "Is valid board: false" << std::endl;
            std::cout << "  Error: " << e.what() << std::endl;
        }

        // Test with ways-to-win matches (3 consecutive columns)
        Board board2 = {
            {1, 1, 1, 2, 3},
            {2, 1, 1, 4, 6},
            {3, 2, 1, 5, 7},
            {4, 3, 2, 6, 8},
            {-2, 5, 4, 7, -2}
        };

        std::cout << "Testing board with 3-column ways-to-win match:" << std::endl;
        SlotBase::printBoard(board2);
        std::cout << "Is valid board: " << std::boolalpha << ss03.is_valid_board(board2) << std::endl;
        
        auto [patterns2, has_match2] = ss03.find_matches(board2);
        std::cout << "Has match: " << has_match2 << std::endl;
        if (has_match2) {
            std::cout << "Match patterns found:" << std::endl;
            for (const auto& [symbol, positions] : patterns2) {
                std::cout << "Symbol " << symbol << ": ";
                for (const auto& [row, col] : positions) {
                    std::cout << "(" << row << "," << col << ") ";
                }
                std::cout << std::endl;
            }
        }
        
        auto [result2, score2] = ss03.step(board2);
        std::cout << "Score: " << score2 << std::endl;
        std::cout << "Result board after step:" << std::endl;
        SlotBase::printBoard(result2);
        std::cout << std::endl;

        // Test with golden tiles (symbol >= 100, < 200)
        Board board3 = {
            {5, 2, 5, 2, 3},
            {2, 105, 5, 104, 6},
            {3, 2, 105, 6, 7},
            {4, 3, 2, 7, 8},
            {-2, 6, 3, 8, -2}
        };

        std::cout << "Testing board with golden tiles (105 = golden 5):" << std::endl;
        SlotBase::printBoard(board3);
        std::cout << "Is valid board: " << std::boolalpha << ss03.is_valid_board(board3) << std::endl;
        
        auto [patterns3, has_match3] = ss03.find_matches(board3);
        std::cout << "Has match: " << has_match3 << std::endl;
        if (has_match3) {
            std::cout << "Match patterns found:" << std::endl;
            for (const auto& [symbol, positions] : patterns3) {
                std::cout << "Symbol " << symbol << ": ";
                for (const auto& [row, col] : positions) {
                    std::cout << "(" << row << "," << col << ") ";
                }
                std::cout << std::endl;
            }
        }
        
        auto [result3, score3] = ss03.step(board3);
        std::cout << "Score: " << score3 << std::endl;
        std::cout << "Result board after step (golden tiles should become WILD=202):" << std::endl;
        SlotBase::printBoard(result3);
        std::cout << std::endl;


        // Test gravity
        Board board4 = {
            {-1, 1, -1, 2, -1},
            {2, -1, 3, -1, 5},
            {-1, 3, -1, 4, 6},
            {4, 2, 5, 1, 7},
            {-2, 6, 7, 8, -2}
        };

        std::cout << "Testing gravity with holes:" << std::endl;
        SlotBase::printBoard(board4);
        
        Board gravity_result = ss03.apply_gravity(board4);
        std::cout << "After gravity:" << std::endl;
        SlotBase::printBoard(gravity_result);
        std::cout << std::endl;

        // Test eliminate_matches with golden tiles
        std::cout << "=== Testing eliminate_matches() ===" << std::endl;

        std::cout << "\n1. Testing eliminate_matches with mixed golden and regular symbols:" << std::endl;
        Board test_elim_board1 = {
            {4, 104, 4, 2, 3},
            {4, 4, 104, 4, 6},
            {3, 2, 4, 5, 7},
            {4, 3, 2, 6, 8},
            {-2, 6, 3, 7, -2}
        };
        std::cout << "Original board (104 = golden 4, mixed with regular 4s):" << std::endl;
        SlotBase::printBoard(test_elim_board1);
        
        auto [test_patterns1, test_match1] = ss03.find_matches(test_elim_board1);
        std::cout << "Patterns found:" << std::endl;
        for (const auto& [symbol, positions] : test_patterns1) {
            std::cout << "  Symbol " << symbol << ": ";
            for (const auto& [row, col] : positions) {
                std::cout << "(" << row << "," << col << ") ";
            }
            std::cout << std::endl;
        }
        
        Board test_elim_result1 = ss03.eliminate_matches(test_elim_board1, test_patterns1);
        std::cout << "After eliminate_matches:" << std::endl;
        SlotBase::printBoard(test_elim_result1);
        std::cout << std::endl;
        
        // Test apply_gravity with WILD symbols
        std::cout << "=== Testing apply_gravity() with WILD symbols ===" << std::endl;
        
        std::cout << "\n1. Testing gravity with mixed -1 and WILD:" << std::endl;
        Board test_gravity1 = {
            {-1, 1, -1, 2, -1},
            {202, -1, 3, -1, 5},      // WILD at (1,0)
            {-1, 3, 202, 4, 6},       // WILD at (2,2)
            {4, 2, -1, 1, 7},
            {-2, 6, 7, 8, -2}
        };
        std::cout << "Before gravity (202 = WILD):" << std::endl;
        SlotBase::printBoard(test_gravity1);
        
        Board test_gravity_result1 = ss03.apply_gravity(test_gravity1);
        std::cout << "After gravity (WILD should fall like regular symbols):" << std::endl;
        SlotBase::printBoard(test_gravity_result1);
        std::cout << std::endl;
        
        std::cout << "\n2. Testing full cascade: eliminate → gravity:" << std::endl;
        Board test_cascade = {
            {5, 105, 5, 2, 3},
            {2, 5, 105, 4, 6},
            {3, 2, 5, 5, 7},
            {4, 3, 2, 6, 8},
            {-2, 6, 3, 7, -2}
        };
        std::cout << "Original board:" << std::endl;
        SlotBase::printBoard(test_cascade);
        
        auto [cascade_patterns, cascade_match] = ss03.find_matches(test_cascade);
        std::cout << "Patterns found:" << std::endl;
        for (const auto& [symbol, positions] : cascade_patterns) {
            std::cout << "  Symbol " << symbol << ": ";
            for (const auto& [row, col] : positions) {
                std::cout << "(" << row << "," << col << ") ";
            }
            std::cout << std::endl;
        }
        
        Board after_eliminate = ss03.eliminate_matches(test_cascade, cascade_patterns);
        std::cout << "After eliminate_matches:" << std::endl;
        SlotBase::printBoard(after_eliminate);
        
        Board after_gravity = ss03.apply_gravity(after_eliminate);
        std::cout << "After apply_gravity:" << std::endl;
        SlotBase::printBoard(after_gravity);
        std::cout << std::endl;

        std::cout << "\n✅ All SS03Pay tests completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
#endif