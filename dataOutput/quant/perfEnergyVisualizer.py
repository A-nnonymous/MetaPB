import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter, LogLocator

# Specify the directory structure
directories = ['./batch_1024MiB', './batch_128MiB', './batch_2048MiB', './batch_256MiB', './batch_4096MiB', './batch_512MiB']

# Initialize a dictionary to store all data
data_summary = {
    'CPU': {'Energy': {}, 'Perf': {}},
    'DPU': {'Energy': {}, 'Perf': {}}
}

# Energy conversion factor for DPU
dpu_energy_conversion = 280

# Traverse directories and files
for directory in directories:
    for filename in os.listdir(directory):
        if 'Scatters' in filename:
            parts = filename.replace('.csv', '').split('_')
            device = parts[0]
            metric = 'Energy' if 'energy' in filename.lower() else 'Perf'
            operator = '_'.join(parts[3:])
            
            file_path = os.path.join(directory, filename)
            data = pd.read_csv(file_path)
            data['dataSize_MiB'] = data['dataSize_MiB'].astype(int)
            
            if device == 'DPU' and 'Perf' in metric:
                data['Energy_Joule'] = data['Time_Second'] * dpu_energy_conversion
                metric = 'Energy'
                if operator not in data_summary[device][metric]:
                    data_summary[device][metric][operator] = []
                data_summary[device][metric][operator].append(data)
                metric = 'Perf'
                if operator not in data_summary[device][metric]:
                    data_summary[device][metric][operator] = []
                data_summary[device][metric][operator].append(data)
                continue
            
            if device == 'CPU' and 'Energy' in metric:
                data.rename(columns={'Time_Second': 'Energy_Joule'}, inplace=True)
            
            if operator not in data_summary[device][metric]:
                data_summary[device][metric][operator] = []
            data_summary[device][metric][operator].append(data)

def x_format_func(value, tick_number):
    # Find the number of digits in the value
    num_digits = int(np.floor(np.log10(value))) + 1
    if num_digits <= 3:
        return '{:.0}'.format(value)
    else:
        return '{:.0}'.format(value)
# Function to format y-axis ticks with decimal places
def format_ticks(value, pos):
    if value < 1:
        return '{:.2f}'.format(value)
    elif value < 10:
        return '{:.1f}'.format(value)
    else:
        return '{:,.0f}'.format(value)

# Function to plot data, sort legend, and refine axis ticks
def plot_data(ax, data_summary, device_metric, ylabel, title, y_formatter):
    lines = []
    labels = []
    for device, metrics in data_summary.items():
        for operator, data_list in metrics[device_metric].items():
            data = pd.concat(data_list)
            data = data.groupby('dataSize_MiB').mean().reset_index()
            if not data.empty:
                operator_label = f"{device}_{operator}"
                line, = ax.plot(data['dataSize_MiB'], data[ylabel], marker='o', markersize=3, label=operator_label)
                lines.append(line)
                labels.append(operator_label)

    if lines:
        legend_items = sorted(zip(lines, labels), key=lambda x: x[0].get_ydata()[-1], reverse=True)
        lines_sorted, labels_sorted = zip(*legend_items)
        ax.legend(lines_sorted, labels_sorted)
    
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Data Size (MiB)')
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(True, which='both', linestyle='-', linewidth='0.5', color='black')

    # Set the formatter for the y-axis ticks
    ax.yaxis.set_major_formatter(FuncFormatter(y_formatter))

    # Set the locator for the x-axis ticks
    ax.xaxis.set_major_locator(LogLocator(base=2, numticks=15))
    ax.xaxis.set_minor_locator(LogLocator(base=2, subs=np.arange(2, 10) * 0.1, numticks=15))
    ax.xaxis.set_major_formatter(FuncFormatter(x_format_func))

    # Set the locator for the y-axis ticks
    ax.yaxis.set_major_locator(LogLocator(base=10))
    ax.yaxis.set_minor_locator(LogLocator(base=10, subs='auto', numticks=15))

    # Set the formatter for the y-axis ticks
    ax.yaxis.set_major_formatter(FuncFormatter(y_formatter))

    # Define a function to format the x-axis ticks

    # Set the formatter for the x-axis ticks
    ax.xaxis.set_major_formatter(FuncFormatter(x_format_func))

    # Set the locator for the x-axis ticks
    ax.xaxis.set_minor_locator(LogLocator(base=2, subs=np.arange(1.0, 10.0) * 0.1, numticks=5))

    # Set the locator for the y-axis ticks
    ax.yaxis.set_minor_locator(LogLocator(base=10, subs=np.arange(1.0, 10.0) * 0.1, numticks=5))

# Plot performance comparison
fig, ax1 = plt.subplots(figsize=(14, 8))
plot_data(ax1, data_summary, 'Perf', 'Time_Second', 'CPU and DPU Performance Comparison', format_ticks)
performance_image_filename = 'CPU_DPU_Performance_Comparison_log_scale.png'
plt.savefig(performance_image_filename, bbox_inches='tight')
plt.close()

# Plot energy consumption comparison
fig, ax2 = plt.subplots(figsize=(14, 8))
plot_data(ax2, data_summary, 'Energy', 'Energy_Joule', 'CPU and DPU Energy Consumption Comparison', lambda x, pos: '{:,.0f}'.format(x))
energy_image_filename = 'CPU_DPU_Energy_Consumption_Comparison_log_scale.png'
plt.savefig(energy_image_filename, bbox_inches='tight')
plt.close()