#/usr/bin/env bash

# Make sure all directories needed for this run exist
for VAR in "${!FPGA_PATH2EMU_@}"; do mkdir -p ${!VAR}; done

echo "[INFO] Running test for mode: ${XCL_EMULATION_MODE_CHANGED}"

if [[ "${XCL_EMULATION_MODE_CHANGED}" = "hw" ]]
then
  unset XCL_EMULATION_MODE
  export EMCONFIG_PATH=${FPGA_PATH2CONF}
  ${FPGA_PATH2EMU_BIN}/app_${XCL_EMULATION_MODE_CHANGED} \
    --xclbin ${FPGA_PATH2EMU_BIN}/par_kernels.xclbin
elif [[ "${XCL_EMULATION_MODE_CHANGED}" = "hw_emu" || "${XCL_EMULATION_MODE_CHANGED}" = "sw_emu" ]]
then
  if [ -z ${XCL_EMULATION_MODE} ]; then
    export XCL_EMULATION_MODE="${XCL_EMULATION_MODE_CHANGED}"
  fi
  export EMCONFIG_PATH=${FPGA_PATH2CONF}
  ${FPGA_PATH2EMU_BIN}/app_${XCL_EMULATION_MODE_CHANGED} \
    --xclbin ${FPGA_PATH2EMU_BIN}/par_kernels.xclbin \
  --treename ${XCL_EMULATION_MODE_CHANGED}
else
  echo "[ERROR] Wrong emulation mode: ${XCL_EMULATION_MODE_CHANGED}"
fi

mv *.csv ${FPGA_PATH2EMU_SUM}/
mv *.run_summary ${FPGA_PATH2EMU_SUM}/

# move xilinx-files only if they are present
find . -maxdepth 1 -type f -iname 'xilinx*' -exec mv -t ${FPGA_PATH2EMU_SUM}/ {} \+

