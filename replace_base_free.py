#!/usr/bin/env python3
"""
Replace 'base' and 'free' sections in Insert_Script.json with content from SS02_scripts_smart.json
while preserving the exact formatting of all other sections (buy_free, multiplier_table, config).

Also replaces 'multiplier_table' with the version exported from SS02_ExportMultiplierTable if available.
"""

import re
import os
import json

def find_json_section(content, section_name, start_pos=0):
    """Find a JSON section by name and return its start and end positions."""
    # Find the section start
    pattern = rf'"{section_name}"\s*:\s*'
    match = re.search(pattern, content[start_pos:])
    if not match:
        return None, None
    
    section_start = start_pos + match.start()
    value_start = start_pos + match.end()
    
    # Determine if it's an array or object
    while content[value_start].isspace():
        value_start += 1
    
    if content[value_start] == '[':
        # Find matching closing bracket
        bracket_count = 0
        pos = value_start
        while pos < len(content):
            if content[pos] == '[':
                bracket_count += 1
            elif content[pos] == ']':
                bracket_count -= 1
                if bracket_count == 0:
                    return section_start, pos + 1
            pos += 1
    elif content[value_start] == '{':
        # Find matching closing brace
        brace_count = 0
        pos = value_start
        while pos < len(content):
            if content[pos] == '{':
                brace_count += 1
            elif content[pos] == '}':
                brace_count -= 1
                if brace_count == 0:
                    return section_start, pos + 1
            pos += 1
    
    return None, None

print("Reading SS02_scripts_smart.json...")
with open('SS02_scripts_smart.json', 'r') as f:
    smart_content = f.read()

print("Reading FG_hist/Insert_Script.json...")
with open('FG_hist/Insert_Script.json', 'r') as f:
    insert_content = f.read()

# Try to read multiplier table from exported file if it exists
multiplier_table_content = None
if os.path.exists('SS02_multiplier_table.json'):
    print("Reading SS02_multiplier_table.json...")
    try:
        with open('SS02_multiplier_table.json', 'r') as f:
            multiplier_table_json = json.load(f)
            # Store just the table content (not wrapped in key)
            multiplier_table_content = json.dumps(multiplier_table_json, indent=2)
    except (json.JSONDecodeError, IOError) as e:
        print("⚠️  Warning: Could not parse SS02_multiplier_table.json, will use existing table")
        multiplier_table_content = None
else:
    print("⚠️  SS02_multiplier_table.json not found, will preserve existing multiplier_table")

# Try to read mystery trigger from exported file if it exists
mystery_trigger_value = None
if os.path.exists('SS02_mystery_trigger.json'):
    print("Reading SS02_mystery_trigger.json...")
    try:
        with open('SS02_mystery_trigger.json', 'r') as f:
            mystery_trigger_json = json.load(f)
            mystery_trigger_value = mystery_trigger_json.get('double_chance_rate')
    except (json.JSONDecodeError, IOError) as e:
        print("⚠️  Warning: Could not parse SS02_mystery_trigger.json, will use existing value")
        mystery_trigger_value = None
else:
    print("⚠️  SS02_mystery_trigger.json not found, will preserve existing double_chance_rate")

# Extract base and free sections from SS02_scripts_smart.json
print("Extracting 'base' section from SS02_scripts_smart.json...")
base_start, base_end = find_json_section(smart_content, 'base')
if base_start is None:
    raise Exception("Could not find 'base' section in SS02_scripts_smart.json")
smart_base = smart_content[base_start:base_end]

print("Extracting 'free' section from SS02_scripts_smart.json...")
free_start, free_end = find_json_section(smart_content, 'free')
if free_start is None:
    raise Exception("Could not find 'free' section in SS02_scripts_smart.json")
smart_free = smart_content[free_start:free_end]

# Find the data section in Insert_Script.json
print("Finding 'data' section in Insert_Script.json...")
data_pattern = r'"data"\s*:\s*\{'
data_match = re.search(data_pattern, insert_content)
if not data_match:
    raise Exception("Could not find 'data' section in Insert_Script.json")

data_start = data_match.end()

# Find where data section ends
brace_count = 1
data_end = data_start
while data_end < len(insert_content) and brace_count > 0:
    if insert_content[data_end] == '{':
        brace_count += 1
    elif insert_content[data_end] == '}':
        brace_count -= 1
    data_end += 1

