#!/usr/bin/env bash

path_hw_from="out/hw.p_xilinx_u280_gen3x16.mode_debug"
path_hw_to="condor/results/hw_"
path_sw_from="out/emu_sw.p_xilinx_u280_gen3x16.mode_debug"
path_sw_to="condor/results/sw_"

path_form=${path_hw_from}
path_to="condor/results/sweep/hw_"
# path_to="dat/archive/2024.11.01 pipe/"

# echo -e "\n##########################################################################################################################"
# 
# echo "[$(date +%H:%M:%S)] test all copy-commands and paths"
# cp "src/kernels_split.cpp" src/kernels.cpp
# cp "src/kernels_direct.cpp" src/kernels.cpp
# cp -R "${path_form}" "${path_to}test"
# echo -e "##########################################################################################################################\n"

compile_and_run () {
    echo -e "\n\n\n##########################################################################################################################"
    echo -e "##########################################################################################################################"
    echo -e "##########################################################################################################################\n\n\n"
    echo "[$(date +%H:%M:%S)] compile region for HW ; MAX_FULL_GRAPH_EDGES = $1 ; version = $2"
    cp "condor/kernel/FULL_GRAPH_EDGES/test_kernels_$1.hpp" inc/test_kernels.hpp
    cp "condor/kernel/FULL_GRAPH_EDGES/test_kernels_$2.cpp" src/test_kernels.cpp
    echo -e "[$(date +%H:%M:%S)] make clean - to make sure that the code is compiled with the new parameters"
    make clean
    echo -e "[$(date +%H:%M:%S)] make - compile the code\n"
    make test
    echo -e "\n\n##########################################################################################################################"
    echo -e "[$(date +%H:%M:%S)] run the code\n\n"
    ./test_run.sh
    echo -e "\n[$(date +%H:%M:%S)] copy results back to condor/results/sweep/hw_$1_$2\n"
    cp -R "${path_form}" "${path_to}$1_$2"
}
compile_and_run_uint16 () {
    echo -e "\n\n\n##########################################################################################################################"
    echo -e "##########################################################################################################################"
    echo -e "##########################################################################################################################\n\n\n"
    echo "[$(date +%H:%M:%S)] compile region for HW ; UINT16 ; MAX_FULL_GRAPH_EDGES = $1 ; version = $2"
    cp "condor/kernel/FULL_GRAPH_EDGES/test_kernels_uint16_$1.hpp" inc/test_kernels.hpp
    # cp "condor/kernel/FULL_GRAPH_EDGES/test_kernels_$2.cpp" src/test_kernels.cpp
    echo -e "[$(date +%H:%M:%S)] make clean - to make sure that the code is compiled with the new parameters"
    make clean
    echo -e "[$(date +%H:%M:%S)] make - compile the code\n"
    make test
    echo -e "\n\n##########################################################################################################################"
    # echo -e "[$(date +%H:%M:%S)] run the code\n\n"
    # ./test_run.sh
    # echo -e "\n[$(date +%H:%M:%S)] copy results back to condor/results/sweep/hw_$1_$2\n"
    # cp -R "${path_form}" "${path_to}uint_$1_$2"
}
compile_and_run_band () {
    echo -e "\n\n\n##########################################################################################################################"
    echo -e "##########################################################################################################################"
    echo -e "##########################################################################################################################\n\n\n"
    echo "[$(date +%H:%M:%S)] compile region for HW ; Bandwidth ; MAX_FULL_GRAPH_EDGES = $1"
    cp "condor/kernel/FULL_GRAPH_EDGES/test_kernels_band_$1.hpp" inc/test_kernels.hpp
    echo -e "[$(date +%H:%M:%S)] make clean - to make sure that the code is compiled with the new parameters"
    make clean
    echo -e "[$(date +%H:%M:%S)] make - compile the code\n"
    make test
    echo -e "\n\n##########################################################################################################################"
}

# compile_and_run_uint16 "128" "URAM"
# compile_and_run_uint16 "64" "URAM"
# compile_and_run_uint16 "32" "URAM"

# compile_and_run "32" "URAM"
# compile_and_run "64" "URAM"
# compile_and_run "32" "CTRL"
# compile_and_run "64" "CTRL"
# compile_and_run "128" "CTRL"
# compile_and_run "256" "HBM"

# compile_and_run_band "256"
# compile_and_run_band "64"
# compile_and_run_band "128"

echo -e "\n##########################################################################################################################\n"
echo -e "[$(date +%H:%M:%S)] make clean - to make sure that the code is fully compiled"
make clean

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
echo "[$(date +%H:%M:%S)] compile large filter for HW with make"
echo -e "\n[$(date +%H:%M:%S)] make clean skipped\n"
echo -e "[$(date +%H:%M:%S)] make - compile the code\n"
make large
echo -e "\n[$(date +%H:%M:%S)] no copy back needed\n"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
echo "[$(date +%H:%M:%S)] compile bandwidth version for HW with make"
echo -e "\n[$(date +%H:%M:%S)] make clean skipped\n"
echo -e "[$(date +%H:%M:%S)] make - compile the code\n"
make ddr
echo -e "\n[$(date +%H:%M:%S)] no copy back needed\n"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
echo "[$(date +%H:%M:%S)] compile parallel filter for HW with make"
echo -e "\n[$(date +%H:%M:%S)] make clean skipped\n"
echo -e "[$(date +%H:%M:%S)] make - compile the code\n"
make
echo -e "\n[$(date +%H:%M:%S)] no copy back needed\n"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
echo "[$(date +%H:%M:%S)] compile region uint16 for HW with make test"
echo -e "\n[$(date +%H:%M:%S)] make clean skipped\n"
echo -e "[$(date +%H:%M:%S)] make - compile the code\n"
make test
echo -e "\n[$(date +%H:%M:%S)] no copy back needed\n"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
echo "[$(date +%H:%M:%S)] compile parallel direct for HW with make par"
echo -e "\n[$(date +%H:%M:%S)] make clean skipped\n"
echo -e "[$(date +%H:%M:%S)] make - compile the code\n"
make par
echo -e "\n[$(date +%H:%M:%S)] no copy back needed\n"

# echo -e "\n\n\n##########################################################################################################################"
# echo -e "##########################################################################################################################"
# echo -e "##########################################################################################################################\n\n\n"
# echo "[$(date +%H:%M:%S)] compile Test for HW"
# #cp hpp, cpp
# # cp "condor/kernel/eight/direct_fixed_rolled.cpp" src/kernels.cpp
# #make clean
# echo -e "[$(date +%H:%M:%S)] make clean - to make sure that the code is compiled with the new parameters"
# make clean
# #make
# echo -e "[$(date +%H:%M:%S)] make test - compile the code\n"
# make test
# #copy results back
# echo -e "\n[$(date +%H:%M:%S)] copy results to save them before new compilation\n"
# cp -R "${path_form}" "${path_to}test2"


echo -e "\n[$(date +%H:%M:%S)] all compilation ended"

