#ifndef SCRIPT_CONFIG_H
#define SCRIPT_CONFIG_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <map>
#include "json.hpp"
#include "SlotPay.hpp"

namespace ScriptApp {

// Main configuration class
struct ScriptData {
    using Script = std::vector<Board>;  // Vector of boards (each board represents a state)
    Script script;                      // The actual script data
    int stop = 0;                       // Stop value for this script
    int payout = 0;                     // Payout value
    int payout_id = 0;                  // Payout identifier
    int special_multipliers = 1;        // Special multiplier value (default 1 for no multiplier effect)
    int multiple_table = 0;             // Multiple table value (only for free section)
    bool is_free = false;               // Flag to indicate if this is from free section
};

struct ScriptConfig {
    std::map<int, ScriptData> base_scripts;  // Map of index to base script data
    std::map<int, ScriptData> free_scripts;  // Map of index to free script data
    

    static ScriptConfig loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open script configuration file: " + filename);
        }

        nlohmann::json j;
        file >> j;
        
        ScriptConfig config;
        
        // Check if this is SS02_scripts.json format (has "result" wrapper) or old format
        nlohmann::json data = j;
        if (j.contains("result")) {
            // SS02_scripts.json format - data is nested under "result"
            data = j.at("result");
        }
        
        // Process base entries
        if (data.contains("base")) {
            for (const auto& base : data.at("base")) {
                int index = base.at("index").get<int>();
                
                ScriptData scriptData;
                scriptData.script = base.at("script").get<ScriptData::Script>();
                scriptData.stop = base.at("stop").get<int>();
                scriptData.is_free = false;
                
                // Read payout information
                if (base.contains("payout") && !base.at("payout").is_null()) {
                    scriptData.payout = base.at("payout").get<int>();
                }
                if (base.contains("payout_id") && !base.at("payout_id").is_null()) {
                    scriptData.payout_id = base.at("payout_id").get<int>();
                }
                if (base.contains("special_multipliers") && !base.at("special_multipliers").is_null()) {
                    scriptData.special_multipliers = base.at("special_multipliers").get<int>();
                }
                
                config.base_scripts[index] = scriptData;
            }
        }
        
        // Process free entries
        if (data.contains("free")) {
            for (const auto& free : data.at("free")) {
                int index = free.at("index").get<int>();
                
                ScriptData scriptData;
                scriptData.script = free.at("script").get<ScriptData::Script>();
                scriptData.stop = free.at("stop").get<int>();
                scriptData.is_free = true;
                
                // Read payout information
                if (free.contains("payout") && !free.at("payout").is_null()) {
                    scriptData.payout = free.at("payout").get<int>();
                }
                if (free.contains("payout_id") && !free.at("payout_id").is_null()) {
                    scriptData.payout_id = free.at("payout_id").get<int>();
                }
                if (free.contains("special_multipliers") && !free.at("special_multipliers").is_null()) {
                    scriptData.special_multipliers = free.at("special_multipliers").get<int>();
                }
                
                // Set multiple_table to 1 if special_multipliers is 3 or 20
                if (scriptData.special_multipliers == 1 || scriptData.special_multipliers == 20 || scriptData.special_multipliers == 40) {
                    scriptData.multiple_table = 1;
                }
                
                config.free_scripts[index] = scriptData;
            }
        }
        
        return config;
    }

    // Helper function to get base script data by index
    const ScriptData& pick_single_base_script(int index) const {
        auto it = base_scripts.find(index);
        if (it == base_scripts.end()) {
            throw std::runtime_error("Base script index not found: " + std::to_string(index));
        }
        return it->second;
    }
    
    // Helper function to get free script data by index
    const ScriptData& pick_single_free_script(int index) const {
        auto it = free_scripts.find(index);
        if (it == free_scripts.end()) {
            throw std::runtime_error("Free script index not found: " + std::to_string(index));
        }
        return it->second;
    }
};

} // namespace ScriptApp

#endif // SCRIPT_CONFIG_H
