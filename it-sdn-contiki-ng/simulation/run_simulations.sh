#!/bin/bash
#set -e
#set -x
#set -v

# Simulation set configuration
MIN_ITER=1
MAX_ITER=2
# MIN_ITER=1
# MAX_ITER=2
COOJA_INSTANCES=2 #max simulations running in parallel
COOJA_CURRENT_INSTANCE=1

DO_NOT_OVERWRITE=true

nodes_v=(25 16 9)
nodes_v=(100 81 64 49 36 25 16 169 256)
nodes_v=(16 25 36 49 64 81 100)
nodes_v=(49)
# topologies=(GRID-FULL GRID-RND GRID-CTA GRID-SPN)
# topologies=(BERLIN-FULL BERLIN-RND BERLIN-CTA BERLIN-SPN GRID-FULL GRID-RND GRID-CTA GRID-SPN)
#topologies=(GRID-SPN BERLIN-SPN)
#topologies=(GRID-RND GRID-CTA GRID-SPN)
topologies=(GRID-FULL)
#nd_possibilities=(NV CL BL) # Naive Collect and Baseline
# nd_possibilities=(NV-NV IM-NV CL NV-SC IM-SC)
# nd_possibilities=(IM-SC CL NV-NV)
# nd_possibilities=(IM-SC-unidir IM-SC-contikimac CL-contikimac CL-nullrdc)
# nd_possibilities=(IM-SC-unidir)
# nd_possibilities=(NV-NV-unidir)
# nd_possibilities=(IM-SC-nullrdc-bidir IM-SC-nullrdc-unidir)
nd_possibilities=(NV-NV)
# taxa com que os pacotes de dados sao gerados - pacotes/min
# datarates=(0.2 2)
datarates=(1)
# SIM_TIME_MS=6000   # 0.1 minute
# SIM_TIME_MS=60000   # 1 minute
# SIM_TIME_MS=180000   # 3 minutes
# SIM_TIME_MS=300000  # 5 minutes
 SIM_TIME_MS=600000  # 10 minutes
# SIM_TIME_MS=1200000 # 20 minutes
# SIM_TIME_MS=1800000 # 30 minutes
# SIM_TIME_MS=3600000 # 60 minutes

## datadistrs=(CTE EXP)
### datadistrs=(CTE)

# the follwing variables are defined in path_simulation.sh:
# contiki_dir, cooja_dir, simulation_files_dir, simulation_output_dir,
# build_dir, controller_dir, controller_build_dir, QMAKE, CONTIKI
source path_simulation.sh

# list of process ids of currently running cooja simulators
pidJava=()
# list of process ids of currently running controllers
pidController=()
# list of process simlation logs (to run sdn_get_statistics_preproc.py on)
simulationLogs=()

run_cooja() {
# $1 => path to cooja .csc file
# $2 => where the cooja log will be saved
# $3 => where the controller log will be saved
# $4 => number employed to choose a different cooja directory for
#				each parallel simulation
	# (
		# cd "${contiki_dir}/${cooja_dir}/dist"
		# java -mx1512m -jar cooja.jar -nogui="$1" > $2 2>&1 &
		FILE="$1"
		cd "${contiki_dir}/${cooja_dir}/"
		./gradlew run -Dorg.gradle.jvmargs="-Xmx512m" --args="--no-gui $FILE" > $2 2>&1 &

		# append the pid of the last process (i.e. $!) to the list
		pidJava=(${pidJava[@]} $!)
		echo ${pidJava[@]}

		# wait until the cooja log exists
		while [ ! -f $2 ]; do
				sleep 0.5s
				echo "Waiting log"
		done

		# wait until the serial socket is available
		while [ `grep -c "Listening on port" $2` -ne 1 ]; do
						sleep 0.5s
						echo "Waiting for serial socket to become available"
		done

		# runnig the controller
		"$controller_build_dir"/controller-pc $nnodes --platform offscreen > $3 2>&1 &
		# appends the pid of the last process (i.e. $!) to the list
		pidController=(${pidController[@]} $!)
		cd -
	# )
}

# echo "" > makelog.log

mkdir -p $simulation_output_dir

if [ $COOJA_INSTANCES -gt $(($MAX_ITER - $MIN_ITER + 1)) ]; then
	echo COOJA_INSTANCES has to be larger than the iteration range defined by MAX_ITER and MIN_ITER
	exit
fi

