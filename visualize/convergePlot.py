import pandas as pd
import matplotlib.pyplot as plt
import sys

def create_plots(total_val_file, converge_val_file, experiment_name, output_filename):
    # Read the 'totalVal' csv file into a DataFrame
    total_df = pd.read_csv(total_val_file)

    # Create a line plot for each column in the total dataframe
    for column in total_df.columns:
        plt.plot(total_df.index, total_df[column])

    # Read the 'convergeVal' csv file into a DataFrame
    converge_df = pd.read_csv(converge_val_file)

    # Create a line plot for the converge values, with emphasis
    converge_line, = plt.plot(converge_df.index, converge_df.iloc[:, 0], label='Converge Value', linewidth=2.5, color='red', zorder=5)

    # Add labels and title
    plt.xlabel('Iteration')
    plt.ylabel('Value')
    plt.title(f'{experiment_name} - Value Change Over Iterations')
    plt.yscale('log')

    # Adjust the layout to make space for the legend at the bottom
    plt.tight_layout(rect=[0, 0.05, 1, 1])

    # Add a legend only for the converge line and place it at the bottom
    plt.legend(handles=[converge_line], loc='lower center', bbox_to_anchor=(0.5, -0.22), ncol=1)

    # Save the plot to files with high resolution
    plt.savefig(f"{output_filename}.png", dpi=300)
    plt.savefig(f"{output_filename}.svg", format='svg', bbox_inches='tight', dpi=300)

    # Show the plot
    plt.show()

if __name__ == "__main__":
    # Check if the correct number of arguments are passed
    if len(sys.argv) != 5:
        print("Usage: python plot_script.py <total_val_file> <converge_val_file> <experiment_name> <output_filename>")
        sys.exit(1)

    # Assign arguments to variables
    total_val_filepath = sys.argv[1]
    converge_val_filepath = sys.argv[2]
    experiment_name = sys.argv[3]
    output_filename = sys.argv[4]

    # Call the function with the provided arguments
    create_plots(total_val_filepath, converge_val_filepath, experiment_name, output_filename)