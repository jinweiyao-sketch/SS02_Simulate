#include "SlotPay.hpp"
#include "SS02Pay.hpp"
#include "ScriptConfig.h"
#include "BoardAnalyzer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

// SS02-specific analysis functions

// Helper function to analyze a set of SS02 scripts
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
        // Create a new SS02Pay instance for each script with appropriate game type
        SlotSS02 game(true, 20.0f, gameType); 
        
        {
            // Run without printing details
            if (scriptData.script.empty()) {
                std::cout << "ERROR: Empty script found at index " << index << "\n";
                std::cout << "Data integrity issue detected. Stopping analysis.\n";
                return;
            }
            
            try {
                auto [final_board, total_score, actual_stop, patterns, boards_match] = game.steps(scriptData.script, scriptData.special_multipliers);
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

// Main SS02 analysis function
static void analyzeScripts(const ScriptApp::ScriptConfig& config, AnalysisContext& context) {
    // Create a game instance to get the FG trigger probability from SS02
    SlotSS02 game(true, 20.0f, "base");
    double fgTriggerProb = game.get_fg_trigger_probability();
    double fgRetriggerProb = game.get_fg_retrigger_probability();
    
    std::cout << "\n==============================================\n";
    std::cout << "       SCRIPT ANALYSIS OVERVIEW\n";
    std::cout << "==============================================\n";
    std::cout << "Total base scripts: " << config.base_scripts.size() << "\n";
    std::cout << "Total free scripts: " << config.free_scripts.size() << "\n";
    std::cout << "FG Trigger Probability: " << std::fixed << std::setprecision(4) << fgTriggerProb << "\n";
    std::cout << "FG Retrigger Probability: " << std::fixed << std::setprecision(4) << fgRetriggerProb << "\n";
    
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
    
    // Reset for free scripts
    context.totalPayout = 0.0;
    context.totalCalculatedPayout = 0.0;
    
    // Analyze free scripts
    analyzeScriptSet(config.free_scripts, "FREE", "free", context);
    freeTotalExpected = context.totalPayout;
    freeTotalCalculated = context.totalCalculatedPayout;
    
    // Print overall summary (payout statistics only)
    std::cout << "\n==============================================\n";
    std::cout << "       OVERALL SUMMARY (BASE + FREE)\n";
    std::cout << "==============================================\n";
    std::cout << "Total Base Scripts: " << config.base_scripts.size() << "\n";
    std::cout << "Total Free Scripts: " << config.free_scripts.size() << "\n";
    std::cout << "FG Trigger Probability: " << std::fixed << std::setprecision(4) << fgTriggerProb << "\n";
    std::cout << "FG Retrigger Probability: " << std::fixed << std::setprecision(4) << fgRetriggerProb << "\n";
    
    // Calculate averages for base and free
    double baseExpectedAvg = config.base_scripts.size() > 0 ? (baseTotalExpected / config.base_scripts.size()) : 0.0;
    double baseCalculatedAvg = config.base_scripts.size() > 0 ? (baseTotalCalculated / config.base_scripts.size()) : 0.0;
    double freeExpectedAvg = config.free_scripts.size() > 0 ? (freeTotalExpected / config.free_scripts.size()) : 0.0;
    double freeCalculatedAvg = config.free_scripts.size() > 0 ? (freeTotalCalculated / config.free_scripts.size()) : 0.0;
    
    // Calculate expected FG length with retrigger: 10 / (1 - retrigger_prob * 10)
    double expectedFGLength = 10.0 / (1.0 - (fgRetriggerProb * 10.0));
    
    std::cout << "\nBase Game Payout:\n";
    std::cout << "  Expected Average: " << std::fixed << std::setprecision(2) << baseExpectedAvg << "\n";
    std::cout << "  Calculated Average: " << std::fixed << std::setprecision(2) << baseCalculatedAvg << "\n";
    
    // Calculate base game variance from results
    double baseCalculatedVariance = 0.0;
    size_t baseCount = 0;
    for (size_t i = 0; i < config.base_scripts.size() && i < context.allResults.size(); ++i) {
        double diff = context.allResults[i].calculatedPayout - baseCalculatedAvg;
        baseCalculatedVariance += diff * diff;
        baseCount++;
    }
    if (baseCount > 0) {
        baseCalculatedVariance /= baseCount;
    }
    std::cout << "  Calculated Variance: " << std::fixed << std::setprecision(2) << baseCalculatedVariance << "\n";
    std::cout << "  Calculated Std Dev: " << std::fixed << std::setprecision(2) << std::sqrt(baseCalculatedVariance) << "\n";
    
    std::cout << "\nFree Game Payout:\n";
    std::cout << "  Expected Average: " << std::fixed << std::setprecision(2) << freeExpectedAvg << "\n";
    std::cout << "  Calculated Average: " << std::fixed << std::setprecision(2) << freeCalculatedAvg << "\n";
    
    // Calculate free game variance from results
    double freeCalculatedVariance = 0.0;
    size_t freeCount = 0;
    for (size_t i = config.base_scripts.size(); i < context.allResults.size(); ++i) {
        double diff = context.allResults[i].calculatedPayout - freeCalculatedAvg;
        freeCalculatedVariance += diff * diff;
        freeCount++;
    }
    if (freeCount > 0) {
        freeCalculatedVariance /= freeCount;
    }
    std::cout << "  Calculated Variance: " << std::fixed << std::setprecision(2) << freeCalculatedVariance << "\n";
    std::cout << "  Calculated Std Dev: " << std::fixed << std::setprecision(2) << std::sqrt(freeCalculatedVariance) << "\n";
    
    std::cout << "\nExpected FG Length (with retrigger): " << std::fixed << std::setprecision(2) << expectedFGLength << "\n";
    
    // Calculate total average payout = base_average + fg_average * FG_trigger_prob * expected_FG_length
    double overallExpectedAvg = baseExpectedAvg + (freeExpectedAvg * fgTriggerProb * expectedFGLength);
    double overallCalculatedAvg = baseCalculatedAvg + (freeCalculatedAvg * fgTriggerProb * expectedFGLength);
    std::cout << "\nAverage Payout per Base Game Spin:\n";
    std::cout << "  Expected Average: " << std::fixed << std::setprecision(2) << overallExpectedAvg << "\n";
    std::cout << "  Calculated Average: " << std::fixed << std::setprecision(2) << overallCalculatedAvg << "\n";
    std::cout << "  Average Difference: " << std::fixed << std::setprecision(2) << (overallCalculatedAvg - overallExpectedAvg) << "\n";
    
    // Calculate Antebet Free RTP
    std::cout << "\n==============================================\n";
    std::cout << "       ANTEBET RTP CALCULATION\n";
    std::cout << "==============================================\n";
    double antebetFreeRTP = (overallCalculatedAvg * 1.5 - baseCalculatedAvg) / 30.0;
    double averageFeatureValue = freeCalculatedAvg * expectedFGLength / 30.0;
    
    // First calculate expectedPullsToFG from the RTP relationship
    double expectedPullsToFG = averageFeatureValue / antebetFreeRTP;
    
    // Then solve for mystryTrigger from: expectedPullsToFG = 1 / (1 - (1 - fgTriggerProb) * (1 - mystryTrigger))
    // Rearranging: mystryTrigger = 1 - (1 - 1/expectedPullsToFG) / (1 - fgTriggerProb)
    double mystryTrigger = 1.0 - (1.0 - 1.0/expectedPullsToFG) / (1.0 - fgTriggerProb);
    
    std::cout << "Antebet Free RTP: " << std::fixed << std::setprecision(4) << antebetFreeRTP << "\n";
    std::cout << "Average Feature Value: " << std::fixed << std::setprecision(4) << averageFeatureValue << "\n";
    std::cout << "Expected Pulls to Free Games: " << std::fixed << std::setprecision(2) << expectedPullsToFG << "\n";
    std::cout << "Mystery Trigger Probability: " << std::fixed << std::setprecision(4) << mystryTrigger << "\n";
    
    // Export mystery trigger to file for integration into Insert_Script.json
    try {
        // Round to 4 decimal places
        double roundedMystryTrigger = std::round(mystryTrigger * 10000.0) / 10000.0;

        // Manually format JSON to preserve 4 decimal places
        std::ostringstream oss;
        oss << "{\n  \"double_chance_rate\": " << std::fixed << std::setprecision(4) << roundedMystryTrigger << "\n}";
        std::string json_content = oss.str();
        
        std::ofstream mystery_file("SS02_mystery_trigger.json");
        if (mystery_file.is_open()) {
            mystery_file << json_content;
            mystery_file.close();
            std::cout << "✅ Exported mystery trigger to SS02_mystery_trigger.json\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "⚠️  Warning: Failed to export mystery trigger: " << e.what() << "\n";
    }

    // Calculate Antebet Free RTP
    std::cout << "\n==============================================\n";
    std::cout << "       Report Generation\n";
    std::cout << "==============================================\n";
    // Export detailed results to JSON file with separated base and free sections
    BoardAnalyzer::exportResultsToJson("script_results.json", 
                       config.base_scripts.size(), 
                       config.free_scripts.size(),
                       baseTotalExpected, baseTotalCalculated,
                       freeTotalExpected, freeTotalCalculated,
                       fgTriggerProb, context);
}

int main() {
    std::cout << "=== SS02Pay Script Test Program ===\n\n";
    
    try {
        std::cout << "DEBUG: About to load configuration file\n";
        auto config = ScriptApp::ScriptConfig::loadFromFile("SS02_scripts.json");
        std::cout << "DEBUG: Configuration file loaded successfully\n\n";

        // Create analysis context
        AnalysisContext context;

        // Check first board uniqueness for both base and free
        BoardAnalyzer::checkFirstBoardUniqueness(config);
        
        // Analyze all scripts (base and free separately) - SS02-specific version
        analyzeScripts(config, context);
        
        // Export multiplier tables
        std::cout << "\n============================================\n";
        std::cout << "Exporting Multiplier Tables\n";
        std::cout << "============================================\n";
        
        try {
            // Create a game instance to get volatility type
            SlotSS02 game(true, 20.0f, "free");
            std::string volatility_type = game.get_volatility_type();
            
            // Get only the current volatility table
            nlohmann::json multiplier_table = SlotSS02::get_multiplier_table(volatility_type);
            
            // Convert to string first to avoid any stream issues
            std::string table_str = multiplier_table.dump(2);
            
            // Export to file only (don't print)
            std::ofstream mult_file("SS02_multiplier_table.json");
            if (mult_file.is_open()) {
                mult_file.write(table_str.c_str(), table_str.length());
                mult_file.close();
                
                std::cout << "✅ Exported multiplier table to SS02_multiplier_table.json\n";
                std::cout << "   - Volatility Type: " << volatility_type << "\n";
            } else {
                std::cerr << "⚠️  Warning: Could not open SS02_multiplier_table.json for writing\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "⚠️  Warning: Failed to export multiplier tables: " << e.what() << "\n";
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
