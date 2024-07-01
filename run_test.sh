#/usr/bin/env bash

# Make sure all directories needed for this run exist
for VAR in "${!FPGA_PATH2EMU_@}"; do mkdir -p ${!VAR}; done

echo "[INFO] Running test for mode: ${XCL_EMULATION_MODE}"
${FPGA_PATH2EMU_BIN}/app_${XCL_EMULATION_MODE} \
  --xclbin ${FPGA_PATH2EMU_BIN}/kernels.xclbin \
  --treename ${XCL_EMULATION_MODE} \

mv *.csv ${FPGA_PATH2EMU_SUM}/
mv *.run_summary ${FPGA_PATH2EMU_SUM}/


