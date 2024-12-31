import matplotlib.pyplot as plt

# Sample data (time, initial ssthresh, updated ssthresh)
data = [
    (1.01256, 0, 0),
    (1.42897, 0, 26532),
    (1.69707, 26532, 12864),
    (10.8846, 12864, 17420),
]

# Extract time and updated ssthresh values
time = [row[0] for row in data]
ssthresh_values = [row[2] for row in data]  # Plotting only the updated values

# Plotting
plt.figure(figsize=(10, 6))
plt.plot(time, ssthresh_values, marker='o', linestyle='-', color='b', label="ssthresh")

plt.xlabel("Time (seconds)")
plt.ylabel("ssthresh value")
plt.title("ssthresh Evolution Over Time")
plt.legend()
plt.grid(True)
plt.show()

