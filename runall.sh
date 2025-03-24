#!/bin/bash
# filepath: run_all_tests.sh

# Set up working directory
cd "$(dirname "$0")"
BUILD_DIR="./build"
OUTPUT_DIR="./results"

# Create results directory if it doesn't exist
mkdir -p $OUTPUT_DIR

# Set date for unique filename suffix
DATE_SUFFIX=$(date +"%Y%m%d_%H%M%S")

echo "===== RUNNING PVHSS-RH TESTS ====="
echo "Starting tests at $(date)"
echo "Results will be saved in $OUTPUT_DIR/"

# Function to run a subproject and capture output
run_test() {
    local project_name=$1
    local output_file="$OUTPUT_DIR/${project_name}_${DATE_SUFFIX}.txt"
    
    echo "Running $project_name..."
    echo "==== $project_name Test Results - $(date) ====" > $output_file
    
    # Run the executable and redirect output
    if [ -f "$BUILD_DIR/$project_name/$project_name" ]; then
        $BUILD_DIR/$project_name/$project_name >> $output_file 2>&1
        echo "Test completed. Results saved to $output_file"
    else
        echo "Error: Executable $BUILD_DIR/$project_name/$project_name not found!" | tee -a $output_file
    fi
    
    echo "---------------------------------"
}

# Run build if needed
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Building project..."
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    cmake ..
    make -j$(nproc)
    cd ..
fi

# Run each subproject in order
echo "Running all subprojects in sequence..."
#run_test "GroupPi1"
#run_test "GroupPi2" 
#run_test "GroupHSS"
# run_test "RLWEHSS"
# run_test "RLWEPi1"
# run_test "RLWEPi2"
run_test "CZ23"

echo "===== ALL TESTS COMPLETED ====="
echo "Test results are saved in $OUTPUT_DIR/"

# Generate a summary file
summary_file="$OUTPUT_DIR/summary_${DATE_SUFFIX}.txt"
echo "===== PVHSS-RH Test Summary - $(date) =====" > $summary_file
echo "" >> $summary_file

for project in common GroupPi1 GroupPi2 GroupHSS RLWEHSS RLWEPi1 RLWEPi2; do
    result_file="$OUTPUT_DIR/${project}_${DATE_SUFFIX}.txt"
    if [ -f "$result_file" ]; then
        echo "== $project Summary ==" >> $summary_file
        # Extract timing information and other key metrics
        grep "time\|Time\|TEST\|test\|Result" $result_file | head -n 10 >> $summary_file
        echo "" >> $summary_file
    fi
done

echo "Summary generated at $summary_file"