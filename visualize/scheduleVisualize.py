import pandas as pd
import matplotlib.pyplot as plt
import argparse

# Set up argument parsing
parser = argparse.ArgumentParser(description='Visualize DPU Loads Schedule')
parser.add_argument('input_file', type=str, help='Path to the input CSV file containing the schedule')
parser.add_argument('output_file', type=str, help='Path to the output image file for the visualization')
parser.add_argument('schedule_name', type=str, help='Name of the schedule to set as the image title')

# Parse the arguments
args = parser.parse_args()

# Read the schedule csv file into a DataFrame
df = pd.read_csv(args.input_file)

# Normalize the data to the range [0, 1] for visualization
normalized_df = df.clip(0, 1)

# Define the texture patterns for the offloaded and local parts of the bars
# More dense pattern for offloaded part (DPU), less dense for local part (CPU)
patterns = ('////', '....')

# Create a bar plot
fig, ax = plt.subplots()

# Plot each load
for i, column in enumerate(normalized_df.columns):
    # Offloaded part with a more dense pattern (DPU)
    ax.bar(i, normalized_df[column][0], hatch=patterns[0], edgecolor='black', color='white', label='DPU (offloadRatio)' if i == 0 else "")
    # Local part with a less dense pattern (CPU)
    ax.bar(i, 1 - normalized_df[column][0], bottom=normalized_df[column][0], hatch=patterns[1], edgecolor='black', color='white', label='CPU (1-offloadRatio)' if i == 0 else "")

# Set the labels
ax.set_xlabel('Loads')
ax.set_ylabel('Offload Ratio')

# Set the title using the provided schedule name
ax.set_title("Offload Ratios scheduled by "+ args.schedule_name)

# Set the x-axis ticks to show the load names
ax.set_xticks(range(len(normalized_df.columns)))
ax.set_xticklabels(normalized_df.columns)

# Add legend to the plot, positioned at the bottom center in a single row (ncol=2)
ax.legend(loc='upper center', bbox_to_anchor=(0.5, -0.15), shadow=True, ncol=2)

# Adjust layout to prevent clipping of ylabel
plt.tight_layout()

# Save the plot to the specified output file
plt.savefig(args.output_file)
plt.close()