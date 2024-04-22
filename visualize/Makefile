pfBest=PSO
efBest=PSO
hyBest=PSO
DataSize=2048
stringLen=6
dataFilePath=/output/MetaPB_Results/string_workload/
outputPath=../dataOutput/images/metaVisual/
string_visualize:
	mkdir -p ${outputPath}${DataSize}
	python3 scatterPlot.py ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_${efBest}_totalPtHist.csv  ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_${efBest}_convergePtFrm.csv "MetaPB_EnergyFirst" ${outputPath}${DataSize}/EFScatter
	python3 scatterPlot.py ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_Hybrid_${hyBest}_totalPtHist.csv  ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_Hybrid_${hyBest}_convergePtFrm.csv "MetaPB_Hybrid" ${outputPath}${DataSize}/HYScatter
	python3 scatterPlot.py ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_${pfBest}_totalPtHist.csv  ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_${pfBest}_convergePtFrm.csv "MetaPB_PerfFirst" ${outputPath}${DataSize}/PFScatter
	python3 scheduleVisualize.py ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_Hybrid_schedule.csv  ${outputPath}${DataSize}/HYSchedule MetaPB_Hybrid
	python3 scheduleVisualize.py ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_schedule.csv  ${outputPath}${DataSize}/PFSchedule MetaPB_PerfFirst
	python3 scheduleVisualize.py ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_schedule.csv  ${outputPath}${DataSize}/EFSchedule MetaPB_EnergyFirst
	python3 convergePlot.py  ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_Hybrid_${hyBest}_totalValHist.csv ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_Hybrid_${hyBest}_convergeValFrm.csv "MetaPB_Hybrid" ${outputPath}${DataSize}/HYconverge
	python3 convergePlot.py  ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_${pfBest}_totalValHist.csv ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_PerfFirst_${pfBest}_convergeValFrm.csv "MetaPB_PerfFirst" ${outputPath}${DataSize}/PFconverge
	python3 convergePlot.py  ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_${efBest}_totalValHist.csv ${dataFilePath}${DataSize}MiB_${stringLen}_MetaPB_EnergyFirst_${efBest}_convergeValFrm.csv "MetaPB_EnergyFirst" ${outputPath}${DataSize}/EFconverge
