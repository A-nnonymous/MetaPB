import pandas as pd
import matplotlib.pyplot as plt

# Read the CSV file into a pandas DataFrame
df = pd.read_csv('timings.csv')

# Calculate the duration for each task
df['Duration'] = df['End(ms)'] - df['Start(ms)']

# Find the minimum start time to set as zero
min_start_time = df['Start(ms)'].min()
df['Start(ms)'] -= min_start_time

# Create a figure and an axis for the plot
fig, ax = plt.subplots(figsize=(10, 6))

# List of unique task types
task_types = df['Type'].unique().tolist()

# Colors for each task type
colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red']

# Plot each task as a bar in the Gantt chart
for i, task_type in enumerate(task_types):
    # Filter the DataFrame for the current task type
    task_df = df[df['Type'] == task_type]

    # Plot the tasks
    ax.broken_barh([(start, duration) for start, duration in zip(task_df['Start(ms)'], task_df['Duration'])],
                   (i - 0.4, 0.8), facecolors=colors[i], edgecolor='black', label=task_type)

# Set the y-axis labels to show task types
ax.set_yticks(range(len(task_types)))
ax.set_yticklabels(task_types)

# Set the x-axis to start from 0
ax.set_xlim(left=0)

# Add labels and title
plt.xlabel('Time (ms)')
plt.ylabel('Task Type')
plt.title('Gantt Chart of Task Execution')

# Add a legend
plt.legend(title='Task Types')

# Save the figure as SVG and PNG
plt.savefig('gantt_chart.svg', format='svg')
plt.savefig('gantt_chart.png', format='png')

# Show the plot
plt.tight_layout()
plt.show()
