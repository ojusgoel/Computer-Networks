import sys
import os
import subprocess

def generate_files(input_file):
    # Define different bandwidth and latency configurations
    configs = [
        {"DataRate": "5Mbps", "Delay": "2ms"},
        {"DataRate": "5Mbps", "Delay": "5ms"},
        {"DataRate": "5Mbps", "Delay": "10ms"},
        {"DataRate": "5Mbps", "Delay": "15ms"},
        {"DataRate": "5Mbps", "Delay": "20ms"}
    ]
    
    # Extract the base name of the input file (without the directory or extension)
    base_name = os.path.splitext(os.path.basename(input_file))[0]
    
    # Read the original .cc file content
    with open(input_file, 'r') as file:
        code = file.read()
    
    # Process and generate new files with modified DataRate and Delay values
    for i, config in enumerate(configs, start=1):
        # Create unique identifiers for DataRate and Delay
        data_rate = config["DataRate"]
        delay = config["Delay"]
        
        # Update the DataRate and Delay values in the code
        modified_code = code
        modified_code = modified_code.replace(
            'pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));', 
            f'pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("{data_rate}"));'
        )
        modified_code = modified_code.replace(
            'pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));', 
            f'pointToPoint.SetChannelAttribute ("Delay", StringValue ("{delay}"));'
        )
        
        # Replace the EnablePcapAll parameter with a unique name like file_1, file_2, etc.
        pcap_filename = f"file_{i}"
        modified_code = modified_code.replace(
            'pointToPoint.EnablePcapAll ("Vegas_file");',
            f'pointToPoint.EnablePcapAll ("{pcap_filename}");'
        )
        
        # Define a new file name for each configuration
        output_file_name = f"{base_name}_{data_rate}_{delay}.cc"
        output_file_path = os.path.join("scratch", output_file_name)
        
        # Write the modified code to the new file in the scratch directory
        with open(output_file_path, 'w') as new_file:
            new_file.write(modified_code)
        
        print(f"Generated file: {output_file_path}")

        # Run the .cc file with ns-3 to generate the corresponding .pcap files
        try:
            print(f"Running ns-3 simulation for {output_file_path}...")
            subprocess.run(["./ns3", "run", f"scratch/{output_file_name}"], check=True)
            print(f"Successfully generated pcap for {pcap_filename}.pcap")
        except subprocess.CalledProcessError:
            print(f"Error running simulation for {output_file_path}")
            continue

# Check if a file path is provided
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python generate_files.py <input_file>")
    else:
        input_file = sys.argv[1]
        generate_files(input_file)

