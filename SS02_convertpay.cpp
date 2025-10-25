#include "ScriptConfig.h"
#include "SS02Pay.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <set>

// Function to find overlap using subsequence matching
// Finds longest consecutive prefix of currentBoard that appears as a subsequence in previousBoard
// Does not allow prefix to end with eliminated symbols
int findBoardOverlap(const std::vector<int>& previousBoard, const std::vector<int>& currentBoard, 
                     const MatchPatterns& eliminationPatterns, int currentCol) {
    int maxOverlap = 0;
    
    // Try all possible prefix lengths of currentBoard (from longest to shortest)
    for (int prefixLen = static_cast<int>(currentBoard.size()); prefixLen >= 1; --prefixLen) {
        
        // Check if the last symbol of this prefix is an eliminated symbol
        bool endsWithEliminatedSymbol = false;
        int lastSymbol = currentBoard[prefixLen - 1];
        
        for (const auto& [symbol, positions] : eliminationPatterns) {
            for (const auto& [row, col] : positions) {
                if (col == currentCol && symbol == lastSymbol) {
                    endsWithEliminatedSymbol = true;
                    break;
                }
            }
            if (endsWithEliminatedSymbol) break;
        }
        
        // Skip this prefix if it ends with an eliminated symbol
        if (endsWithEliminatedSymbol) {
            continue;
        }
        
        // Check if this prefix exists as a subsequence in previousBoard
        int prevIndex = 0;
        int matchCount = 0;
        
        for (int currIndex = 0; currIndex < prefixLen; ++currIndex) {
            // Look for currentBoard[currIndex] in previousBoard starting from prevIndex
            while (prevIndex < static_cast<int>(previousBoard.size()) && 
                   previousBoard[prevIndex] != currentBoard[currIndex]) {
                prevIndex++;
            }
            
            if (prevIndex < static_cast<int>(previousBoard.size())) {
                // Found a match
                matchCount++;
                prevIndex++; // Move to next position for next search
            } else {
                // No match found for this element
                break;
            }
        }
        
        // If all elements in the prefix were found as a subsequence
        if (matchCount == prefixLen) {
            maxOverlap = prefixLen;
            return maxOverlap; // Return the first (longest) match found
        }
    }
    
    return maxOverlap;
}

