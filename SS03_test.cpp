#include "SlotPay.hpp"
#include "SS03Pay.hpp"
#include "ScriptConfig.h"
#include "BoardAnalyzer.h"
#include <iostream>
#include <cmath>

// SS03-specific analysis functions

// Helper function to analyze a set of SS03 scripts
static void analyzeScriptSet(const std::map<int, ScriptApp::ScriptData>& scripts, 
                              const std::string& scriptType, 
                              const std::string& gameType,
                              AnalysisContext& context) {
    std::cout << "\n============================================\n";
    std::cout << "***** ANALYZING " << scriptType << " SCRIPTS *****\n";
    std::cout << "============================================\n";
    std::cout << "Total " << scriptType << " scripts: " << scripts.size() << "\n";
    
    if (scripts.empty()) {
        std::cout << "No " << scriptType << " scripts to analyze.\n";
        return;
    }
    
    // Reset counters for this script set
    int stopMismatches = 0;
    int cascadingMismatches = 0;
    int terminalLastBoardScripts = 0;
    double totalPayout = 0.0;
    double totalCalculatedPayout = 0.0;
    int payoutMismatches = 0;
    std::vector<ScriptResult> results;
    
    int displayCount = 0;  // Track displayed items (mismatches + errors)
    
    std::cout << "\n***** RUNNING MISMATCH CHECKS: Stop, Cascading, Terminal *****\n";
    
    for (const auto& [index, scriptData] : scripts) {
        // Create a new SS03Pay instance for each script with appropriate game type
        SlotSS03 game(true, 20.0f, gameType); 
        
        {
            // Run without printing details
            if (scriptData.script.empty()) {
                std::cout << "ERROR: Empty script found at index " << index << "\n";
                std::cout << "Data integrity issue detected. Stopping analysis.\n";
                return;
            }
            
            try {
                auto [final_board, total_score, actual_stop, patterns, boards_match] = game.steps(scriptData.script);
                bool stopMismatch = (actual_stop != scriptData.stop);
                
                // Show details for first 5 mismatches only
                if ((stopMismatch || !boards_match) && displayCount < 5) {
                    displayCount++;
                    std::cout << "\n*** MISMATCH #" << displayCount << " - Script " << index << " ***\n";
                    std::cout << "Calculated Score: " << total_score << "\n";
                    std::cout << "Expected Stop: " << scriptData.stop << ", Actual: " << actual_stop << "\n";
                    if (stopMismatch) std::cout << "❌ STOP MISMATCH!\n";
                    else std::cout << "✅ STOP MATCH!\n";
                    if (!boards_match) std::cout << "❌ Cascading MISMATCH WARNING!\n";
                    else std::cout << "✅ CASCADING MATCH!\n";
                    std::cout << "Script has " << scriptData.script.size() << " boards\n\n";
                    
                    // Print all boards in this script
                    for (size_t i = 0; i < scriptData.script.size(); ++i) {
                        std::cout << "Board " << i << ":\n";
                        SlotBase::printBoard(scriptData.script[i]);
                    }

                    std::cout << "Final Board after cascading:\n";
                    SlotBase::printBoard(final_board);
                    
                    // Print pattern information
                    std::cout << "Patterns found during processing:\n";
                    for (size_t step = 0; step < patterns.size(); ++step) {
                        std::cout << "Step " << step << ":\n";
                        for (const auto& [symbol, positions] : patterns[step]) {
                            if (!positions.empty()) {
                                std::cout << "  Symbol " << symbol << ": " << positions.size() << " matches\n";
                            }
                        }
                    }

                    if (!game.is_terminal(final_board)) {
                        std::cout << "\n❌ Final board is not in a terminal state, but stopped due to lack of next board within the script.\n";
                    } else {
                        std::cout << "\n✅ Final board is indeed a terminal state.\n";
                    }

                    std::cout << "******************************************\n";
                }
                
                // Check if the last board in the script is in terminal state
                if (!scriptData.script.empty()) {
                    const Board& lastBoard = scriptData.script.back();
                    if (game.is_terminal(lastBoard)) {
                        terminalLastBoardScripts++;
                    }
                }
                
                // Calculate first board patterns
                std::vector<PatternInfo> firstBoardPatterns;
                if (!patterns.empty()) {
                    for (const auto& [symbol, positions] : patterns[0]) {
                        if (!positions.empty()) {
                            firstBoardPatterns.push_back({symbol, static_cast<int>(positions.size())});
                        }
                    }
                }
                
                // Get expected payout from script data
                double expectedPayout = static_cast<double>(scriptData.payout);
                double calculatedPayout = static_cast<double>(total_score);
                
                // Update totals
                totalPayout += expectedPayout;
                totalCalculatedPayout += calculatedPayout;
                
                // Check for payout mismatch
                bool payoutMismatch = (expectedPayout != calculatedPayout);
                if (payoutMismatch) payoutMismatches++;
                
                // Record this script's results
                results.push_back({
                    index,
                    expectedPayout,
                    calculatedPayout,
                    scriptData.stop,
                    actual_stop,
                    payoutMismatch,
                    stopMismatch,
                    !boards_match,  // cascading mismatch
                    firstBoardPatterns
                });
                
                if (stopMismatch) stopMismatches++;
                if (!boards_match) cascadingMismatches++;
            } catch (const std::exception& e) {
                BoardAnalyzer::handleScriptMismatch(index, e, displayCount);
            }
        }
    }
    
    // Calculate averages for this script set
    double expectedAverage = results.empty() ? 0.0 : (totalPayout / results.size());
    double calculatedAverage = results.empty() ? 0.0 : (totalCalculatedPayout / results.size());
    
    // Calculate variance for calculated payout
    double calculatedVariance = 0.0;
    if (!results.empty()) {
        for (const auto& result : results) {
            double diff = result.calculatedPayout - calculatedAverage;
            calculatedVariance += diff * diff;
        }
        calculatedVariance /= results.size();
    }
    
    // Print summary for this script set
    std::cout << "\n========== " << scriptType << " SCRIPTS SUMMARY ==========\n";
    BoardAnalyzer::printAnalysisSummary(expectedAverage, calculatedAverage,
                        scripts.size(), payoutMismatches, stopMismatches, 
                        cascadingMismatches, terminalLastBoardScripts);
    
    // Print variance information
    std::cout << "\nVariance Analysis:\n";
    std::cout << "Calculated Payout Variance: " << std::fixed << std::setprecision(2) << calculatedVariance << "\n";
    std::cout << "Calculated Payout Standard Deviation: " << std::fixed << std::setprecision(2) << std::sqrt(calculatedVariance) << "\n";
    
    // Append to context
    context.allResults.insert(context.allResults.end(), results.begin(), results.end());
    context.stopMismatches += stopMismatches;
    context.cascadingMismatches += cascadingMismatches;
    context.terminalLastBoardScripts += terminalLastBoardScripts;
    context.totalPayout += totalPayout;
    context.totalCalculatedPayout += totalCalculatedPayout;
    context.payoutMismatches += payoutMismatches;
}

