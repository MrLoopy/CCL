#!/usr/bin/env bash

path_hw_from="out/hw.p_xilinx_u280_gen3x16.mode_debug"
path_hw_to="condor/results/hw_"
path_sw_from="out/emu_sw.p_xilinx_u280_gen3x16.mode_debug"
path_sw_to="condor/results/sw_"

path_form=${path_hw_from}
path_to="dat/archive/2024.10.04 pragmas/"

# echo -e "\n##########################################################################################################################"
# time="$(date +%H:%M:%S)"
# echo "[${time}] test all copy-commands and paths"
# cp "src/kernels_split.cpp" src/kernels.cpp
# cp "src/kernels_direct.cpp" src/kernels.cpp
# cp -R "${path_form}" "${path_to}test"
# echo -e "##########################################################################################################################\n"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] compile for HW, AFTER loop-labels and fixed loop-bounds"
#cp hpp, cpp
cp "src/kernels_split.cpp" src/kernels.cpp
#make clean
time="$(date +%H:%M:%S)"
echo -e "[${time}] make clean - to make sure that the code is compiled with the new parameters"
make clean
#make
time="$(date +%H:%M:%S)"
echo -e "[${time}] make - compile the code\n"
make
#copy results back
time="$(date +%H:%M:%S)"
echo -e "\n[${time}] copy results to save them before new compilation\n"
cp -R "${path_form}" "${path_to}after"


echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] compile for HW, DIRECT version"
#cp hpp, cpp
cp "src/kernels_direct.cpp" src/kernels.cpp
#make clean
time="$(date +%H:%M:%S)"
echo -e "[${time}] make clean - to make sure that the code is compiled with the new parameters"
make clean
#make
time="$(date +%H:%M:%S)"
echo -e "[${time}] make - compile the code\n"
make
#copy results back
time="$(date +%H:%M:%S)"
echo -e "\n[${time}] copy results to save them before new compilation\n"
cp -R "${path_form}" "${path_to}direct"


# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] compilation complete, ready to execute\n"

