#!/usr/bin/env bash

path_hw_from="out/hw.p_xilinx_u280_gen3x16.mode_debug"
path_hw_to="condor/results/hw_"
path_sw_from="out/emu_sw.p_xilinx_u280_gen3x16.mode_debug"
path_sw_to="condor/results/sw_"

path_form=${path_hw_from}
path_to=${path_hw_to}

#header - fixed version
echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] execute fixed version for HW-execution"
#cp hpp, cpp
cp "condor/kernel/fixed/kernels_dummy.hpp" inc/kernels.hpp
#make clean
time="$(date +%H:%M:%S)"
echo -e "[${time}] make clean - to make sure that the code is compiled with the new parameters"
make clean
#make
time="$(date +%H:%M:%S)"
echo -e "[${time}] make - compile the code\n"
make
#run (for short)
time="$(date +%H:%M:%S)"
echo -e "\n\n##########################################################################################################################"
echo -e "[${time}] run the code\n\n"
./run_test.sh
#cp to hpp_cpp_path
time="$(date +%H:%M:%S)"
echo -e "\n[${time}] copy results back to condor/results\n"
cp -R "${path_form}" "${path_to}fixed"


#header - compile large
echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] compile large version for HW-execution"
#cp hpp, cpp
cp "condor/kernel/fixed/kernels_event.hpp" inc/kernels.hpp
#make clean
time="$(date +%H:%M:%S)"
echo -e "[${time}] make clean - to make sure that the code is compiled with the new parameters"
make clean
#make
time="$(date +%H:%M:%S)"
echo -e "[${time}] make - compile the code\n"
make

echo -e "\n[${time}] compilation complete, ready to execute\n"