port=60000
for nnodes in "${nodes_v[@]}"; do
	for topo in "${topologies[@]}"; do
		for nd in "${nd_possibilities[@]}"; do
			for datarate in "${datarates[@]}"; do
				for iter in `seq $MIN_ITER $MAX_ITER`; do
					# s - schedule (CTK - contiki default, IND - individual slots for each node)
					# n - number of nodes
					# d - data traffic type
					# l - lambda (data rate = 1/packet interval)
					# i - iteration (experimetn number)

					cooja_file="$simulation_files_dir"/n${nnodes}_${topo}.csc
					cooja_motes_out_file="$simulation_output_dir"/"cooja_n"$nnodes"_top"$topo"_nd"$nd"_l"$datarate"_i"$iter'.txt'
					cooja_log_file="$simulation_output_dir"/"logcooja_n"$nnodes"_top"$topo"_nd"$nd"_l"$datarate"_i"$iter'.txt'
					controller_out_file="$simulation_output_dir"/"controller_n"$nnodes"_top"$topo"_nd"$nd"_l"$datarate"_i"$iter'.txt'

					echo Cooja simulation file: $cooja_file
					echo Cooja logfile name: $cooja_motes_out_file
					echo Controller logfile name: $controller_out_file

					if [[ ("$DO_NOT_OVERWRITE" = true) && (`grep -c "Duration" ${cooja_log_file}` -eq 1) ]]; then
						echo "Skipping $cooja_log_file"
						continue
					fi
					echo "Not Skipping"

					simulationLogs=(${simulationLogs[@]} ${cooja_motes_out_file})
					port=$(($port+1));
					echo "port $port"

					# Change the COOJA simuation file to save simulation log with a different name
					sed -i.bak "s,my_output =.*,my_output = new FileWriter(\"$cooja_motes_out_file\");," ${cooja_file}
					# change csc file serial server port accordingly
					sed -i.bak "s,<port>60.*,<port>${port}</port>," ${cooja_file}
					# setting simulation timeout
					sed -i.bak "s,TIMEOUT.*,TIMEOUT($SIM_TIME_MS);," ${cooja_file}
					# set max simulation speed
					sed -i.bak "s,sim.setSpeedLimit(.*);,sim.setSpeedLimit(4);," ${cooja_file}

					# Clean current binaries
					(
						cd "$build_dir"

						if [ "$nd" == "CL" ]; then
							./change_to_collect.sh
						elif [ "$nd" == "NV-NV" ]; then
							./change_to_NV-NV.sh
						elif [ "$nd" == "IM-NV" ]; then
							./change_to_IM-NV.sh
						elif [ "$nd" == "NV-SC" ]; then
							./change_to_NV-SC.sh
						elif [ "$nd" == "IM-SC" ]; then
							./change_to_IM-SC.sh
						elif [ "$nd" == "IM-SC-nullrdc-unidir" ]; then
							./change_to_IM-SC_nullrdc-unidir.sh
						elif [ "$nd" == "IM-SC-nullrdc-bidir" ]; then
							./change_to_IM-SC_nullrdc-bidir.sh
						elif [ "$nd" == "IM-SC-unidir" ]; then
							./change_to_IM-SC_unidirRDC.sh
						elif [ "$nd" == "IM-SC-contikimac" ]; then
							./change_to_IM-SC_contikimac.sh
						elif [ "$nd" == "CL-contikimac" ]; then
							./change_to_collect_contikimac.sh
						elif [ "$nd" == "CL-nullrdc" ]; then
							./change_to_collect_nullrdc.sh
						elif [ "$nd" == "NV-NV-unidir" ]; then
							./change_to_NV-NV_unidirRDC.sh
						# elif [ "$nd" == "BL" ]; then
						else
							echo "there is no such ND algorithm"
							continue
						fi

						# make TARGET=sky clean -f Makefile_enabled_node
						# make TARGET=sky -f Makefile_enabled_node sink-node

						# make TARGET=sky clean -f Makefile_enabled_node
						# make TARGET=sky -f Makefile_enabled_node enabled-node

						make TARGET=sky clean -f Makefile_enabled_node
						make TARGET=sky -f Makefile_enabled_node

						make TARGET=sky clean -f Makefile_controller_node
						make TARGET=sky -f Makefile_controller_node
					)

					#controller recompilation
					(
						cd $controller_build_dir
						# changing serial server expected port
						sed -i.bak "s,<number>60.*,<number>${port}</number>," ../mainwindow.ui
						$QMAKE ../controller-pc.pro
						$MAKE clean
						$MAKE
					)

					run_cooja `readlink -f ${cooja_file}` "$cooja_log_file" "$controller_out_file" $COOJA_CURRENT_INSTANCE
					((COOJA_CURRENT_INSTANCE++))

					# reset csc file parameters
					sed -i.bak "s,my_output =.*,my_output = new FileWriter(\"/dev/null\");," ${cooja_file}
					sed -i.bak "s,<port>60.*,<port>60001</port>," ${cooja_file}
					sed -i.bak "s,TIMEOUT.*,TIMEOUT(1800000);," ${cooja_file}

					if [ ${#pidJava[@]} -eq $COOJA_INSTANCES ]; then
						# Waiting simulations to finish
						wait ${pidJava[@]}

						# python sdn_get_statistics_preproc.py  $cooja_motes_out_file &
						for simulationLog in "${simulationLogs[@]}"; do
							# python sdn_get_statistics_preproc.py  $simulationLog &
							echo "Preprocessing" "$simulationLog"preproc
							grep -E -e "=(T|R)X.?=" -e =FG= -e E:\ [0-9]+\ mJ $simulationLog > "$simulationLog"preproc
						done

						# Killing controllers
						kill ${pidController[@]}

						# reseting pid lists
						pidJava=()
						pidController=()
						simulationLogs=()
						COOJA_CURRENT_INSTANCE=1
					fi

				done
			done
		done
	done
done

# wait for any remaining simulations
wait ${pidJava[@]}
# python sdn_get_statistics_preproc.py  $cooja_motes_out_file &
for simulationLog in "${simulationLogs[@]}"; do
	python sdn_get_statistics_preproc.py  $simulationLog &
done
# Killing controllers
kill ${pidController[@]}

# set motes to default configuration
(
	cd "$build_dir"

	./change_to_collect.sh

	make TARGET=sky clean -f Makefile_enabled_node
	make TARGET=sky -f Makefile_enabled_node sink-node

	make TARGET=sky clean -f Makefile_enabled_node
	make TARGET=sky -f Makefile_enabled_node enabled-node

	make TARGET=sky clean -f Makefile_controller_node
	make TARGET=sky -f Makefile_controller_node
)

# controller default port
(
	cd $controller_build_dir
	# changing serial server expected port
	sed -i.bak "s,<number>60.*,<number>60001</number>," ../mainwindow.ui
	$QMAKE ../controller-pc.pro
	$MAKE clean
	$MAKE
)
echo END