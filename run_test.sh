#/usr/bin/env bash

# Make sure all directories needed for this run exist
for VAR in "${!FPGA_PATH2EMU_@}"; do mkdir -p ${!VAR}; done

echo "[INFO] Running test for mode: ${XCL_EMULATION_MODE}"
export EMCONFIG_PATH=${FPGA_PATH2CONF}
${FPGA_PATH2EMU_BIN}/app_${XCL_EMULATION_MODE} \
  --xclbin ${FPGA_PATH2EMU_BIN}/kernels.xclbin \
  --treename ${XCL_EMULATION_MODE} \

mv *.csv ${FPGA_PATH2EMU_SUM}/
mv *.run_summary ${FPGA_PATH2EMU_SUM}/

# move xilinx-files only if they are present
find . -maxdepth 1 -type f -iname 'xilinx*' -exec mv -t ${FPGA_PATH2EMU_SUM}/ {} \+