# Find old base and free sections in Insert_Script.json
old_base_start, old_base_end = find_json_section(insert_content, 'base', data_start)
old_free_start, old_free_end = find_json_section(insert_content, 'free', data_start)
old_mult_table_start, old_mult_table_end = find_json_section(insert_content, 'multiplier_table', data_start)

if old_base_start is None or old_free_start is None:
    raise Exception("Could not find 'base' or 'free' in Insert_Script.json")

# Build the new content
print("Reconstructing Insert_Script.json...")
# Everything before data section
new_content = insert_content[:data_match.end()]

# Add proper indentation and newline
new_content += '\n    '

# Determine the order and reconstruct
sections = [
    ('base', smart_base, old_base_start),
    ('free', smart_free, old_free_start),
]

# Find buy_free, multiplier_table, config positions
buy_free_start, buy_free_end = find_json_section(insert_content, 'buy_free', data_start)
mult_table_start, mult_table_end = find_json_section(insert_content, 'multiplier_table', data_start)
config_start, config_end = find_json_section(insert_content, 'config', data_start)

if buy_free_start:
    sections.append(('buy_free', insert_content[buy_free_start:buy_free_end], buy_free_start))

# Use new multiplier_table if available, otherwise use original
if multiplier_table_content:
    print("✅ Using new multiplier_table from SS02_multiplier_table.json")
    # Reconstruct the full multiplier_table section with proper formatting
    mult_table_section = '"multiplier_table": ' + multiplier_table_content
    sections.append(('multiplier_table', mult_table_section, mult_table_start))
elif mult_table_start:
    print("ℹ️  Preserving original multiplier_table")
    sections.append(('multiplier_table', insert_content[mult_table_start:mult_table_end], mult_table_start))

# Handle config section - update double_chance_rate if mystery trigger is available
if config_start:
    config_section = insert_content[config_start:config_end]
    
    if mystery_trigger_value is not None:
        print(f"✅ Updating double_chance_rate to {mystery_trigger_value}")
        # Parse the config section to update double_chance_rate
        config_value_start = insert_content.find(':', config_start) + 1
        config_value_end = config_end
        
        # Extract and parse the config JSON
        config_json_str = insert_content[config_value_start:config_value_end].strip()
        try:
            config_json = json.loads(config_json_str)
            config_json['double_chance_rate'] = mystery_trigger_value
            
            # Custom JSON serialization to preserve 4 decimal places for double_chance_rate
            config_json_formatted = json.dumps(config_json, indent=2)
            # Replace the double_chance_rate value with 4 decimal places format
            import re
            pattern = r'"double_chance_rate"\s*:\s*[\d\.]+'
            replacement = f'"double_chance_rate" : {mystery_trigger_value:.4f}'
            config_json_formatted = re.sub(pattern, replacement, config_json_formatted)
            
            # Reconstruct config section
            config_section = '"config" : ' + config_json_formatted
        except json.JSONDecodeError:
            print("⚠️  Warning: Could not parse config section, preserving original")
    else:
        print("ℹ️  Preserving original config section")
    
    sections.append(('config', config_section, config_start))

# Sort by original position to maintain order
sections.sort(key=lambda x: x[2])

# Add sections with proper separators
for i, (name, content, pos) in enumerate(sections):
    new_content += content
    if i < len(sections) - 1:
        new_content += ','
    new_content += '\n    '

# Remove the last added newline and spaces, add closing
new_content = new_content.rstrip()
new_content += '\n  }\n}'

print("Writing to FG_hist/Insert_Script.json...")
with open('FG_hist/Insert_Script.json', 'w') as f:
    f.write(new_content)

print("\n✅ Successfully replaced 'base' and 'free' in Insert_Script.json")
if multiplier_table_content:
    print("✅ Multiplier table updated from SS02_multiplier_table.json")
else:
    print("ℹ️  Multiplier table preserved from original")
if mystery_trigger_value is not None:
    print(f"✅ Config double_chance_rate updated to {mystery_trigger_value}")
else:
    print("ℹ️  Config section preserved from original")
print("   All other sections (buy_free) preserved with original formatting")
print("   Reel arrays maintain their original compact format")
