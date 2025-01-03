#!/bin/bash

# Check if the input file is provided
if [ -z "$1" ]; then
  echo "Usage: $0 <input_file.cc>"
  exit 1
fi

INPUT_FILE="$(pwd)/$1"

# Run the Python script to generate .cc files with different DataRate and Delay
python3 q2_4.py "$INPUT_FILE"

# Array of different .pcap files generated by the Python script
pcap_files=("file_1-1-0.pcap" "file_2-1-0.pcap" "file_3-1-0.pcap" "file_4-1-0.pcap" "file_5-1-0.pcap")
datarates=("5Mbps" "5Mbps" "5Mbps" "5Mbps" "5Mbps")  # Example datarates used in Python script
delays=("2ms" "4ms" "6ms" "8ms" "10ms")                  # Example delays used in Python script

# Header for output table
echo "=============================================="
echo "| DataRate | Delay | Throughput (Bytes/sec)   |"
echo "=============================================="

# Loop through each pcap file to calculate throughput
for i in "${!pcap_files[@]}"; do
  pcap_file="${pcap_files[i]}"
  
  # Run tshark to extract the bytes sent from the pcap file
  #output=$(tshark -r "$pcap_file" -q -z io,stat,0,"ip.src==10.1.1.1")
  
  # Store tshark output in a variable
  output=$(tshark -r "$pcap_file"  -qz io,stat,19)

  # Parse the bytes value dynamically from the output
  bytes=$(echo "$output" | grep -oP "(?<=\| )\d+(?= \| *$)")

  echo "Parsed Bytes: $bytes"

  
  # Calculate throughput (Bytes/sec)
  duration=19.0  # Fixed duration as per instruction
  throughput=$(echo "scale=2; $bytes / $duration" | bc)
  
  # Print DataRate, Delay, and Throughput in the table format
  echo "| ${datarates[i]} | ${delays[i]} | $throughput Bytes/sec |"
done

echo "=============================================="

