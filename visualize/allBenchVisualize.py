import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Load the CSV file from the provided URL
url = "./4096MiB_wholeSome.csv"
data = pd.read_csv(url)

# Calculate performance and energy efficiency
data['performance'] = 4 / data['timeConsume_Seconds']  # loadSize_GiB/s
data['energy_efficiency'] = 4 / data['energyConsume_Joules']  # loadSize_GiB/Joule

# Normalize the data based on CPUOnly strategy
cpuonly_performance = data[data['scheduler_name'] == 'CPUOnly'].set_index('workload_name')['performance']
cpuonly_efficiency = data[data['scheduler_name'] == 'CPUOnly'].set_index('workload_name')['energy_efficiency']

# Apply normalization
data['performance_relative'] = data.apply(lambda row: row['performance'] / cpuonly_performance[row['workload_name']], axis=1)
data['energy_efficiency_relative'] = data.apply(lambda row: row['energy_efficiency'] / cpuonly_efficiency[row['workload_name']], axis=1)

# Get unique loads and strategies
loads = data['workload_name'].unique()
strategies = data['scheduler_name'].unique()
patterns = ['////', '\\\\', '||||', '----', '++++', 'xxxx', 'oooo', '....']
# Widen the figure
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(15, 10), sharex=True)

# Define bar width and patterns for differentiation
bar_width = 0.1  # Reduced bar width
cluster_spacing = 0.3  # Increase space between clusters

# Set the positions of the bars on the x-axis
bar_positions = np.arange(len(loads)) * (bar_width * len(strategies) + cluster_spacing)

# Plot bars for each strategy
for i, strategy in enumerate(strategies):
    # Select data for the current strategy
    strategy_data = data[data['scheduler_name'] == strategy]
    # Calculate bar positions
    strategy_positions = bar_positions + (i * bar_width)
    # Plot performance bars
    ax1.bar(strategy_positions, strategy_data['performance_relative'], width=bar_width, label=strategy, hatch=patterns[i % len(patterns)], edgecolor='black', color='white')
    # Plot energy efficiency bars (without adding to legend)
    ax2.bar(strategy_positions, strategy_data['energy_efficiency_relative'], width=bar_width, hatch=patterns[i % len(patterns)], edgecolor='black', color='white')

# Set the labels and titles
ax1.set_ylabel('Relative Performance (Normalized to CPUOnly)')
ax1.set_title('Performance of Different Strategies by Different Workloads')
ax2.set_ylabel('Relative Energy Efficiency (Normalized to CPUOnly)')
ax2.set_xlabel('Load')

# Set the x-axis tick labels
ax2.set_xticks(bar_positions + bar_width * (len(strategies) - 1) / 2)
ax2.set_xticklabels(loads, rotation=45)  # Rotate labels if they overlap
ax2.invert_yaxis()
# Add the legend at the bottom in a single row
fig.legend(title='Strategy', loc='lower center', bbox_to_anchor=(0.5, 0.02), ncol=len(strategies))

# Adjust layout to make space for the legend
fig.subplots_adjust(bottom=0.15, hspace=0.05)  # Adjust the bottom to provide space for the legend

# Save the plot to files with high resolution
plt.savefig("./test.png", dpi=300, bbox_inches='tight')
plt.savefig("./test.svg", format='svg', dpi=300, bbox_inches='tight')

# Show the plot
plt.show()