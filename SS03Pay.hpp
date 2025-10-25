#pragma once
#include "SlotPay.hpp"
#include <array>
#include <stdexcept>

// Custom exception for board validation errors
class BoardValidationError : public std::runtime_error {
public:
    explicit BoardValidationError(const std::string& message) 
        : std::runtime_error(message) {}
};

class SlotSS03 : public SlotBase {
private:
    void init_ss03_pay_table();
    
    // Column heights for irregular board: {4,5,5,5,4}
    static constexpr std::array<int, 5> COLUMN_HEIGHTS = {4, 5, 5, 5, 4};
    static constexpr int PADDING_CELL = -2;  // Special value for padding cells
    static constexpr int WILD = 202; // WILD symbol = 0. 0 is a good number.
    static constexpr int SCATTER = 201; // SCATTER symbol = 10
    std::vector<int> special_effect_mask_;  // Store special effects mask (respects padding)

    // Helper methods for padding
    bool is_padding_cell(int row, int col) const;
    void init_board_padding(Board& board) const;

public:
    SlotSS03(bool cascade = true, float game_cost = 20.0f, std::string game_type = "base");
    ~SlotSS03() = default;
    
    // SS03-specific steps implementation
    std::tuple<Board, float, int, std::vector<MatchPatterns>, bool> steps(const std::vector<Board>& script);
    
    // Single step method without combo tracking
    std::pair<Board, float> step(const Board& board);

    // Override base class methods to handle padding
    std::pair<MatchPatterns, bool> find_matches(const Board& board) override;
    
    // SS03-specific eliminate_matches for golden tile logic (hides base class method)
    Board eliminate_matches(const Board& board, const MatchPatterns& patterns);
    
    // Override gravity and refill to respect column heights
    Board apply_gravity(const Board& board) override;
    Board refill(const Board& current_board, int current_stop, const std::vector<Board>& script) override;
    
    
    // Override scoring to calculate "ways to win" (without multiplier)
    float get_score(const MatchPatterns& patterns);
    
    // Special effects mask management
    
    // Utility method to create properly padded board
    Board create_padded_board() const;
    
    // Validate board padding
    bool is_valid_padding(const Board& board) const;
    
    // Validate board structure and content (throws BoardValidationError if invalid)
    bool is_valid_board(const Board& board, bool throw_on_error = false) const;
};