#!/usr/bin/env bash
# Christof Sauer 03.2024
# Kadir Tastepe 05.2024
# Tobias Linker 06.2024

#==============================
#
# Global configuration
#
#==============================

if [ -z ${XCL_EMULATION_MODE_CHANGED} ]; then
  export XCL_EMULATION_MODE_CHANGED=sw_emu
fi
if [ -z ${FPGA_RETURNTYPE} ]; then
  export FPGA_RETURNTYPE=float
fi

# Change name
IFS='_' read -ra __XCL_EMULATION_MODE_CHANGED__ <<< "${XCL_EMULATION_MODE_CHANGED}"
export FPGA_EMULATION_MODE="${__XCL_EMULATION_MODE_CHANGED__[0]}"
if [ "${#__XCL_EMULATION_MODE_CHANGED__[@]}" -eq 2 ]; then
  export FPGA_EMULATION_MODE="${__XCL_EMULATION_MODE_CHANGED__[1]}_${__XCL_EMULATION_MODE_CHANGED__[0]}"
fi

# Set the platform
export FPGA_PLATFORM=xilinx_u280_gen3x16_xdma_1_202211_1
# Split the plattform name
IFS='_' read -ra __FPGA_PLATFORM__ <<< "${FPGA_PLATFORM}"
export FPGA_PLATFORM_SHORT="${__FPGA_PLATFORM__[0]}_${__FPGA_PLATFORM__[1]}_${__FPGA_PLATFORM__[2]}"

# Some more infos
export FPGA_RUN_MODE=debug

# Define some variable
# export FPGA_PREFIX="${FPGA_EMULATION_MODE}.p_${FPGA_PLATFORM_SHORT}.rtype_${lookup[${FPGA_RETURNTYPE}]}.mode_${FPGA_RUN_MODE}"
export FPGA_PREFIX="${FPGA_EMULATION_MODE}.p_${FPGA_PLATFORM_SHORT}.mode_${FPGA_RUN_MODE}"

#==============================
#
# All path and directory stuff
#
#==============================

# Get the path to this script
SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
  SOURCE=$(readlink "$SOURCE")
  [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE 
done
SCRIPTPATH=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
# Define home path variable
export FPGA_PATH2HOME=${SCRIPTPATH}

# List of directories
#DIRS=(src inc etc logs bin lib dat obj fig tmp out ext)
DIRS=(src inc conf ext out dat)
# Define path variables to directories
for DIR in "${DIRS[@]}"; do
  export FPGA_PATH2${DIR^^}=${FPGA_PATH2HOME}/${DIR}
done

# Define emulation path variable
export FPGA_PATH2EMU=${FPGA_PATH2OUT}/${FPGA_PREFIX}
# List of output-directories
# DIRSOUT=(bin obj logs tmp etc dat fig)
DIRSOUT=(bin obj log sum)
# Substructure of output directories
for DIR in "${DIRSOUT[@]}"; do
  export FPGA_PATH2EMU_${DIR^^}=${FPGA_PATH2EMU}/${DIR}
done

# Path to profiling configuration file
export XRT_INI_PATH=${FPGA_PATH2CONF}/xrt.ini

# Create directories if they do not exist (only if they have the correct prefix)
for VAR in "${!FPGA_PATH@}"; do mkdir -p ${!VAR}; done

# Create some links
ln -snf ${FPGA_PATH2EMU} ${FPGA_PATH2OUT}/current_setup


#==============================
#
# Some helper
#
#==============================

export BOLDTXT=$(tput bold)
export NORMTXT=$(tput sgr0)

#==============================
#
# Some convenient functions
#
#==============================

function set_mode {
  # Check arguments
  if [ $# -eq 0 ]; then
    echo "[ERROR] No identifier has been provided. Please name one [sw_emu/hw_emu/hw]."
  elif [[ "$1" == "sw_emu" || "$1" == "hw_emu" || "$1" == "hw" ]]; then
    export XCL_EMULATION_MODE_CHANGED=${1}
    source ${SCRIPTPATH}/setup.sh
  else
    echo "[ERROR] Argument '${1}' not valid [sw_mode/hw_mode/hw]."
  fi
}
export -f set_mode

function set_type {
  # Check arguments
  if [ $# -eq 0 ]; then
    echo "[ERROR] No type has been provided. Please name one [double/float/int]."
  elif [[ "$1" == "double" || "$1" == "float" || "$1" == "int" ]]; then
    export FPGA_RETURNTYPE=${1}
    source ${SCRIPTPATH}/setup.sh
  else
    echo "[ERROR] Type '${1}' not valid [double/float/int]."
  fi
}
export -f set_type

#==============================
#
# FPGA specific configurations
#
#==============================

# home made exports for xilinx
export XLINXD_LICENSE_FILE=2100@129.206.127.92
export FINN_XILINX_PATH=/opt/xilinx
export FINN_XILINX_VERSION=2023.1
export PLATFORM_REPO_PATHS=$FINN_XILINX_PATH/Vitis/2023.1/platforms
source $FINN_XILINX_PATH/Vitis/2023.1/settings64.sh
source $FINN_XILINX_PATH/xrt/setup.sh > /dev/null 2>&1

#==============================
#
# Finalize
#
#==============================

# Show some config details to the user
# echo "[${BOLDTXT}INFO${NORMTXT}] Emulation mode: ${BOLDTXT}${XCL_EMULATION_MODE_CHANGED}${NORMTXT}; return type ${BOLDTXT}${FPGA_RETURNTYPE}${NORMTXT}"
echo "[${BOLDTXT}INFO${NORMTXT}] Emulation mode: ${BOLDTXT}${XCL_EMULATION_MODE_CHANGED}${NORMTXT}"

# Tell other scripts that setup.sh was called 
export FPGA_ENV_INIT=true
