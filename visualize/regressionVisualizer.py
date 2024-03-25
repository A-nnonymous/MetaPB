import pandas as pd
import matplotlib.pyplot as plt
import os

def plot_and_save_all_batches(operator, batch_folders):
    all_scatter_data = pd.DataFrame()
    all_regression_data = pd.DataFrame()

    for batch_dir in batch_folders:
        scatter_file = os.path.join(batch_dir, f'CPU_energy_Scatters_{operator}.csv')
        regression_file = os.path.join(batch_dir, f'CPU_energy_Regression_{operator}.csv')

        if os.path.exists(scatter_file) and os.path.exists(regression_file):
            scatter_df = pd.read_csv(scatter_file)
            regression_df = pd.read_csv(regression_file)
            all_scatter_data = pd.concat([all_scatter_data, scatter_df])
            
            # Group by dataSize_MiB and calculate average Time_Second
            regression_df = regression_df.groupby('dataSize_MiB', as_index=False)['Time_Second'].mean()
            all_regression_data = pd.concat([all_regression_data, regression_df])

    if not all_scatter_data.empty and not all_regression_data.empty:
        all_scatter_data = all_scatter_data.sort_values(by='dataSize_MiB')
        all_regression_data = all_regression_data.sort_values(by='dataSize_MiB')

        # Calculate residuals
        merged_df = pd.merge(all_scatter_data, all_regression_data, on='dataSize_MiB', suffixes=('_scatter', '_regression'))
        merged_df['residual'] = merged_df['Time_Second_scatter'] - merged_df['Time_Second_regression']

        plt.figure(figsize=(14, 7))

        # Scatter plot
        sAx = plt.subplot(1, 2, 1)
        plt.scatter(merged_df['dataSize_MiB'], merged_df['Time_Second_scatter'], label='Real Data')
        plt.plot(merged_df['dataSize_MiB'], merged_df['Time_Second_regression'], color='red', label='Fitted Data')
        plt.xlabel('Data Size (MiB)')
        plt.ylabel('Energy cost (Joule)')
        plt.title(f'CPU Time Cost and Regression for Operator {operator}')
        sAx.set_yscale('log')
        sAx.set_xscale('log')
        sAx.grid(True, which='major', linestyle='-', linewidth='0.5', color='black')
        plt.legend()

        # Residual plot
        rAx = plt.subplot(1, 2, 2)
        plt.scatter(merged_df['dataSize_MiB'], merged_df['residual'], color='green', label='Residual')
        plt.axhline(y=0, color='black', linestyle='--')
        plt.xlabel('Data Size (MiB)')
        plt.ylabel('Residual (Second)')
        plt.title(f'Residual Plot for {operator}')
        rAx.grid(True, which='major', linestyle='-', linewidth='0.5', color='black')
        plt.legend()

        plt.tight_layout()

        # Save figures
        plt.savefig("./CPU_Energy/"+f'{operator}_fit_and_residuals.png')

# List of folders and operators to process
batch_folders = ['batch_1024MiB', 'batch_256MiB', 'batch_2048MiB', 'batch_512MiB','batch_4096MiB']
operators = ['CONV_1D', 'DOT_ADD', 'DOT_PROD', 'EUDIST', 'MAC', 'MAP','REDUCE']  # Add all your operators here

for operator in operators:
    plot_and_save_all_batches(operator, batch_folders)