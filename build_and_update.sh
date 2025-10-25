#!/bin/bash

# Build and Update Script
# This script compiles SS02_test, runs SS02_convertpay, and updates Insert_Script.json
# Integrated with build.sh functionality

set -e  # Exit on error

echo "=========================================="
echo "Starting Build and Update Process"
echo "=========================================="
echo ""

# Show current files
echo "Files in directory:"
ls -la *.cpp *.hpp *.py 2>/dev/null || true
echo ""

# Step 1: Compile and run SS02_test
echo "Step 1: Compiling and running SS02_test..."
echo "------------------------------------------"

# Clean previous build
if [ -f "SS02_test" ]; then
    echo "Removing previous SS02_test executable..."
    rm SS02_test
fi

echo "Running: g++ -std=c++17 -Wall -Wextra -o SS02_test SlotPay.cpp SS02Pay.cpp SS02_test.cpp"
g++ -std=c++17 -Wall -Wextra -o SS02_test SlotPay.cpp SS02Pay.cpp SS02_test.cpp

if [ $? -eq 0 ] && [ -f "SS02_test" ]; then
    echo "✅ SS02_test compiled successfully"
    echo ""
    echo "Running SS02_test..."
    ./SS02_test
    echo ""
    echo "✅ SS02_test completed"
else
    echo "❌ SS02_test compilation failed"
    echo "Trying with verbose output to see errors:"
    g++ -std=c++17 -Wall -Wextra -v -o SS02_test SlotPay.cpp SS02Pay.cpp SS02_test.cpp
    exit 1
fi
echo ""

# Step 2: Compile and run SS02_convertpay
echo "Step 2: Compiling and running SS02_convertpay..."
echo "------------------------------------------"

# Clean previous build
if [ -f "SS02_convertpay" ]; then
    echo "Removing previous SS02_convertpay executable..."
    rm SS02_convertpay
fi

echo "Running: g++ -std=c++17 -Wall -Wextra -o SS02_convertpay SlotPay.cpp SS02Pay.cpp SS02_Convertpay.cpp"
g++ -std=c++17 -Wall -Wextra -o SS02_convertpay SlotPay.cpp SS02Pay.cpp SS02_Convertpay.cpp

if [ $? -eq 0 ] && [ -f "SS02_convertpay" ]; then
    echo "✅ SS02_convertpay compiled successfully"
    echo ""
    echo "Running SS02_convertpay..."
    ./SS02_convertpay
    echo ""
    echo "✅ SS02_convertpay completed"
else
    echo "❌ SS02_convertpay compilation failed"
    echo "Trying with verbose output to see errors:"
    g++ -std=c++17 -Wall -Wextra -v -o SS02_convertpay SlotPay.cpp SS02Pay.cpp SS02_Convertpay.cpp
    exit 1
fi
echo ""

# Step 3: Replace base and free scripts in Insert_Script.json
echo "Step 3: Replacing base and free scripts in Insert_Script.json..."
echo "------------------------------------------"

if [ ! -f "replace_base_free.py" ]; then
    echo "❌ replace_base_free.py not found!"
    exit 1
fi

python3 replace_base_free.py

# Step 4: Clean up temporary files
echo "Step 4: Cleaning up temporary files..."
echo "------------------------------------------"

if [ -f "SS02_scripts_converted.json" ]; then
    rm SS02_scripts_converted.json
    echo "✅ Removed SS02_scripts_converted.json"
fi

if [ -f "SS02_scripts_smart.json" ]; then
    rm SS02_scripts_smart.json
    echo "✅ Removed SS02_scripts_smart.json"
fi

if [ -f "SS02_multiplier_table.json" ]; then
    rm SS02_multiplier_table.json
    echo "✅ Removed SS02_multiplier_table.json (integrated into Insert_Script.json)"
fi

if [ -f "SS02_mystery_trigger.json" ]; then
    rm SS02_mystery_trigger.json
    echo "✅ Removed SS02_mystery_trigger.json (integrated into Insert_Script.json)"
fi
echo ""

echo "=========================================="
echo "✅ All tasks completed successfully!"
echo "=========================================="
echo ""
echo "Generated files:"
echo "  - SS02_test (executable)"
echo "  - SS02_convertpay (executable)"
echo "  - script_results.json (from SS02_test)"
echo "  - FG_hist/Insert_Script.json (updated with base, free, multiplier_table, and config)"
echo ""
echo "Temporary files cleaned up:"
echo "  - SS02_scripts_converted.json (removed)"
echo "  - SS02_scripts_smart.json (removed)"
echo "  - SS02_multiplier_table.json (removed - integrated into Insert_Script.json)"
echo "  - SS02_mystery_trigger.json (removed - integrated into Insert_Script.json config)"
echo ""
echo "Run executables with:"
echo "  ./SS02_test"
echo "  ./SS02_convertpay"
