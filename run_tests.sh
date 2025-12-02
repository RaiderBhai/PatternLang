#!/bin/bash

# Step 1: Compile the compiler
echo "Compiling compiler..."
g++ src/*.cpp -o compiler -std=c++17

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo "Compilation done."
echo "----------------------------------------"

# Step 2: Run each test file
for file in tests/test*.pat; do
    echo "Running $file"
    echo "----------------------------------------"
    
    ./compiler "$file"
    
    echo "----------------------------------------"
    echo
done