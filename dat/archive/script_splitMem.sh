#!/usr/bin/env bash

path_hw_from="out/hw.p_xilinx_u280_gen3x16.mode_debug"
path_hw_to="condor/results/hw_"
path_sw_from="out/emu_sw.p_xilinx_u280_gen3x16.mode_debug"
path_sw_to="condor/results/sw_"

path_form=${path_hw_from}
path_to=${path_hw_to}

fun_man_short () {
    #header - manual-short
    echo -e "\n\n\n##########################################################################################################################"
    echo -e "##########################################################################################################################"
    echo -e "##########################################################################################################################\n\n\n"
    time="$(date +%H:%M:%S)"
    echo "[${time}] execute $1 Manual-Version for HW-execution"
    #cp hpp, cpp
    cp "condor/kernel/Mem_split/kernels_short.hpp" inc/kernels.hpp
    cp "condor/kernel/Mem_split/kernels_manual.cpp" src/kernels.cpp
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
    cp -R "${path_form}" "${path_to}manual_short_$1"
}

fun_prag_short () {
    #header - pragma-short
    echo -e "\n\n\n##########################################################################################################################"
    echo -e "##########################################################################################################################"
    echo -e "##########################################################################################################################\n\n\n"
    time="$(date +%H:%M:%S)"
    echo "[${time}] execute $1 Pragma-Version for HW-execution"
    #cp hpp, cpp
    cp "condor/kernel/Mem_split/kernels_short.hpp" inc/kernels.hpp
    cp "condor/kernel/Mem_split/kernels_pragma.cpp" src/kernels.cpp
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
    cp -R "${path_form}" "${path_to}pragma_short_$1"
}

fun_man_short "1"

fun_prag_short "1"

#header - manual-long
echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] compile Manual-Version for Event"
#cp hpp, cpp
cp "condor/kernel/Mem_split/kernels_long.hpp" inc/kernels.hpp
cp "condor/kernel/Mem_split/kernels_manual.cpp" src/kernels.cpp
#make clean
time="$(date +%H:%M:%S)"
echo -e "[${time}] make clean - to make sure that the code is compiled with the new parameters"
make clean
#make
time="$(date +%H:%M:%S)"
echo -e "[${time}] make - compile the code\n"
make
#cp to hpp_cpp_path
time="$(date +%H:%M:%S)"
echo -e "\n[${time}] copy results of compilation back to condor/results\n"
cp -R "${path_form}" "${path_to}manual_long"


#header - pragma-long
echo -e "\n\n\n##########################################################################################################################"
echo -e "##########################################################################################################################"
echo -e "##########################################################################################################################\n\n\n"
time="$(date +%H:%M:%S)"
echo "[${time}] compile Pragma-Version for Event"
#cp hpp, cpp
cp "condor/kernel/Mem_split/kernels_long.hpp" inc/kernels.hpp
cp "condor/kernel/Mem_split/kernels_pragma.cpp" src/kernels.cpp
#make clean
time="$(date +%H:%M:%S)"
echo -e "[${time}] make clean - to make sure that the code is compiled with the new parameters"
make clean
#make
time="$(date +%H:%M:%S)"
echo -e "[${time}] make - compile the code\n"
make
#cp to hpp_cpp_path
time="$(date +%H:%M:%S)"
echo -e "\n[${time}] copy results of compilation back to condor/results\n"
cp -R "${path_form}" "${path_to}pragma_long"


fun_man_short "2"
fun_man_short "3"
fun_prag_short "2"
fun_prag_short "3"


