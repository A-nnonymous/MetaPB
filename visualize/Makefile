pfBest=PSO
efBest=RSA
hyBest=PSO
DataSize=2048
stringLen=6
outputPath=../dataOutput/images/metaVisual/
string_visualize:
	mkdir -p ${outputPath}${DataSize}
	python3 scatterPlot.py /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_${efBest}_totalPtHist.csv  /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_${efBest}_convergePtFrm.csv "MetaPB_EnergyFirst" ${outputPath}${DataSize}/EFScatter
	python3 scatterPlot.py /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_Hybrid_${hyBest}_totalPtHist.csv  /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_Hybrid_${hyBest}_convergePtFrm.csv "MetaPB_Hybrid" ${outputPath}${DataSize}/HYScatter
	python3 scatterPlot.py /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_${pfBest}_totalPtHist.csv  /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_${pfBest}_convergePtFrm.csv "MetaPB_PerfFirst" ${outputPath}${DataSize}/PFScatter
	python3 scheduleVisualize.py /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_Hybrid_schedule.csv  ${outputPath}HYSchedule MetaPB_Hybrid
	python3 scheduleVisualize.py /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_schedule.csv  ${outputPath}PFSchedule MetaPB_PerfFirst
	python3 scheduleVisualize.py /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_schedule.csv  ${outputPath}EFSchedule MetaPB_EnergyFirst
	python3 convergePlot.py  /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_Hybrid_${hyBest}_totalValHist.csv /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_Hybrid_${hyBest}_convergeValFrm.csv "MetaPB_Hybrid" ${outputPath}${DataSize}/HYconverge
	python3 convergePlot.py  /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_${pfBest}_totalValHist.csv /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_${pfBest}_convergeValFrm.csv "MetaPB_PerfFirst" ${outputPath}${DataSize}/PFconverge
	python3 convergePlot.py  /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_${efBest}_totalValHist.csv /output/MetaPB_Results/string_workload/${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_${efBest}_convergeValFrm.csv "MetaPB_EnergyFirst" ${outputPath}${DataSize}/EFconverge