// Function to convert source format to the new format
void convertJsonFormat(const std::string& inputFile, const std::string& outputFile) {
    try {
        auto config = ScriptApp::ScriptConfig::loadFromFile(inputFile);
        
        std::ofstream outFile(outputFile);
        
        // Process base scripts
        if (!config.base_scripts.empty()) {
            SlotSS02 baseGame(true, 20.0f, "base"); // Use "base" game type
            std::vector<std::tuple<int, std::vector<MatchPatterns>, int, float>> base_elimination_data;
            
            for (const auto& [index, scriptData] : config.base_scripts) {
                auto [final_board, total_score, actual_stop, patterns, boards_match] = baseGame.steps(scriptData.script, scriptData.special_multipliers);
                base_elimination_data.emplace_back(index, patterns, actual_stop, total_score);
            }
            
            outFile << "  \"base\": [\n";
            
            bool isFirst = true;
            for (const auto& [index, scriptData] : config.base_scripts) {
                if (!isFirst) outFile << ",\n";
                isFirst = false;
                
                outFile << "    {\n      \"number\": " << index << ",\n      \"stopover\": " << scriptData.stop;
                outFile << ",\n      \"script\": [\n";
                
                for (int col = 0; col < 6; ++col) {
                    if (col > 0) outFile << ",\n";
                    outFile << "        {\n          \"index\": " << col << ",\n";
                    
                    int totalStops = scriptData.script.size() * 5;
                    outFile << "          \"stop\": " << totalStops << ",\n          \"reel\": [";
                    
                    bool firstValue = true;
                    for (const auto& board : scriptData.script) {
                        for (int row = static_cast<int>(board.size()) - 1; row >= 0; --row) {
                            if (board[row].size() > static_cast<size_t>(col)) {
                                if (!firstValue) outFile << ", ";
                                firstValue = false;
                                outFile << board[row][col];
                            }
                        }
                    }
                    outFile << "]\n        }";
                }
                outFile << "\n      ]\n    }";
            }
            outFile << "\n  ]";
        }
        
        // Process free scripts
        if (!config.free_scripts.empty()) {
            if (!config.base_scripts.empty()) {
                outFile << ",\n";
            }
            
            SlotSS02 freeGame(true, 20.0f, "free"); // Use "free" game type
            std::vector<std::tuple<int, std::vector<MatchPatterns>, int, float>> free_elimination_data;
            
            for (const auto& [index, scriptData] : config.free_scripts) {
                auto [final_board, total_score, actual_stop, patterns, boards_match] = freeGame.steps(scriptData.script, scriptData.special_multipliers);
                free_elimination_data.emplace_back(index, patterns, actual_stop, total_score);
            }
            
            outFile << "  \"free\": [\n";
            
            bool isFirst = true;
            for (const auto& [index, scriptData] : config.free_scripts) {
                if (!isFirst) outFile << ",\n";
                isFirst = false;
                
                outFile << "    {\n      \"number\": " << index << ",\n      \"stopover\": " << scriptData.stop;
                
                // Add multiple_table field for free section scripts
                outFile << ",\n      \"multiple_table\": " << scriptData.multiple_table;
                
                outFile << ",\n      \"script\": [\n";
                
                for (int col = 0; col < 6; ++col) {
                    if (col > 0) outFile << ",\n";
                    outFile << "        {\n          \"index\": " << col << ",\n";
                    
                    int totalStops = scriptData.script.size() * 5;
                    outFile << "          \"stop\": " << totalStops << ",\n          \"reel\": [";
                    
                    bool firstValue = true;
                    for (const auto& board : scriptData.script) {
                        for (int row = static_cast<int>(board.size()) - 1; row >= 0; --row) {
                            if (board[row].size() > static_cast<size_t>(col)) {
                                if (!firstValue) outFile << ", ";
                                firstValue = false;
                                outFile << board[row][col];
                            }
                        }
                    }
                    outFile << "]\n        }";
                }
                outFile << "\n      ]\n    }";
            }
            outFile << "\n  ]";
        }
        
        outFile.close();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

// Smart conversion with board overlap detection
void convertJsonFormatAdvanced(const std::string& inputFile, const std::string& outputFile) {
    try {
        auto config = ScriptApp::ScriptConfig::loadFromFile(inputFile);
        
        std::ofstream outFile(outputFile);
        
        // Process base scripts
        if (!config.base_scripts.empty()) {
            SlotSS02 baseGame(true, 20.0f, "base"); // Use "base" game type
            std::map<int, std::vector<MatchPatterns>> base_elimination_data;
            
            for (const auto& [index, scriptData] : config.base_scripts) {
                auto [final_board, total_score, actual_stop, patterns, boards_match] = baseGame.steps(scriptData.script, scriptData.special_multipliers);
                base_elimination_data[index] = patterns;
            }
            
            outFile << "  \"base\": [\n";
            
            bool isFirst = true;
            for (const auto& [index, scriptData] : config.base_scripts) {
                if (!isFirst) outFile << ",\n";
                isFirst = false;
                
                outFile << "    {\n      \"number\": " << index << ",\n      \"stopover\": " << scriptData.stop;
                outFile << ",\n      \"script\": [\n";
                
                for (int col = 0; col < 6; ++col) {
                    if (col > 0) outFile << ",\n";
                    outFile << "        {\n          \"index\": " << col << ",\n";
                    
                    std::vector<int> finalReel;
                    
                    for (size_t boardIdx = 0; boardIdx < scriptData.script.size(); ++boardIdx) {
                        const auto& board = scriptData.script[boardIdx];
                        std::vector<int> currentBoardColumn;
                        for (int row = static_cast<int>(board.size()) - 1; row >= 0; --row) {
                            if (board[row].size() > static_cast<size_t>(col)) {
                                currentBoardColumn.push_back(board[row][col]);
                            }
                        }
                        
                        if (boardIdx == 0) {
                            finalReel.insert(finalReel.end(), currentBoardColumn.begin(), currentBoardColumn.end());
                        } else {
                            std::vector<int> previousBoardColumn;
                            const auto& previousBoard = scriptData.script[boardIdx - 1];
                            for (int row = static_cast<int>(previousBoard.size()) - 1; row >= 0; --row) {
                                if (previousBoard[row].size() > static_cast<size_t>(col)) {
                                    previousBoardColumn.push_back(previousBoard[row][col]);
                                }
                            }
                            
                            MatchPatterns eliminationPatterns;
                            if (boardIdx - 1 < base_elimination_data[index].size()) {
                                eliminationPatterns = base_elimination_data[index][boardIdx - 1];
                            }
                            
                            int maxOverlap = findBoardOverlap(previousBoardColumn, currentBoardColumn, eliminationPatterns, col);
                            
                            for (size_t i = maxOverlap; i < currentBoardColumn.size(); ++i) {
                                finalReel.push_back(currentBoardColumn[i]);
                            }
                        }
                    }
                    
                    outFile << "          \"stop\": " << finalReel.size() << ",\n          \"reel\": [";
                    for (size_t i = 0; i < finalReel.size(); ++i) {
                        if (i > 0) outFile << ", ";
                        outFile << finalReel[i];
                    }
                    outFile << "]\n        }";
                }
                outFile << "\n      ]\n    }";
            }
            outFile << "\n  ]";
        }
        
        // Process free scripts
        if (!config.free_scripts.empty()) {
            if (!config.base_scripts.empty()) {
                outFile << ",\n";
            }
            
            SlotSS02 freeGame(true, 20.0f, "free"); // Use "free" game type
            std::map<int, std::vector<MatchPatterns>> free_elimination_data;
            
            for (const auto& [index, scriptData] : config.free_scripts) {
                auto [final_board, total_score, actual_stop, patterns, boards_match] = freeGame.steps(scriptData.script, scriptData.special_multipliers);
                free_elimination_data[index] = patterns;
            }
            
            outFile << "  \"free\": [\n";
            
            bool isFirst = true;
            for (const auto& [index, scriptData] : config.free_scripts) {
                if (!isFirst) outFile << ",\n";
                isFirst = false;
                
                outFile << "    {\n      \"number\": " << index << ",\n      \"stopover\": " << scriptData.stop;
                
                // Add multiple_table field for free section scripts
                outFile << ",\n      \"multiple_table\": " << scriptData.multiple_table;
                
                outFile << ",\n      \"script\": [\n";
                
                for (int col = 0; col < 6; ++col) {
                    if (col > 0) outFile << ",\n";
                    outFile << "        {\n          \"index\": " << col << ",\n";
                    
                    std::vector<int> finalReel;
                    
                    for (size_t boardIdx = 0; boardIdx < scriptData.script.size(); ++boardIdx) {
                        const auto& board = scriptData.script[boardIdx];
                        std::vector<int> currentBoardColumn;
                        for (int row = static_cast<int>(board.size()) - 1; row >= 0; --row) {
                            if (board[row].size() > static_cast<size_t>(col)) {
                                currentBoardColumn.push_back(board[row][col]);
                            }
                        }
                        
                        if (boardIdx == 0) {
                            finalReel.insert(finalReel.end(), currentBoardColumn.begin(), currentBoardColumn.end());
                        } else {
                            std::vector<int> previousBoardColumn;
                            const auto& previousBoard = scriptData.script[boardIdx - 1];
                            for (int row = static_cast<int>(previousBoard.size()) - 1; row >= 0; --row) {
                                if (previousBoard[row].size() > static_cast<size_t>(col)) {
                                    previousBoardColumn.push_back(previousBoard[row][col]);
                                }
                            }
                            
                            MatchPatterns eliminationPatterns;
                            if (boardIdx - 1 < free_elimination_data[index].size()) {
                                eliminationPatterns = free_elimination_data[index][boardIdx - 1];
                            }
                            
                            int maxOverlap = findBoardOverlap(previousBoardColumn, currentBoardColumn, eliminationPatterns, col);
                            
                            for (size_t i = maxOverlap; i < currentBoardColumn.size(); ++i) {
                                finalReel.push_back(currentBoardColumn[i]);
                            }
                        }
                    }
                    
                    outFile << "          \"stop\": " << finalReel.size() << ",\n          \"reel\": [";
                    for (size_t i = 0; i < finalReel.size(); ++i) {
                        if (i > 0) outFile << ", ";
                        outFile << finalReel[i];
                    }
                    outFile << "]\n        }";
                }
                outFile << "\n      ]\n    }";
            }
            outFile << "\n  ]";
        }
        
        outFile.close();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

// Test function using existing elimination data and json reel data
void testSymbolEliminationWithReelData() {
    std::cout << "\n=== SYMBOL ELIMINATION VALIDATION TEST ===\n";
    std::cout << "This test validates that the slot game mechanics are correctly balanced:\n";
    std::cout << "• For each script, we simulate the cascading slot game to count eliminated symbols\n";
    std::cout << "• We read the corresponding reel data from the smart conversion file\n";
    std::cout << "• We verify: Number of eliminated symbols = Number of symbols being added from reels\n";
    std::cout << "• Formula: Total Eliminated = Sum(all reel lengths) - 30\n";
    std::cout << "• This ensures perfect symbol conservation when board size stays constant at 30\n";
    std::cout << "• Each eliminated symbol is replaced by exactly one new symbol from the reels\n\n";
    
    try {
        // Get elimination data from the original config
        auto config = ScriptApp::ScriptConfig::loadFromFile("BG_hist/BG_333.json");
        
        // Read reel data from smart conversion file
        std::ifstream smartFile("BG_hist/BG_smart.json");
        if (!smartFile.is_open()) {
            throw std::runtime_error("Unable to open smart conversion file");
        }
        
        nlohmann::json smartJson;
        smartFile >> smartJson;
        smartFile.close();
        
        SlotSS02 game(true, 20.0f); // cascade enabled
        
        int testCount = static_cast<int>(config.base_scripts.size()); // Test ALL scripts
        int passedTests = 0;
        int failedTests = 0;
        
        std::cout << "Testing " << testCount << " scripts...\n";
        
        for (int testIdx = 0; testIdx < testCount; ++testIdx) {
            const auto& [index, scriptData] = *std::next(config.base_scripts.begin(), testIdx);
            
            try {
                // Get elimination data from game simulation
                auto [final_board, total_score, actual_stop, patterns, boards_match] = game.steps(scriptData.script, scriptData.special_multipliers);
                
                // Count total eliminated symbols
                int total_eliminated_symbols = 0;
                
                for (size_t board_idx = 0; board_idx < patterns.size(); ++board_idx) {
                    const auto& step_patterns = patterns[board_idx];
                    
                    for (const auto& [symbol, positions] : step_patterns) {
                        int count = positions.size();
                        total_eliminated_symbols += count;
                    }
                }
                
                // Find corresponding reel data in smart JSON
                int total_reel_symbols = 0;
                bool found_script = false;
                
                // Determine the section name in the smart JSON
                std::string smartSectionName = "base"; // default
                if (smartJson.contains("free") && !smartJson.contains("base")) {
                    smartSectionName = "free";
                }
                
                for (const auto& smart_script : smartJson[smartSectionName]) {
                    if (smart_script["number"].get<int>() == index) {
                        found_script = true;
                        // Sum all reel lengths for this script
                        for (const auto& reel_data : smart_script["script"]) {
                            total_reel_symbols += reel_data["reel"].size();
                        }
                        break;
                    }
                }
                
                if (!found_script) {
                    std::cout << "\n❌ Script " << index << " not found in smart conversion file\n";
                    failedTests++;
                    continue;
                }
                
                int expected_eliminated = total_reel_symbols - 30; // Total reel symbols - board size
                
                if (total_eliminated_symbols == expected_eliminated) {
                    passedTests++;
                } else {
                    std::cout << "\n❌ FAIL Script " << index << ":\n";
                    std::cout << "  Eliminated: " << total_eliminated_symbols << ", Expected: " << expected_eliminated;
                    std::cout << ", Difference: " << (total_eliminated_symbols - expected_eliminated) << "\n";
                    failedTests++;
                }
                
            } catch (const std::exception& e) {
                std::cout << "\n❌ ERROR Script " << index << ": " << e.what() << "\n";
                failedTests++;
            }
        }
        
        std::cout << "\n=== SYMBOL ELIMINATION VALIDATION RESULTS ===\n";
        std::cout << "• Total tests: " << testCount << " scripts\n";
        std::cout << "• Passed: " << passedTests << " scripts\n";
        std::cout << "• Failed: " << failedTests << " scripts\n";
        std::cout << "• Success rate: " << (testCount > 0 ? (passedTests * 100.0 / testCount) : 0) << "%\n";
        
        if (failedTests == 0) {
            std::cout << "\n✅ VALIDATION SUCCESSFUL: All scripts maintain perfect symbol conservation!\n";
            std::cout << "   This confirms that the slot game mechanics and reel conversion are correct.\n";
        } else {
            std::cout << "\n❌ VALIDATION ISSUES DETECTED: " << failedTests << " scripts failed validation.\n";
            std::cout << "   This indicates potential problems with game mechanics or reel conversion.\n";
        }
        std::cout << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in elimination vs reel test: " << e.what() << "\n";
    }
}

// Function to replace base and free content in Insert_Script.json
void replaceInsertScriptContent(const std::string& smartJsonFile, const std::string& insertScriptFile) {
    try {
        // Read the smart JSON file - it starts with "base": without outer braces
        std::ifstream smartFile(smartJsonFile);
        if (!smartFile.is_open()) {
            throw std::runtime_error("Cannot open " + smartJsonFile);
        }
        
        // Manually wrap the content in braces to make it valid JSON
        std::string smartContent((std::istreambuf_iterator<char>(smartFile)),
                                 std::istreambuf_iterator<char>());
        smartFile.close();
        
        // Add outer braces if not present
        std::string wrappedContent = "{" + smartContent + "}";
        nlohmann::json smartJson = nlohmann::json::parse(wrappedContent);
        
        // Read the Insert_Script.json file - PRESERVE THE ORIGINAL STRUCTURE
        std::ifstream insertFile(insertScriptFile);
        if (!insertFile.is_open()) {
            throw std::runtime_error("Cannot open " + insertScriptFile);
        }
        
        nlohmann::json insertJson;
        insertFile >> insertJson;
        insertFile.close();
        
        // Replace ONLY base and free content within data, preserve buy_free, multiplier_table, config, and everything else
        if (insertJson.find("data") != insertJson.end() && insertJson["data"].is_object()) {
            if (smartJson.find("base") != smartJson.end() && smartJson.find("free") != smartJson.end()) {
                // Save existing buy_free, multiplier_table, config, and any other fields
                nlohmann::json buyFreeBackup, multiplierTableBackup, configBackup;
                bool hasBuyFree = false, hasMultiplierTable = false, hasConfig = false;
                
                if (insertJson["data"].find("buy_free") != insertJson["data"].end()) {
                    buyFreeBackup = insertJson["data"]["buy_free"];
                    hasBuyFree = true;
                }
                
                if (insertJson["data"].find("multiplier_table") != insertJson["data"].end()) {
                    multiplierTableBackup = insertJson["data"]["multiplier_table"];
                    hasMultiplierTable = true;
                }
                
                if (insertJson["data"].find("config") != insertJson["data"].end()) {
                    configBackup = insertJson["data"]["config"];
                    hasConfig = true;
                }
                
                // Replace only base and free arrays
                insertJson["data"]["base"] = smartJson["base"];
                insertJson["data"]["free"] = smartJson["free"];
                
                // Restore buy_free, multiplier_table, and config
                if (hasBuyFree) {
                    insertJson["data"]["buy_free"] = buyFreeBackup;
                }
                if (hasMultiplierTable) {
                    insertJson["data"]["multiplier_table"] = multiplierTableBackup;
                }
                if (hasConfig) {
                    insertJson["data"]["config"] = configBackup;
                }
                
                // Write back to Insert_Script.json
                std::ofstream outFile(insertScriptFile);
                if (!outFile.is_open()) {
                    throw std::runtime_error("Cannot write to " + insertScriptFile);
                }
                
                // Write with proper formatting
                outFile << std::setw(2) << insertJson << std::endl;
                outFile.close();
                
                std::cout << "✅ Successfully replaced 'base' and 'free' content in " << insertScriptFile << "\n";
                std::cout << "   Base scripts: " << smartJson["base"].size() << "\n";
                std::cout << "   Free scripts: " << smartJson["free"].size() << "\n";
                if (hasBuyFree) std::cout << "   ✓ Preserved 'buy_free' section\n";
                if (hasMultiplierTable) std::cout << "   ✓ Preserved 'multiplier_table' section\n";
                if (hasConfig) std::cout << "   ✓ Preserved 'config' section\n";
            } else {
                throw std::runtime_error("Smart JSON does not have 'base' or 'free' fields");
            }
        } else {
            throw std::runtime_error("Insert_Script.json does not have 'data' field");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error in elimination vs reel test: " << e.what() << "\n";
    }
}

int main() {
    std::string inputFile = "SS02_scripts.json";
    
    std::cout << "Simple conversion...\n";
    convertJsonFormat(inputFile, "SS02_scripts_converted.json");

    std::cout << "Smart conversion...\n";
    convertJsonFormatAdvanced(inputFile, "SS02_scripts_smart.json");

    std::cout << "All conversions completed!\n";
    std::cout << "Output files:\n";
    std::cout << "  - SS02_scripts_converted.json (simple conversion)\n";
    std::cout << "  - SS02_scripts_smart.json (smart conversion with overlap detection)\n";
    return 0;
}
