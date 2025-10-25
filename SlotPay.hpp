// SlotBase.hpp
#pragma once
#include <vector>
#include <unordered_map>
#include <random>
#include <tuple>

struct GameConfig {
    int board_height;
    int board_width;

    std::vector<int> symbols;
    int min_match_size;
    bool cascade;
    float game_cost;
    std::string game_type;
    std::unordered_map<int, std::unordered_map<int, float>> pay_table;
};

using Board = std::vector<std::vector<int>>;
using MatchPattern = std::vector<std::pair<int, int>>;
using MatchPatterns = std::unordered_map<int, std::vector<std::pair<int, int>>>;

// C++ Oracle base class - implements all common functionality
class SlotBase {
protected:
    GameConfig config_;
    mutable std::mt19937 rng_;

public:
    SlotBase(const GameConfig& config);
    virtual ~SlotBase() = default;

    // Common core methods - base class implementation, shared by all subclasses
    Board eliminate_matches(const Board& board, const MatchPatterns& patterns);
    virtual Board apply_gravity(const Board& board) = 0;
    virtual Board refill(const Board& current_board, int current_stop, const std::vector<Board>& script) = 0;
    float get_score(const MatchPatterns& patterns);
    bool is_terminal(const Board& board);

    // Pure virtual functions - each game must implement its own matching logic
    virtual std::pair<MatchPatterns, bool> find_matches(const Board& board) = 0;

    // Public accessors for C interface
    const GameConfig& get_config() const { return config_; }

    // Getters
    int get_board_height() const { return config_.board_height; }
    int get_board_width() const { return config_.board_width; }
    const std::vector<int>& get_symbols() const { return config_.symbols; }
    int get_min_match_size() const { return config_.min_match_size; }

    // Print board state
    static void printBoard(const Board& board);
};