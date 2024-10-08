#!/usr/bin/env bash

path_hw_from="out/hw.p_xilinx_u280_gen3x16.mode_debug"
path_hw_to="condor/results/hw_"
path_sw_from="out/emu_sw.p_xilinx_u280_gen3x16.mode_debug"
path_sw_to="condor/results/sw_"

path_form=${path_hw_from}
path_to="dat/archive/2024.10.08 eight/"

echo -e "\n##########################################################################################################################"
time="$(date +%H:%M:%S)"
echo "[${time}] start"
echo "1) direct_fixed_rolled"
echo "2) direct_variable_rolled"
echo "3) split_fixed_rolled"
echo "4) split_variable_rolled"
echo "5) direct_fixed_unrolled"
echo "6) direct_variable_unrolled"
echo "7) split_fixed_unrolled"
echo "8) split_variable_unrolled"
# echo "[${time}] test all copy-commands and paths"
# cp "src/kernels_split.cpp" src/kernels.cpp
# cp "src/kernels_direct.cpp" src/kernels.cpp
# cp -R "${path_form}" "${path_to}test"
echo -e "##########################################################################################################################\n"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] 1) compile for HW, direct_fixed_rolled"
#cp hpp, cpp
cp "condor/kernel/eight/direct_fixed_rolled.cpp" src/kernels.cpp
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
cp -R "${path_form}" "${path_to}direct_fixed_rolled"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] 2) compile for HW, direct_variable_rolled"
#cp hpp, cpp
cp "condor/kernel/eight/direct_variable_rolled.cpp" src/kernels.cpp
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
cp -R "${path_form}" "${path_to}direct_variable_rolled"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] 3) compile for HW, split_fixed_rolled"
#cp hpp, cpp
cp "condor/kernel/eight/split_fixed_rolled.cpp" src/kernels.cpp
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
cp -R "${path_form}" "${path_to}split_fixed_rolled"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] 4) compile for HW, split_variable_rolled"
#cp hpp, cpp
cp "condor/kernel/eight/split_variable_rolled.cpp" src/kernels.cpp
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
cp -R "${path_form}" "${path_to}split_variable_rolled"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] 5) compile for HW, direct_fixed_unrolled"
#cp hpp, cpp
cp "condor/kernel/eight/direct_fixed_unrolled.cpp" src/kernels.cpp
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
cp -R "${path_form}" "${path_to}direct_fixed_unrolled"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] 6) compile for HW, direct_variable_unrolled"
#cp hpp, cpp
cp "condor/kernel/eight/direct_variable_unrolled.cpp" src/kernels.cpp
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
cp -R "${path_form}" "${path_to}direct_variable_unrolled"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] 7) compile for HW, split_fixed_unrolled"
#cp hpp, cpp
cp "condor/kernel/eight/split_fixed_unrolled.cpp" src/kernels.cpp
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
cp -R "${path_form}" "${path_to}split_fixed_unrolled"

echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] 8) compile for HW, split_variable_unrolled"
#cp hpp, cpp
cp "condor/kernel/eight/split_variable_unrolled.cpp" src/kernels.cpp
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
cp -R "${path_form}" "${path_to}split_variable_unrolled"


time="$(date +%H:%M:%S)"
echo -e "\n[${time}] all compilation ended"

