#pragma once

#include "SlotPay.hpp"
#include "ScriptConfig.h"
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <cmath>

struct PatternInfo {
    int symbol;
    int count;
};

struct ScriptResult {
    int index;
    double expectedPayout;
    double calculatedPayout;
    int expectedStop;
    int actualStop;
    bool payoutMismatch;
    bool stopMismatch;
    bool cascadingMismatch;
    std::vector<PatternInfo> firstBoardPatterns;  // All patterns found in first board
};

// Analysis context to replace global variables
struct AnalysisContext {
    int stopMismatches = 0;
    int cascadingMismatches = 0;
    int terminalLastBoardScripts = 0;
    double totalPayout = 0.0;
    double totalCalculatedPayout = 0.0;
    int payoutMismatches = 0;
    std::vector<ScriptResult> allResults;
    
    void reset() {
        stopMismatches = 0;
        cascadingMismatches = 0;
        terminalLastBoardScripts = 0;
        totalPayout = 0.0;
        totalCalculatedPayout = 0.0;
        payoutMismatches = 0;
        allResults.clear();
    }
};

class BoardAnalyzer {
public:
    // Helper function to check first board uniqueness for a specific script collection
    static void checkFirstBoardSetUniqueness(const std::map<int, ScriptApp::ScriptData>& scripts, const std::string& scriptType) {
        std::cout << "\n=== Checking " << scriptType << " First Board Uniqueness ===\n";
        std::cout << "Total " << scriptType << " scripts to check: " << scripts.size() << "\n";
        
        if (scripts.empty()) {
            std::cout << "No " << scriptType << " scripts to analyze.\n";
            return;
        }
        
        std::map<Board, std::vector<int>> firstBoardToIndices;
        
        // Collect all first boards and track which script indices they belong to
        for (const auto& [index, scriptData] : scripts) {
            if (!scriptData.script.empty()) {
                Board firstBoard = scriptData.script[0]; // Get the first board
                firstBoardToIndices[firstBoard].push_back(index);
            }
        }
        
        std::cout << "Number of unique first boards: " << firstBoardToIndices.size() << "\n";
        
        // Find and report identical first boards
        int identicalCount = 0;
        
        for (const auto& [firstBoard, scriptIndices] : firstBoardToIndices) {
            if (scriptIndices.size() > 1) {
                identicalCount++;
                std::cout << "❌ IDENTICAL FIRST BOARD found in " << scriptIndices.size() << " scripts: ";
                for (size_t i = 0; i < scriptIndices.size(); ++i) {
                    std::cout << scriptIndices[i];
                    if (i < scriptIndices.size() - 1) std::cout << ", ";
                }
                std::cout << "\n";
            }
        }
        
        if (identicalCount == 0) {
            std::cout << "✅ ALL " << scriptType << " FIRST BOARDS ARE UNIQUE!\n";
        } else {
            std::cout << "⚠️  Found " << identicalCount << " groups of " << scriptType << " scripts with identical first boards.\n";
        }
    }

    // First board uniqueness analysis - checks both base and free scripts
    static void checkFirstBoardUniqueness(const ScriptApp::ScriptConfig& config) {
        std::cout << "\n******* CHECKING FIRST BOARD UNIQUENESS *******\n";
        
        // Check base scripts
        checkFirstBoardSetUniqueness(config.base_scripts, "BASE");
        
        // Check free scripts
        checkFirstBoardSetUniqueness(config.free_scripts, "FREE");
    }

    // Script display
    static void printScript(const ScriptApp::ScriptData::Script& script) {
        for (size_t i = 0; i < script.size(); ++i) {
            std::cout << "Board " << i << ":\n";
            SlotBase::printBoard(script[i]);
        }
    }
    
