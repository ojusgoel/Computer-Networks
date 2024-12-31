import re
import sys

def count_cwnd_reductions(filename):
    # Pattern to match the timestamp and cwnd values
    cwnd_pattern = re.compile(r"(\d+\.\d+)\s+(\d+)\s+(\d+)")
    reductions = 0
    prev_cwnd = None
    reasons = []

    with open(filename, 'r') as file:
        for line in file:
            match = cwnd_pattern.match(line)
            if match:
                timestamp, old_cwnd, new_cwnd = float(match.group(1)), int(match.group(2)), int(match.group(3))

                # Check if there was a reduction
                if prev_cwnd is not None and new_cwnd < prev_cwnd:
                    reductions += 1
                    reason = f"At time {timestamp:.2f}s, cwnd reduced from {prev_cwnd} to {new_cwnd}."
                    
                    # Explanation for reduction
                    if new_cwnd < old_cwnd:
                        reason += " Likely due to a packet loss event, which triggers congestion control."
                    else:
                        reason += " Possibly due to slow-start threshold adjustment."
                    
                    reasons.append(reason)

                prev_cwnd = new_cwnd  # Update prev_cwnd for the next iteration

    return reductions, reasons

# Check if a file path is provided
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python count_cwnd_reductions.py <file_path.cwnd>")
    else:
        filename = sys.argv[1]
        reductions, reasons = count_cwnd_reductions(filename)
        print(f"Total number of cwnd reductions: {reductions}")
