# SS02 Slot Machine Simulator

This project contains tools for testing and converting SS02 slot machine game scripts.

## Overview

The SS02 simulator implements a 6x5 board slot machine with cascading mechanics and minimum 8-match requirements. The project includes two main programs:

1. **SS02_test** - Tests and analyzes slot machine scripts
2. **SS02_Convertpay** - Converts script formats for different use cases

## Programs

### SS02_test.cpp

**Purpose**: Analyzes slot machine scripts to validate game mechanics and calculate payouts.

**Features**:
- Validates script execution against expected results
- Calculates payout statistics (average, variance, standard deviation)
- Analyzes base game and free game scripts separately
- Detects mismatches in payouts, stop counts, and cascading behavior
- Computes Antebet RTP and mystery trigger probability
- Exports volatility-based multiplier tables
- Exports mystery trigger configuration
- Exports detailed results to JSON format


**Input**: Reads from `SS02_scripts.json`

**Output**:
- `SS02_scripts_converted.json` - Simple conversion
- `SS02_scripts_smart.json` - Smart conversion with overlap optimization

**Conversion Process**:
1. **Load Scripts**: Reads input JSON with base and free game scripts
2. **Simple Conversion**: Direct format transformation
3. **Smart Conversion**: Analyzes board overlaps and eliminates duplicate entries created during cascading mechanics
   - Detects when elimination patterns create identical board sequences
   - Removes redundant intermediate states that occur during symbol elimination
   - Compresses cascading sequences by identifying common subsequences between board states
   - Prevents duplicate entries from symbol elimination and gravity application cycles
4. **Validation**: Ensures converted scripts maintain game logic integrity

### replace_base_free.py

**Purpose**: Integrates processed slot machine scripts into the backend-compatible format.

**Features**:
- **Backend Integration**: Pastes processed free and base scripts into the format required by the backend system


**Input**: 
- `SS02_scripts_smart.json` - Smart converted scripts
- `FG_hist/Insert_Script.json` - Backend template format

**Output**: 
- `FG_hist/Insert_Script.json` - Updated with processed base/free scripts, multiplier table, and config values

**Integration Process**:
1. **Read Converted Scripts**: Loads optimized scripts from SS02_scripts_smart.json
2. **Load Multiplier Table**: Reads multiplier table from SS02_multiplier_table.json (if available)
3. **Load Mystery Trigger**: Reads mystery trigger probability from SS02_mystery_trigger.json (if available)
4. **Extract Sections**: Separates base game and free game script sections
5. **Load Template**: Reads the backend-compatible template structure
6. **Merge Content**: Integrates processed scripts, multiplier table, and config into template
7. **Output Generation**: Creates final backend-ready JSON file with preserved structure

## Build System

### Automated Build

Use the provided build script for complete workflow:

```bash
./build_and_update.sh
```

This script:
This script:
1. Compiles and runs SS02_test to analyze scripts and export multiplier tables
2. Compiles and runs SS02_convertpay to optimize scripts
3. Runs replace_base_free.py to integrate scripts, multiplier tables, and mystery trigger into backend format
4. Cleans up temporary files (SS02_scripts_converted.json, SS02_scripts_smart.json, SS02_multiplier_table.json, SS02_mystery_trigger.json)

### Mystery Trigger Calculation

**Purpose**: Calculates the mystery trigger probability (double_chance_rate) for Antebet feature.

**Formula**:
```
mystryTrigger = 1 - (1 - 1/expectedPullsToFG) / (1 - fgTriggerProb)
```

Where:
- `expectedPullsToFG` = averageFeatureValue / antebetFreeRTP
- `averageFeatureValue` = freeCalculatedAvg × expectedFGLength / 30.0
- `antebetFreeRTP` = (overallCalculatedAvg × 1.5 - baseCalculatedAvg) / 30.0

**Output**: 
- `SS02_mystery_trigger.json` - Contains computed double_chance_rate with 4 decimal places
- Automatically integrated into Insert_Script.json config section

**Features**:



**Current Behavior**:
- SS02_test detects current volatility type (high/low)
- Exports appropriate multiplier table to SS02_multiplier_table.json
- Integrated into Insert_Script.json by replace_base_free.py

**Volatility Types**:
- **high**: More extreme multiplier distributions (higher variance)
- **low**: More balanced multiplier distributions (lower variance)


## Game Mechanics

### SS02 Slot Machine Rules

- **Board Size**: 6x5 grid
- **Matching**: Minimum 8 connected symbols required for payout
- **Cascading**: Winning symbols are eliminated, remaining symbols fall down
- **Symbol Types**: Various symbols (0-8) with different payout values

See `SS02Pay.cpp` for complete payout table.

## Configuration

Key configuration files:
- `ScriptConfig.h` - Defines input file paths and game parameters
- `GameConfig.json` - Game-specific settings
- `SS02Pay.hpp` - Game mechanics and payout definitions

## Troubleshooting


### File Format Requirements

Input JSON should contain:
```json
{
  "data": {
    "base": [...],
    "free": [...]
  }
}
```

Each script entry requires:
- `index`: Script identifier
- `stop`: Expected cascade stop count
- `script`: Array of board states (6x5 arrays)


## Output Analysis

### Script Results Format

The `script_results.json` contains:
- Individual script analysis results
- Payout comparisons (expected vs calculated)
- Error detection (mismatches, validation failures)
- Statistical summaries

### Performance Metrics

Key metrics to monitor:
- **Average Payout**: Expected return per spin
- **Variance**: Payout volatility measure
---

For detailed implementation information, see the source code comments in each `.cpp` file.
