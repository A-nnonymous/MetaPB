import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

# Check if the required arguments are provided
if len(sys.argv) != 4:
    print("Usage: python script.py <path_to_csv_file> <task_name> <output_path>")
    sys.exit(1)

csv_file_path = sys.argv[1]
task_name = sys.argv[2]
output_path = sys.argv[3]

# Ensure the output directory exists, create if it does not
if not os.path.exists(output_path):
    os.makedirs(output_path)

# Read the CSV file into a pandas DataFrame
df = pd.read_csv(csv_file_path)

# Calculate the duration for each task
df['Duration'] = df['End(ms)'] - df['Start(ms)']

# Find the minimum start time to set as zero
min_start_time = df['Start(ms)'].min()
df['Start(ms)'] -= min_start_time

# Create a figure and an axis for the plot
fig, ax = plt.subplots(figsize=(10, 6))

# List of unique workers
workers = df['Worker'].unique().tolist()

# Improved colors for each OpType
colors = {
    'ComputeBound': '#1f77b4',  # muted blue
    'MemoryBound': '#ff7f0e',   # safety orange
    'MAP': '#2ca02c',           # cooked asparagus green
    'REDUCE': '#d62728',        # brick red
    'Logical': '#9467bd',       # muted purple
    'Undefined': 'black'
}

# Plot each task as a bar in the Gantt chart
for i, worker in enumerate(workers):
    # Filter the DataFrame for the current worker
    worker_df = df[df['Worker'] == worker]

    # Group by OpType
    for op_type, group_df in worker_df.groupby('OpType'):
        ax.broken_barh([(start, duration) for start, duration in zip(group_df['Start(ms)'], group_df['Duration'])],
                       (i - 0.4, 0.8), facecolors=colors[op_type], edgecolor='black', label=op_type)

# Set the y-axis labels to show worker names
ax.set_yticks(range(len(workers)))
ax.set_yticklabels(workers)

# Set the x-axis to start from 0
ax.set_xlim(left=0)

# Add labels and title
plt.xlabel('Time (ms)')
plt.ylabel('Worker')
plt.title(f'Gantt Chart of {task_name} Execution by Worker and OpType')

# Add a legend
handles, labels = plt.gca().get_legend_handles_labels()
by_label = dict(zip(labels, handles))  # Remove duplicates
# Add a legend and place it below the x-axis
legend_num_col = len(by_label)  # Number of columns in legend
plt.legend(by_label.values(), by_label.keys(), loc='upper center', bbox_to_anchor=(0.5, -0.1), ncol=legend_num_col)

# Save the figure as SVG and PNG
output_svg = os.path.join(output_path, f'{task_name}_gantt_chart.svg')
output_png = os.path.join(output_path, f'{task_name}_gantt_chart.png')
plt.savefig(output_svg, format='svg', bbox_inches='tight')
plt.savefig(output_png, format='png', bbox_inches='tight')

# Show the plot
plt.tight_layout()
plt.show()