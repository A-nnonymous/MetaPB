import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

def plot_performance_energy(data_file_w, data_file_wo, output_path):
    # 读取CSV文件
    w_reduce_data = pd.read_csv(data_file_w)
    wo_reduce_data = pd.read_csv(data_file_wo)

    # 计算性能（GiB/s）
    w_reduce_data['performance'] = 4 / w_reduce_data['timeConsume_Seconds']
    wo_reduce_data['performance'] = 4 / wo_reduce_data['timeConsume_Seconds']

    # 获取运算符名称列表
    operators = pd.concat([w_reduce_data['OperatorName'], wo_reduce_data['OperatorName']]).unique()

    # 创建子图
    fig, axs = plt.subplots(1, len(operators), figsize=(5 * len(operators), 5))
    #fig, axs = plt.subplots(2, 3, figsize=(3  , 2))

    # 如果只有一个运算符，将axs转换为列表
    if len(operators) == 1:
        axs = [axs]

    # 存储所有的图例句柄和标签
    handles_list = []
    labels_list = []

    # 对于每个运算符绘制子图
    for ax, operator in zip(axs, operators):
        op_w_reduce = w_reduce_data[w_reduce_data['OperatorName'] == operator]
        op_wo_reduce = wo_reduce_data[wo_reduce_data['OperatorName'] == operator]
        
        # 为了确保柱状图不会重叠，需要为它们设置不同的x位置
        bar_width = 0.04  # 柱子的宽度
        # 绘制性能柱状图，w和wo的柱子分开显示
        bars_w = ax.bar(op_w_reduce['offloadRatio'] - bar_width / 2, op_w_reduce['performance'], width=bar_width, label='Overall Performance With Reduce', align='center')
        bars_wo = ax.bar(op_wo_reduce['offloadRatio'] + bar_width / 2, op_wo_reduce['performance'], width=bar_width, label='Overall Performance Without Reduce', align='center')

        ax_twin = ax.twinx()
        # 绘制能耗折线图
        line_w, = ax_twin.plot(op_w_reduce['offloadRatio'], op_w_reduce['energyConsume_Joules'], 'r-', label='Total Energy cost With Reduce')
        line_wo, = ax_twin.plot(op_wo_reduce['offloadRatio'], op_wo_reduce['energyConsume_Joules'], 'g-', label='Total Energy cost Without Reduce')
        ax_twin.set_ylabel('Energy (Joules)')
        # 添加当前子图的图例句柄和标签到列表
        handles, labels = ax.get_legend_handles_labels()
        handles_list.extend(handles)
        labels_list.extend(labels)
        handles, labels = ax_twin.get_legend_handles_labels()
        handles_list.extend(handles)
        labels_list.extend(labels)
        ax.set_yscale('log')
        ax.grid(True, which='major', linestyle='-', linewidth='0.5', color='black')
        # 设置图表标题和标签
        ax.set_title(operator)
        ax.set_xlabel('OffloadRatio')
        ax.set_ylabel('Performance (GiB/s)')
        ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f'{y:.2f}'))
        
    
    handles, labels = [], []
    for handle, label in zip(handles_list, labels_list):
        if label not in labels:  # 检查是否已经添加了该图例标签
            handles.append(handle)
            labels.append(label)
    # 绘制统一的图例
    fig.legend(handles, labels, loc='lower center', ncol=len(handles_list) // 2, bbox_to_anchor=(0.5, 0.05))

    # 调整布局
    plt.tight_layout(rect=[0, 0.1, 1, 1])  # 为图例留出空间

    # 保存图表为SVG和PNG格式

    # 保存图表为SVG和PNG格式
    plt.savefig(f'{output_path}/performance_energy_plot.svg', format='svg')
    plt.savefig(f'{output_path}/performance_energy_plot.png', format='png')

    # 显示图表
    plt.show()

# 从命令行参数获取文件位置和输出图片位置
if __name__ == '__main__':
    data_file_w = './4096MiB_w_reduce_perf.csv'  # 替换为实际的文件路径
    data_file_wo = './4096MiB_wo_reduce_perf.csv'  # 替换为实际的文件路径
    output_path = './'  # 替换为实际的输出路径

    plot_performance_energy(data_file_w, data_file_wo, output_path)