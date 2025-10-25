// cpp_ss02.hpp
#pragma once
#include "SlotPay.hpp"
#include "json.hpp"

// SS02 Oracle - inherits from C++ base class
class SlotSS02 : public SlotBase {
private:
    void init_ss02_pay_table();
    
    // Multiplier symbol for free games
    static constexpr int MULTIPLIER = 202;
    
    // Free game trigger probability (SS02-specific)
    double fg_trigger_probability_;
    
    // Free game retrigger probability (SS02-specific)
    double fg_retrigger_probability_;
    
    // Volatility type for SS02 (default: high)
    std::string volatility_type_;

public:
    SlotSS02(bool cascade = true, float game_cost = 20.0f, std::string game_type = "base");
    ~SlotSS02() = default;

    // SS02-specific steps implementation
    std::tuple<Board, float, int, std::vector<MatchPatterns>, bool> steps(const std::vector<Board>& script, int special_multipliers = 1);

    // Override gravity with SS02-specific behavior
    Board apply_gravity(const Board& board) override;

    // Override refill with SS02-specific behavior
    Board refill(const Board& current_board, int current_stop, const std::vector<Board>& script) override;

    // Implement SS02 specific matching logic (cluster matching - 8+ symbols anywhere)
    std::pair<MatchPatterns, bool> find_matches(const Board& board) override;
    
    // Getter for free game trigger probability
    double get_fg_trigger_probability() const { return fg_trigger_probability_; }
    
    // Setter for free game trigger probability
    void set_fg_trigger_probability(double probability) { fg_trigger_probability_ = probability; }
    
    // Getter for free game retrigger probability
    double get_fg_retrigger_probability() const { return fg_retrigger_probability_; }
    
    // Setter for free game retrigger probability
    void set_fg_retrigger_probability(double probability) { fg_retrigger_probability_ = probability; }
    
    // Getter for volatility type
    std::string get_volatility_type() const { return volatility_type_; }
    
    // Setter for volatility type
    void set_volatility_type(const std::string& type) { volatility_type_ = type; }
    
    // Get multiplier table based on volatility type
    static nlohmann::json get_multiplier_table(const std::string& volatility_type);
    
private:
    // Static multiplier tables for different volatility types
    static const nlohmann::json HIGH_VOLATILITY_TABLE;
    static const nlohmann::json LOW_VOLATILITY_TABLE;
};