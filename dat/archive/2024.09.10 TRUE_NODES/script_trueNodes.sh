#!/usr/bin/env bash

path_hw_from="out/hw.p_xilinx_u280_gen3x16.mode_debug"
path_hw_to="condor/results/hw_"
path_sw_from="out/emu_sw.p_xilinx_u280_gen3x16.mode_debug"
path_sw_to="condor/results/sw_"

path_form=${path_hw_from}
path_to=${path_hw_to}
echo "execute $0 for HW-execution" #HW-execution #SW-emulation
echo "copy each kernel into inc-directory"
echo "make clean - to make sure that the code is compiled with the new parameters"
echo "make - compile the code"
echo "run the code"
echo "copy results back to condor/results"

# function
compile_and_run () {
    echo -e "\n\n\n##########################################################################################################################"
    echo -e "##########################################################################################################################"
    echo -e "##########################################################################################################################\n\n\n"
    time="$(date +%H:%M:%S)"
    echo "[${time}] kernel $1"
    cp "condor/kernel/TRUE_NODES/kernels$1.hpp" inc/kernels.hpp
    # cp "condor/kernel/MAX_NODES/kernels$1.hpp" inc/kernels.hpp
    time="$(date +%H:%M:%S)"
    echo -e "[${time}] make clean - to make sure that the code is compiled with the new parameters"
    make clean
    time="$(date +%H:%M:%S)"
    echo -e "[${time}] make - compile the code\n"
    make
    time="$(date +%H:%M:%S)"
    echo -e "\n\n##########################################################################################################################"
    echo -e "[${time}] run the code\n\n"
    ./run_test.sh
    time="$(date +%H:%M:%S)"
    echo -e "\n[${time}] copy results back to condor/results\n"
    cp -R "${path_form}" "${path_to}$1"
}


compile_and_run "128"
compile_and_run "256"
compile_and_run "512"
compile_and_run "1024"
compile_and_run "2048"
compile_and_run "4096"
compile_and_run "8192"
compile_and_run "16384"
compile_and_run "32768"
compile_and_run "65536"

# compile_and_run "131072"
# compile_and_run "262144"
# compile_and_run "524288"
# compile_and_run "65536"
# compile_and_run "73728"
# compile_and_run "81920"
# compile_and_run "90112"
# compile_and_run "98304"
# compile_and_run "106496"
# compile_and_run "114688"
# compile_and_run "122880"
# compile_and_run "131072"

# # 128
# num="128"
# time="$(date +%H:%M:%S)"
# echo -e "\n\n\n##########################################################################################################################"
# echo -e "##########################################################################################################################"
# echo -e "##########################################################################################################################\n\n\n"
# echo "[${time}] kernel ${num}"
# cp "condor/kernel/kernels${num}.hpp" inc/kernels.hpp
# time="$(date +%H:%M:%S)"
# echo -e "[${time}] make clean - to make sure that the code is compiled with the new parameters"
# make clean
# time="$(date +%H:%M:%S)"
# echo -e "[${time}] make - compile the code\n"
# make
# time="$(date +%H:%M:%S)"
# echo -e "\n####################################################\n[${time}] run the code\n"
# ./run_test.sh
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] copy results back to condor/results\n"
# cp -R "${path_form}" "${path_to}${num}"

# # 256
# num="256"
# time="$(date +%H:%M:%S)"
# echo -e "\n\n\n##########################################################################################################################"
# echo -e "##########################################################################################################################"
# echo -e "##########################################################################################################################\n\n\n"
# echo "[${time}] kernel ${num}"
# cp "condor/kernel/kernels${num}.hpp" inc/kernels.hpp
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] make clean - to make sure that the code is compiled with the new parameters\n"
# make clean
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] make - compile the code\n"
# make
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] run the code\n"
# ./run_test.sh
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] copy results back to condor/results\n"
# cp -R "${path_form}" "${path_to}${num}"

# # 512
# num="512"
# time="$(date +%H:%M:%S)"
# echo -e "\n\n\n##########################################################################################################################"
# echo -e "##########################################################################################################################"
# echo -e "##########################################################################################################################\n\n\n"
# echo "[${time}] kernel ${num}"
# cp "condor/kernel/kernels${num}.hpp" inc/kernels.hpp
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] make clean - to make sure that the code is compiled with the new parameters\n"
# make clean
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] make - compile the code\n"
# make
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] run the code\n"
# ./run_test.sh
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] copy results back to condor/results\n"
# cp -R "${path_form}" "${path_to}${num}"

# # 1024
# num="1024"
# time="$(date +%H:%M:%S)"
# echo -e "\n\n\n##########################################################################################################################"
# echo -e "##########################################################################################################################"
# echo -e "##########################################################################################################################\n\n\n"
# echo "[${time}] kernel ${num}"
# cp "condor/kernel/kernels${num}.hpp" inc/kernels.hpp
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] make clean - to make sure that the code is compiled with the new parameters\n"
# make clean
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] make - compile the code\n"
# make
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] run the code\n"
# ./run_test.sh
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] copy results back to condor/results\n"
# cp -R "${path_form}" "${path_to}${num}"

# # 2048
# num="2048"
# time="$(date +%H:%M:%S)"
# echo -e "\n\n\n##########################################################################################################################"
# echo -e "##########################################################################################################################"
# echo -e "##########################################################################################################################\n\n\n"
# echo "[${time}] kernel ${num}"
# cp "condor/kernel/kernels${num}.hpp" inc/kernels.hpp
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] make clean - to make sure that the code is compiled with the new parameters\n"
# make clean
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] make - compile the code\n"
# make
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] run the code\n"
# ./run_test.sh
# time="$(date +%H:%M:%S)"
# echo -e "\n[${time}] copy results back to condor/results\n"
# cp -R "${path_form}" "${path_to}${num}"

# echo "copy kernel $1 to inc-dir"
# cp condor/kernel/kernels$1.hpp inc/kernels.hpp
# echo "make clean - make sure that the code is compiled with the new parameters"
# make clean
# echo "make - compile the code"
# make
# echo "run - run the code"
# ./run_test.sh
# echo "copy results back to condor/results"
# cp -R out/hw.p_xilinx_u280_gen3x16.mode_debug condor/results/hw_$1
