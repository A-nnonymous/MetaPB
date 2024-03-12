import numpy as np
import matplotlib.pyplot as plt

# plot all operators' time & energy scaling corresponding to input_size
# Shape: in  1x1 single diagram
def plot_operator_metrics(operator_names, operator_input_size, 
                          operators_energy_costs, operators_time_costs):
    pass

# plot regression results of all operators time&energy scaling data(with cross validation),
# Shape: In len(operator_names) x 2 subplots (regression, bias of both time and energy)
def plot_operator_regressions(operator_names, operator_input_size, 
                              operators_energy_costs, operators_time_costs,
                              deduced_operator_energy_costs, deduced_operator_time_costs):
    pass

# plot converge history of all points for all DAG, in all weight
# Shape: In len(loads_name) * length(weight_names)
def plot_value_converge_histories(loads_names, weight_names, points_value_histories):
    pass

# plot sliced 2D converge scatter-point for all DAG, in all weight
# Shape: In len(loads_name) * length(weight_names) subplots 
def plot_sliced_scattered_history(loads_names, weight_names, points_place_histories,
                                  x_jobIdx, y_jobIdx):
    pass

# Actual sheduling of all strategies
# Shape: In len(strategies) * len(loads)
def plot_optimization_results(strategies, loads, vector_lengths, export_path=None):

    fig, axs = plt.subplots(len(strategies), len(loads), figsize=(20, 15))

    # three metaheuristic algorithm
    colors = ['blue', 'green', 'red']

    for i, strategy in enumerate(strategies):
        for j, load in enumerate(loads):
            # result vector
            vector_length = vector_lengths[j]
            best_result = np.random.rand(vector_length)
            color = np.random.choice(colors)

            # bar blot yielding
            axs[i, j].bar(range(vector_length), best_result, color=color)
            axs[i, j].set_ylim(0, 1)  # scheduling vec item is between 0 and 1
            axs[i, j].set_title(f'{strategy} - {load}')
            axs[i, j].set_xticks([])  # Hide x ticks

    # y&x axis tags
    fig.text(0.04, 0.5, 'Optimization Results', va='center', rotation='vertical', fontsize=12)
    fig.text(0.5, 0.04, 'Vector Elements', ha='center', fontsize=12)

    # adjust layout
    plt.tight_layout()

    # save figure
    if export_path:
        plt.savefig(f"{export_path}/optimization_results.png", bbox_inches='tight')
    plt.show()

# Final summary of all works, compare MetaPB with all baselines
# Shape: In a single complex barplot
def plot_performance_energy(loads, performance, energy, strategies, export_path=None):
    n_loads = len(loads)
    n_strategies = len(strategies)
    index = np.arange(n_loads)
    bar_width = 0.1
    
    # Texture filling
    hatches = ['////', '....', '||||', '----', '++++', 'xxxx']

    # Subplot arrangement
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10), sharex=True)

    # Barplot of performance(related)
    for i, method in enumerate(strategies):
        ax1.bar(index + i * bar_width, performance[:, i], bar_width, label=method, hatch=hatches[i], color='white', edgecolor='black')

    ax1.set_ylabel('Related Performance (Higher is better)')
    ax1.set_title('Relative Performance and Energy Efficiency Comparison')
    ax1.tick_params(labelbottom=False)  # hide upper x label

    # Mirror barplot of energy efficiency(Related)
    for i, method in enumerate(strategies):
        ax2.bar(index + i * bar_width, energy[:, i], bar_width, hatch=hatches[i], color='white', edgecolor='black')

    ax2.set_ylabel('Related Energy Efficiency(Higher is better)')
    ax2.set_xticks(index + bar_width * (n_strategies - 1) / 2)
    ax2.set_xticklabels(loads)
    ax2.invert_yaxis()  # inverse y axis

    # add legends
    handles, labels = ax1.get_legend_handles_labels()
    fig.legend(handles, labels, loc='upper center', bbox_to_anchor=(0.5, -0.05), fancybox=True, shadow=True, ncol=n_strategies)

    # adjust subplot placement
    fig.subplots_adjust(hspace=0, bottom=0)

    if export_path:
        plt.savefig(f"{export_path}/performance_energy_comparison.png", bbox_inches='tight')
    plt.show()

if __name__ == "__main__":
    dataPath = "../dataOutput/"
    opData = dataPath + "OPMetrics.csv"
    vamData= dataPath + "VAM.csv"
    benchmarkData = dataPath + "wholeBenchmark.csv"
    # Operator stage:
    [op_name, op_input_size,
     op_timecost, op_energycost, 
     deduced_op_timecost,deduced_op_energycost] = \
    loadOPCSV(opData)

    plot_operator_metrics(op_name, op_input_size, op_energycost, op_timecost)
    plot_operator_regressions(op_name,op_input_size, 
                              op_energycost,op_timecost,
                              deduced_op_energycost, deduced_op_timecost)
    # Workload stage:
    ## Simple workload(VAM)
    [load_names, strategies,
     performances, energies,
     metapb_wgt_names, metapb_pt_place_hist, metapb_pt_val_hist] =\
    loadWorkloadCSV(vamData)

    ## benchmarking workloads(Random generated DAG from 4OP and 2TOPO)
    [load_names, strategies,
     performances, energies,
     metapb_wgt_names, metapb_pt_place_hist, metapb_pt_val_hist] =\
   loadWorkloadCSV(benchmarkData) 
    
    loads = ['Load1', 'Load2', 'Load3', 'Load4', 'Load5']
    vector_lengths = [10, 10, 10, 10, 10]  # scheduling vector length
    strategies = ['NOT_OFFLOADED', 'ALL_OFFLOADED', 'HEFT', 
                  'MetaPB_Time_Sensitive','MetaPB_Hybrid' , 'MetaPB_Energy_Sensitive']
    performance = np.random.rand(len(loads), len(strategies))
    energy = np.random.rand(len(loads), len(strategies))

    plot_performance_energy(loads, performance, energy, strategies, export_path='.')
    plot_optimization_results(strategies, loads, vector_lengths, export_path='.')