    // Export results to JSON (generic, works for any game)
    static void exportResultsToJson(const std::string& filename, size_t baseScriptCount, size_t freeScriptCount,
                                     double baseTotalExpected, double baseTotalCalculated,
                                     double freeTotalExpected, double freeTotalCalculated,
                                     double fgTriggerProb, const AnalysisContext& context) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not create " << filename << "\n";
            return;
        }
        
        // Calculate combined totals
        double combinedExpected = baseTotalExpected + (fgTriggerProb * freeTotalExpected);
        double combinedCalculated = baseTotalCalculated + (fgTriggerProb * freeTotalCalculated);
        
        file << "{\n";
        file << "  \"summary\": {\n";
        file << "    \"totalScripts\": " << context.allResults.size() << ",\n";
        file << "    \"baseScripts\": " << baseScriptCount << ",\n";
        file << "    \"freeScripts\": " << freeScriptCount << ",\n";
        file << "    \"fgTriggerProbability\": " << std::fixed << std::setprecision(4) << fgTriggerProb << ",\n";
        file << "    \"baseGame\": {\n";
        file << "      \"totalExpectedPayout\": " << std::fixed << std::setprecision(6) << baseTotalExpected << ",\n";
        file << "      \"totalCalculatedPayout\": " << std::fixed << std::setprecision(6) << baseTotalCalculated << ",\n";
        file << "      \"averageExpectedPayout\": " << std::fixed << std::setprecision(6) << (baseScriptCount > 0 ? baseTotalExpected / baseScriptCount : 0.0) << ",\n";
        file << "      \"averageCalculatedPayout\": " << std::fixed << std::setprecision(6) << (baseScriptCount > 0 ? baseTotalCalculated / baseScriptCount : 0.0) << "\n";
        file << "    },\n";
        file << "    \"freeGame\": {\n";
        file << "      \"totalExpectedPayout\": " << std::fixed << std::setprecision(6) << freeTotalExpected << ",\n";
        file << "      \"totalCalculatedPayout\": " << std::fixed << std::setprecision(6) << freeTotalCalculated << ",\n";
        file << "      \"averageExpectedPayout\": " << std::fixed << std::setprecision(6) << (freeScriptCount > 0 ? freeTotalExpected / freeScriptCount : 0.0) << ",\n";
        file << "      \"averageCalculatedPayout\": " << std::fixed << std::setprecision(6) << (freeScriptCount > 0 ? freeTotalCalculated / freeScriptCount : 0.0) << ",\n";
        file << "      \"weightedExpectedPayout\": " << std::fixed << std::setprecision(6) << (fgTriggerProb * freeTotalExpected) << ",\n";
        file << "      \"weightedCalculatedPayout\": " << std::fixed << std::setprecision(6) << (fgTriggerProb * freeTotalCalculated) << "\n";
        file << "    },\n";
        file << "    \"combined\": {\n";
        file << "      \"totalExpectedPayout\": " << std::fixed << std::setprecision(6) << combinedExpected << ",\n";
        file << "      \"totalCalculatedPayout\": " << std::fixed << std::setprecision(6) << combinedCalculated << ",\n";
        file << "      \"averagePerBaseGameSpin\": " << std::fixed << std::setprecision(6) << (baseScriptCount > 0 ? combinedExpected / baseScriptCount : 0.0) << "\n";
        file << "    },\n";
        file << "    \"mismatches\": {\n";
        file << "      \"payoutMismatches\": " << context.payoutMismatches << ",\n";
        file << "      \"stopMismatches\": " << context.stopMismatches << ",\n";
        file << "      \"cascadingMismatches\": " << context.cascadingMismatches << "\n";
        file << "    }\n";
        file << "  },\n";
        file << "  \"baseScripts\": [\n";
        
        // Export base scripts (first baseScriptCount items)
        for (size_t i = 0; i < baseScriptCount && i < context.allResults.size(); ++i) {
            const auto& result = context.allResults[i];
            file << "    {\n";
            file << "      \"index\": " << result.index << ",\n";
            file << "      \"expectedPayout\": " << std::fixed << std::setprecision(6) << result.expectedPayout << ",\n";
            file << "      \"calculatedPayout\": " << std::fixed << std::setprecision(6) << result.calculatedPayout << ",\n";
            file << "      \"expectedStop\": " << result.expectedStop << ",\n";
            file << "      \"actualStop\": " << result.actualStop << ",\n";
            file << "      \"payoutMismatch\": " << (result.payoutMismatch ? "true" : "false") << ",\n";
            file << "      \"stopMismatch\": " << (result.stopMismatch ? "true" : "false") << ",\n";
            file << "      \"cascadingMismatch\": " << (result.cascadingMismatch ? "true" : "false") << ",\n";
            file << "      \"firstBoardPatterns\": [\n";
            
            for (size_t j = 0; j < result.firstBoardPatterns.size(); ++j) {
                const auto& pattern = result.firstBoardPatterns[j];
                file << "        {\n";
                file << "          \"symbol\": " << pattern.symbol << ",\n";
                file << "          \"count\": " << pattern.count << "\n";
                file << "        }";
                if (j < result.firstBoardPatterns.size() - 1) {
                    file << ",";
                }
                file << "\n";
            }
            
            file << "      ]\n";
            file << "    }";
            if (i < baseScriptCount - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ],\n";
        file << "  \"freeScripts\": [\n";
        
        // Export free scripts (remaining items after baseScriptCount)
        for (size_t i = baseScriptCount; i < context.allResults.size(); ++i) {
            const auto& result = context.allResults[i];
            file << "    {\n";
            file << "      \"index\": " << result.index << ",\n";
            file << "      \"expectedPayout\": " << std::fixed << std::setprecision(6) << result.expectedPayout << ",\n";
            file << "      \"calculatedPayout\": " << std::fixed << std::setprecision(6) << result.calculatedPayout << ",\n";
            file << "      \"expectedStop\": " << result.expectedStop << ",\n";
            file << "      \"actualStop\": " << result.actualStop << ",\n";
            file << "      \"payoutMismatch\": " << (result.payoutMismatch ? "true" : "false") << ",\n";
            file << "      \"stopMismatch\": " << (result.stopMismatch ? "true" : "false") << ",\n";
            file << "      \"cascadingMismatch\": " << (result.cascadingMismatch ? "true" : "false") << ",\n";
            file << "      \"firstBoardPatterns\": [\n";
            
            for (size_t j = 0; j < result.firstBoardPatterns.size(); ++j) {
                const auto& pattern = result.firstBoardPatterns[j];
                file << "        {\n";
                file << "          \"symbol\": " << pattern.symbol << ",\n";
                file << "          \"count\": " << pattern.count << "\n";
                file << "        }";
                if (j < result.firstBoardPatterns.size() - 1) {
                    file << ",";
                }
                file << "\n";
            }
            
            file << "      ]\n";
            file << "    }";
            if (i < context.allResults.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        file.close();
        std::cout << "Results exported to " << filename << "\n";
    }
    
    // Analysis summary and reporting (generic)
    static void printAnalysisSummary(double expectedAverage, double calculatedAverage, 
                                    size_t totalScripts, int payoutMismatches, int stopMismatches, 
                                    int cascadingMismatches, int terminalLastBoardScripts) {
        std::cout << "\nPayout Summary:\n";
        std::cout << "Number of scripts: " << totalScripts << "\n";
        std::cout << "Expected Average Payout: " << std::fixed << std::setprecision(2) << expectedAverage << "\n";
        std::cout << "Calculated Average Payout: " << std::fixed << std::setprecision(2) << calculatedAverage << "\n";
        std::cout << "\nMismatch Summary:\n";
        std::cout << "Payout mismatches: " << payoutMismatches << " out of " << totalScripts << " scripts\n";
        std::cout << "Stop mismatches: " << stopMismatches << " out of " << totalScripts << " scripts\n";
        std::cout << "Cascading mismatches: " << cascadingMismatches << " out of " << totalScripts << " scripts\n";
        
        std::cout << "\nTerminal State Summary:\n";
        if (static_cast<size_t>(terminalLastBoardScripts) == totalScripts) {
            std::cout << "✅ Scripts with last board in terminal state: " << terminalLastBoardScripts << " out of " << totalScripts << " scripts\n";
        } else {
            std::cout << "❌ Scripts with last board in terminal state: " << terminalLastBoardScripts << " out of " << totalScripts << " scripts\n";
        }
        
        if (payoutMismatches == 0 && stopMismatches == 0 && cascadingMismatches == 0) {
            std::cout << "✅ ALL SCRIPTS MATCH! Perfect payout, stop, and cascading accuracy.\n";
        } else {
            std::cout << "⚠️  Found mismatches in script validation.\n";
        }
    }        
    
    static void handleScriptMismatch(int index, const std::exception& e, int& displayCount) {
        if (displayCount < 5) {
            displayCount++;
            std::cerr << "\n*** MISMATCH #" << displayCount << " - Script " << index << " ***\n";
            std::cerr << "Mismatch during script execution: " << e.what() << "\n";
            std::cerr << "******************************************\n";
        }
    }
};
