#!/bin/bash

# strong scaling = how much faster can we go as we add cores?
# weak scaling = how fast can we keep going as we multiple the number of cores and the size of the problem by the same amount?

program="./stencil"

repeat=3

initial_stencil_size=4
initial_halo_size=4

separator=','
header="stencil_size${separator}halo_size${separator}gflops"
second_minute_hour_day_month_year=$(date +"%S_%M_%H_%d_%m_%Y")
result_dir="results"
result_file="${result_dir}/gflops_${second_minute_hour_day_month_year}.csv"
# trash_temp="trash_temp"

###############
#GETOPT begin
###############

TEMP=$(getopt -o 'hr:p:s:H:' --long 'repeat:,program:,stencil_size:,halo_size:' -n 'get_gflops.sh' -- "$@")

if [ $? -ne 0 ]; then
	echo 'Terminating...' >&2
	exit 1
fi

# Note the quotes around "$TEMP": they are essential!
eval set -- "$TEMP"
unset TEMP

print_help() {
	cat <<EOF
Usage: $0 [OPTION ...]

Generate data for the given parameters, to be later processed by the graph
script.

OPTIONS

  -h, --help
        Print this message and exit

  -r, --repeat [n]
		Number of runs for the same program and the same args, so that a mean can be computed. Default is ${repeat}.
  -p, --program [prog_name]
		Name of the program to run (the program computes and prints the gflops of its own run). Default is ${program}.
  -s, --stencil_size [n]
		The initial stencil size, potentially increasing along the calls made by this script. Default is ${initial_stencil_size}.
  -h, --halo_size [n]
		The initial halo size, potentially increasing along the calls made by this script. Default is ${initial_halo_size}.
EOF
}

while true; do
	case "$1" in
	'-h' | '--help')
		print_help
		exit 0
		;;
	'-r' | '--repeat')
		repeat=$2
		continue
		;;
	'-p' | '--program')
		program=$2
		continue
		;;
	'-s' | '--stencil_size')
		initial_stencil_size=$2
		continue
		;;
	'-H' | '--halo_size')
		initial_halo_size=$2
		continue
		;;
	'--')
		shift
		break
		;;
	*)
		echo 'Argument parsing error!' >&2
		print_help
		exit 1
		;;
	esac
done

echo 'Remaining arguments:'
for arg; do
	echo "--> '$arg'"
done

###############
#GETOPT end
###############

echo $header >${result_file}

make_run_benchmark() {
	local stencil_size=$1
	local halo_size=$2

	local gflops_sum=0
	local cur_gflops=0

	make clean
	make "CFLAGS += -Wall -g -O4 -DSTENCIL_SIZE=${stencil_size} -DHALO_SIZE=${halo_size}"
	for rep in $(seq 1 ${repeat}); do
		cur_gflops=$(./${program} | grep gflops | cut -d "=" -f 2)
		gflops_sum=$(echo "${gflops_sum} + ${cur_gflops}" | bc)
	done
	local gflops=$(echo "scale=4; ${gflops_sum}/${repeat}" | bc)
	echo "${stencil_size}${separator}${halo_size}${separator}${gflops}" >>${result_file}
}

benchmark_stencil_size() {
	local stencil_size=${initial_stencil_size}
	for i in {1..8}; do
		make_run_benchmark ${stencil_size} ${initial_halo_size}
		stencil_size=$((${stencil_size} * 2))
	done
}

benchmark_stencil_and_halo_size() {
	local halo_size=${initial_halo_size}
	for i in {1..5}; do
		local stencil_size=${initial_stencil_size}
		for j in {1..5}; do
			make_run_benchmark ${stencil_size} ${halo_size}
			stencil_size=$((${stencil_size} * 2))
		done
		halo_size=$((${halo_size} * 2))
	done
}

benchmark_stencil_and_halo_size

echo $result_file

#rm ${trash_temp}
