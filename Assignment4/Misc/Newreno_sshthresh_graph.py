import matplotlib.pyplot as plt

# Given data
x = [1.01256, 1.42897, 1.69707, 10.8846]
y1 = [0, 4294967295, 26532, 12864]
y2 = [4294967295, 26532, 12864, 17420]

# Filter out high values
filtered_y1 = [y for y in y1 if y < 100000]  # adjust the threshold as necessary
filtered_y2 = [y for y in y2 if y < 100000]  # adjust the threshold as necessary
filtered_x = x[:len(filtered_y1)]  # match the length of filtered data

# Create the plot
plt.figure(figsize=(10, 6))
plt.plot(filtered_x, filtered_y1, label='Y1 Values', marker='o')
plt.plot(filtered_x, filtered_y2, label='Y2 Values', marker='o')

# Add titles and labels
plt.title('Filtered SSHthresh Graph')
plt.xlabel('X Values')
plt.ylabel('Y Values')
plt.legend()
plt.grid()

# Show the plot
plt.show()

