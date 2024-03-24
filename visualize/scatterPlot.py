import pandas as pd
import matplotlib.pyplot as plt
import argparse

# Set up argument parsing
parser = argparse.ArgumentParser(description='Visualize High-dimensional Space Points')
parser.add_argument('total_file', type=str, help='Path to the CSV file containing all points')
parser.add_argument('converge_file', type=str, help='Path to the CSV file containing converge points')
parser.add_argument('title', type=str, help='Title for the plot')
parser.add_argument('output_file', type=str, help='Path to the output image file for the visualization')

# Parse the arguments
args = parser.parse_args()

# Read the total and converge csv files into DataFrames
df_total = pd.read_csv(args.total_file)
df_converge = pd.read_csv(args.converge_file)

dim1 = 0
dim2 = 1
# Get the column headers for the last two dimensions
dim_x = df_total.columns[dim1]
dim_y = df_total.columns[dim2]

# Select the last two columns (dimensions) for plotting
x_total = df_total.iloc[:, dim1]
y_total = df_total.iloc[:, dim2]
x_converge = df_converge.iloc[:, dim1]
y_converge = df_converge.iloc[:, dim2]

# Create a scatter plot for the total points
plt.scatter(x_total, y_total, s=1, label='Total Points')  # s is the size of the points

# Create a scatter plot for the converge points
plt.scatter(x_converge, y_converge, color='red', s=10, label='Converge Points')  # s is the size of the points

# Connect the converge points with lines
plt.plot(x_converge, y_converge, color='red')

# Add labels and title
plt.xlabel(dim_x)
plt.ylabel(dim_y)
plt.title(args.title)

# Add a legend
plt.legend(loc='upper center', bbox_to_anchor=(0.5, -0.15), shadow=True, ncol=2)

# Adjust layout to prevent clipping of ylabel
plt.tight_layout()

# Save the plot to the specified output file
plt.savefig(args.output_file)
plt.savefig(args.output_file + ".svg", format='svg',bbox_inches='tight')
plt.close()