// Main SS03 analysis function
static void analyzeScripts(const ScriptApp::ScriptConfig& config, AnalysisContext& context) {
    // Reset context
    context.reset();
    
    // Track base and free payouts separately
    double baseTotalExpected = 0.0;
    double baseTotalCalculated = 0.0;
    double freeTotalExpected = 0.0;
    double freeTotalCalculated = 0.0;
    
    // Store current totals before analyzing base scripts
    analyzeScriptSet(config.base_scripts, "BASE", "base", context);
    baseTotalExpected = context.totalPayout;
    baseTotalCalculated = context.totalCalculatedPayout;
    
    // Skip free scripts analysis for now
    // Reset for free scripts
    // context.totalPayout = 0.0;
    // context.totalCalculatedPayout = 0.0;
    
    // Analyze free scripts
    // analyzeScriptSet(config.free_scripts, "FREE", "free", context);
    freeTotalExpected = 0.0; // context.totalPayout;
    freeTotalCalculated = 0.0; // context.totalCalculatedPayout;
    
    // Export detailed results to JSON file with separated base and free sections
    BoardAnalyzer::exportResultsToJson("majiang_script_results.json", 
                       config.base_scripts.size(), 
                       config.free_scripts.size(),
                       baseTotalExpected, baseTotalCalculated,
                       freeTotalExpected, freeTotalCalculated,
                       0.0, context);  // SS03 doesn't have FG trigger probability
}

int main() {
    std::cout << "=== SS03Pay Script Test Program ===\n\n";
    
    try {
        std::cout << "DEBUG: About to load configuration file\n";
        auto config = ScriptApp::ScriptConfig::loadFromFile("majiang_222.json");
        std::cout << "DEBUG: Configuration file loaded successfully\n\n";

        // Create analysis context
        AnalysisContext context;

        // Check first board uniqueness for both base and free
        BoardAnalyzer::checkFirstBoardUniqueness(config);
        
        // Print all boards in script index 2 of base
        std::cout << "\n============================================\n";
        std::cout << "***** SCRIPT INDEX 2 - ALL BOARDS *****\n";
        std::cout << "============================================\n";
        
        auto it = config.base_scripts.find(2);
        if (it != config.base_scripts.end()) {
            const auto& scriptData = it->second;
            std::cout << "Script Index: 2\n";
            std::cout << "Expected Payout: " << scriptData.payout << "\n";
            std::cout << "Expected Stop: " << scriptData.stop << "\n";
            std::cout << "Total Boards: " << scriptData.script.size() << "\n\n";
            
            // Create a game instance to analyze patterns
            SlotSS03 game(true, 20.0f, "base");
            
            for (size_t i = 0; i < scriptData.script.size(); ++i) {
                std::cout << "Board " << i << ":\n";
                SlotBase::printBoard(scriptData.script[i]);
                
                // Find and print match patterns for this board
                auto [patterns, has_match] = game.find_matches(scriptData.script[i]);
                if (has_match) {
                    std::cout << "Match Patterns:\n";
                    for (const auto& [symbol, positions] : patterns) {
                        if (!positions.empty()) {
                            std::cout << "  Symbol " << symbol << ": " << positions.size() << " matches at positions: ";
                            for (const auto& [row, col] : positions) {
                                std::cout << "(" << row << "," << col << ") ";
                            }
                            std::cout << "\n";
                        }
                    }
                    float score = game.get_score(patterns);
                    std::cout << "  Board Score: " << score << "\n";
                } else {
                    std::cout << "No matches found (terminal state)\n";
                }
                std::cout << "\n";
            }
        } else {
            std::cout << "Script index 2 not found in base scripts.\n";
        }
        std::cout << "============================================\n\n";
        
        // Analyze all scripts (base and free separately) - SS03-specific version
        analyzeScripts(config, context);
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
