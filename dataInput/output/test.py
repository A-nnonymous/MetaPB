import os
import pandas as pd
import matplotlib.pyplot as plt

# Specify the directory structure
directories = ['./batch_1024MiB', './batch_128MiB', './batch_2048MiB', './batch_256MiB', './batch_4096MiB', './batch_512MiB']

# Initialize a dictionary to store all data
data_summary = {
    'CPU': {'Energy': {}, 'Perf': {}},
    'DPU': {'Perf': {}}
}

# Traverse directories and files
for directory in directories:
    for filename in os.listdir(directory):
        if 'Scatters' in filename:
            # Extract device, metric, and operator information
            parts = filename.replace('.csv', '').split('_')
            device = parts[0]  # CPU or DPU
            metric_type = parts[1]  # Energy or Perf
            metric = 'Energy' if metric_type == 'energy' else 'Perf'
            operator = '_'.join(parts[3:])  # Join the remaining parts for the operator name
            
            # Build the complete file path
            file_path = os.path.join(directory, filename)
            # Read the CSV file
            data = pd.read_csv(file_path)
            # Ensure data size is an integer
            data['dataSize_MiB'] = data['dataSize_MiB'].astype(int)
            
            # Add data to the summary dictionary
            if operator not in data_summary[device][metric]:
                data_summary[device][metric][operator] = []
            data_summary[device][metric][operator].append(data)

# Prepare the plot
fig, ax = plt.subplots(figsize=(14, 8))

# Plot performance lines
lines = []
labels = []
for device in ['CPU', 'DPU']:
    for operator, data_list in data_summary[device]['Perf'].items():
        # Concatenate data for the same operator
        data = pd.concat(data_list)
        data = data.groupby('dataSize_MiB').mean().reset_index()
        if not data.empty:
            line, = ax.plot(data['dataSize_MiB'], data['Time_Second'], marker='o', markersize=3, label=operator.replace('_', ' '))
            lines.append(line)
            labels.append(operator.replace('CONV_1D', '1D'))  # Replace CONV_1D with 1D in the label

# Sort the legend, checking if lines have data
if lines:
    legend = sorted(zip(lines, labels), key=lambda x: x[0].get_ydata(orig=True)[-1], reverse=True)
    lines_sorted, labels_sorted = zip(*legend)
    ax.legend(lines_sorted, labels_sorted)

# Set log scale for axes
ax.set_xscale('log')
ax.set_yscale('log')
ax.set_xlabel('Data Size (MiB)')
ax.set_ylabel('Time (Second)')
ax.set_title('CPU and DPU Performance Comparison')
ax.grid(True)

# Save the performance line plot
perf_image_filename = 'CPU_DPU_Performance_Comparison_log_scale.png'
plt.savefig(perf_image_filename, bbox_inches='tight')
plt.close()

# Repeat similar steps for plotting energy consumption...