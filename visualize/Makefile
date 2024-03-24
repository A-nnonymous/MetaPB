pfBest=PSO
efBest=PSO
hyBest=PSO
outputPath=../dataOutput/images/metaVisual/
string_visualize:
	python3 scatterPlot.py /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_EnergyFirst_${efBest}_totalPtHist.csv  /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_EnergyFirst_${efBest}_convergePtFrm.csv "MetaPB_EnergyFirst" ${outputPath}EFScatter
	python3 scatterPlot.py /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_Hybrid_${hyBest}_totalPtHist.csv  /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_Hybrid_${hyBest}_convergePtFrm.csv "MetaPB_Hybrid" ${outputPath}HYScatter
	python3 scatterPlot.py /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_PerfFirst_${pfBest}_totalPtHist.csv  /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_PerfFirst_${pfBest}_convergePtFrm.csv "MetaPB_PerfFirst" ${outputPath}PFScatter
	python3 scheduleVisualize.py /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_Hybrid_schedule.csv  ${outputPath}HYSchedule MetaPB_Hybrid
	python3 scheduleVisualize.py /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_PerfFirst_schedule.csv  ${outputPath}PFSchedule MetaPB_PerfFirst
	python3 scheduleVisualize.py /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_EnergyFirst_schedule.csv  ${outputPath}EFSchedule MetaPB_EnergyFirst
	python3 convergePlot.py  /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_Hybrid_${hyBest}_totalValHist.csv /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_Hybrid_${hyBest}_convergeValFrm.csv "MetaPB_Hybrid" ${outputPath}HYconverge
	python3 convergePlot.py  /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_PerfFirst_${pfBest}_totalValHist.csv /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_PerfFirst_${pfBest}_convergeValFrm.csv "MetaPB_PerfFirst" ${outputPath}PFconverge
	python3 convergePlot.py  /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_EnergyFirst_${efBest}_totalValHist.csv /output/MetaPB_Results/string_workload/4096MiB_10_MetaPB_EnergyFirst_${efBest}_convergeValFrm.csv "MetaPB_EnergyFirst" ${outputPath}EFconverge
