import matplotlib.pyplot as plt
import sys
import os

def plot_cwnd(file_path):
    times = []
    congestion_windows = []

    # Read data from the .cwnd file
    with open(file_path, 'r') as file:
        for line in file:
            if line.strip():  # Ensure line is not empty
                time, _, updated_cwnd = map(float, line.split())
                times.append(time)
                congestion_windows.append(updated_cwnd)
    
    # Plotting
    plt.figure(figsize=(10, 6))
    plt.plot(times, congestion_windows, linestyle='-', color='b', label="Congestion Window")
    plt.title("Congestion Window vs Time")
    plt.xlabel("Time")
    plt.ylabel("Congestion Window Size")
    plt.legend()
    plt.grid(True)
    
    # Save the plot as a .png file
    output_file = os.path.splitext(file_path)[0] + ".png"
    plt.savefig(output_file)
    print(f"Graph saved as {output_file}")
    plt.close()

# Check if a file path is provided
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python plot_cwnd.py <file_path.cwnd>")
    else:
        file_path = sys.argv[1]
        plot_cwnd(file_path)

