#!/usr/bin/env bash

path_output="out/hw.p_xilinx_u280_gen3x16.mode_debug"
path_files="benchmark/src/"
path_results="benchmark/results/"

make_mode="adsf"

compile () {
    echo -e "\n\n\n##########################################################################################################################"
    echo -e "##########################################################################################################################"
    echo -e "##########################################################################################################################\n\n\n"
    echo "[$(date +%H:%M:%S)] compile version $1 in make-mode $2"
    if [[ "$2" = "large" || "$2" = "par" || "$2" = "ddr" || "$2" = "test" ]]; then
        make_mode="$2_"
    else
        make_mode=""
    fi
    echo "[$(date +%H:%M:%S)] copy files to src, inc and conf"
    echo "[$(date +%H:%M:%S)] from: ${path_files}$1/host.cpp to: src/${make_mode}host.cpp"
    cp "${path_files}$1/host.cpp" src/${make_mode}host.cpp
    echo "[$(date +%H:%M:%S)] from: ${path_files}$1/kernels.cpp to: src/${make_mode}kernels.cpp"
    cp "${path_files}$1/kernels.cpp" src/${make_mode}kernels.cpp
    echo "[$(date +%H:%M:%S)] from: ${path_files}$1/kernels.hpp to: inc/${make_mode}kernels.hpp"
    cp "${path_files}$1/kernels.hpp" inc/${make_mode}kernels.hpp
    echo "[$(date +%H:%M:%S)] from: ${path_files}$1/u280.cfg to: conf/${make_mode}u280.cfg"
    cp "${path_files}$1/u280.cfg" conf/${make_mode}u280.cfg
    echo "[$(date +%H:%M:%S)] make $2"
    make $2
    echo "[$(date +%H:%M:%S)] copy results back to ${path_results}$1"
    cp -R "${path_output}" "${path_results}$1"
    echo -e "\n\n##########################################################################################################################"
}

################# ToDo #################
# prepare fe_fil_lookup
# compile "reg_fil_par_8" "ddr"

compile "reg_fil_FPGA" "test"

echo -e "\n[$(date +%H:%M:%S)] all compilation ended"

################# Done #################
# compile "fe_dir_NAIVE" "ddr"
# compile "fe_dir_DDR_1" "ddr"
# compile "fe_dir_DDR_4" "ddr"
# compile "fe_dir_par_1" "par"
# compile "fe_dir_par_2" "par"

# compile "fe_dir_par_4" "par"
# compile "fe_dir_par_8" "par"
# compile "fe_fil_par_1" ""
# compile "fe_fil_par_2" ""
# compile "fe_fil_par_4" ""

# compile "fe_fil_par_8" ""
# compile "reg_dir_FPGA" ""
# compile "reg_dir_par_1" "test"
# compile "reg_dir_par_2" "test"
# compile "reg_dir_par_4" "test"

# compile "reg_dir_par_8" "test"
# compile "reg_fil_FPGA" "test"
# compile "reg_fil_par_1" "large"
# compile "reg_fil_par_2" "large"
# compile "reg_fil_par_4" "large"


