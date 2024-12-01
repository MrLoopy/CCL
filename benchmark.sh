#!/usr/bin/env bash

path_output="out/hw.p_xilinx_u280_gen3x16.mode_debug"
path_files="benchmark/src/"

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
    echo -e "\n\n##########################################################################################################################"
}


compile "fe_dir_par_8" "par"
compile "fe_fil_par_8" ""
compile "reg_dir_par_8" "test"
compile "reg_fil_par_8" "large"
compile "fe_dir_DDR" "ddr"

echo -e "\n[$(date +%H:%M:%S)] all compilation ended"

