import matplotlib.pyplot as plt
import sys
import os

def plot_rtt(filename):
    time = []
    rtt_values = []

    # Read data from the specified file
    with open(filename, 'r') as file:
        for line in file:
            columns = line.split()
            time.append(float(columns[0]))  # Time in seconds
            rtt_values.append(float(columns[2]))  # New RTT in seconds

    # Extract the base name for the plot title
    base_name = os.path.splitext(os.path.basename(filename))[0].split('_')[0]

    # Plotting RTT vs Time
    plt.figure(figsize=(10, 5))
    plt.plot(time, rtt_values, label='RTT', color='blue')
    plt.xlabel('Time (s)')
    plt.ylabel('RTT (s)')
    plt.title(f'RTT vs Time for TCP {base_name}')
    plt.legend()
    plt.grid()

    # Save the plot as a .png file
    output_file = os.path.splitext(filename)[0] + ".png"
    plt.savefig(output_file)
    print(f"Plot saved as {output_file}")
    plt.close()

# Check if a file path is provided
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python plot_rtt.py <file_path>")
    else:
        filename = sys.argv[1]
        plot_rtt(filename)
