#======================
#
# Targets and config
#
#======================

# Binary name
EXE=app_${XCL_EMULATION_MODE_CHANGED}

# Directories to include
HOMEDIR := ${FPGA_PATH2EMU}
OBJDIR := ${FPGA_PATH2EMU_OBJ}
LOGDIR := ${FPGA_PATH2EMU_LOG}
SUMDIR := ${FPGA_PATH2EMU_SUM}
BINDIR := ${FPGA_PATH2EMU_BIN}
# ETCDIR := ${FPGA_PATH2EMU_ETC}
SRCDIR := ${FPGA_PATH2SRC}
INCDIR := ${FPGA_PATH2INC}
EXTDIR := ${FPGA_PATH2EXT}

# Make sure directories exist
#$(shell mkdir -p $(OBJDIR) $(BINDIR) $(LOGDIR) $(ETCDIR))
$(shell mkdir -p $(OBJDIR) $(BINDIR) $(LOGDIR) $(SUMDIR))

#======================
#
# Externals
#
#======================
# Argument parser
ARGPARS     := ${EXTDIR}/CLI11

#======================
#
# Compiler flags
#
#======================

# make phony target to simplify calls
.PHONY: clean 

# Targets
TARGET:= ${BINDIR}/${EXE}

CC = g++
LDFLAGS := -Itemp \
	   -lxrt_coreutil \
	   -L.${INCDIR} \
	   -L${XILINX_HLS}/lib \
	   -L${XILINX_XRT}/lib \
	   $(ROOTLFLAGS)
# DEBUG
# CFLAGS  := -g \
	   -Wno-unused-label \
	   -Wno-unknown-pragmas \
	   -pthread \
	   -std=c++17 \
	   -Wall \
	   -O0 \
	   -I$(ARGPARS)/include \
	   -I${INCDIR} \
	   -isystem ${XILINX_HLS}/include \
	   -I${XILINX_XRT}/include $(ROOTCFLAGS)
# RELEASE
CFLAGS  := \
	   -Wno-unused-label \
	   -Wno-unknown-pragmas \
	   -pthread \
	   -std=c++17 \
	   -O3 \
	   -I$(ARGPARS)/include \
	   -I${INCDIR} \
	   -isystem ${XILINX_HLS}/include \
	   -I${XILINX_XRT}/include $(ROOTCFLAGS)

# For V++
VV = v++

#======================
#
# Targets
#
#======================

# Compile with G++
TARGET: ${OBJDIR}/host.o ${OBJDIR}/kernels.o ${OBJDIR}/kernels.xo ${BINDIR}/kernels.xclbin all
	@echo "[MAKE][$$(date +%H:%M:%S)] compile binary"
	$(CC) $(CFLAGS) ${OBJDIR}/host.o ${OBJDIR}/kernels.o -o ${BINDIR}/${EXE} $(LDFLAGS)

${OBJDIR}/kernels.o: ${SRCDIR}/kernels.cpp
	@echo "[MAKE][$$(date +%H:%M:%S)] compile kernel"
	$(CC) $(CFLAGS) -c ${SRCDIR}/kernels.cpp -o ${OBJDIR}/kernels.o

${OBJDIR}/host.o: ${SRCDIR}/host.cpp
	@echo "[MAKE][$$(date +%H:%M:%S)] compile host"
	$(CC) $(CFLAGS) -c ${SRCDIR}/host.cpp -o ${OBJDIR}/host.o

# Compile with V++
# DEBUG
# ${BINDIR}/kernels.xclbin: ${OBJDIR}/kernels.xo
# 	@echo "[MAKE][$$(date +%H:%M:%S)] compile .xclbin"
# 	$(VV) -g -l -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/u280.cfg \
# 	       	${OBJDIR}/kernels.xo -o ${BINDIR}/kernels.xclbin
# 	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
# 	@mv _x ${FPGA_PATH2EMU}/
# 	@mv *.log $(LOGDIR)/
# ${OBJDIR}/kernels.xo: ${FPGA_PATH2SRC}/kernels.cpp
# 	@echo "[MAKE][$$(date +%H:%M:%S)] compile .xo"
# 	$(VV) -g -c -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/u280.cfg \
# 	       	-k CCL \
# 	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/kernels.cpp -o ${OBJDIR}/kernels.xo
# RELEASE
${BINDIR}/kernels.xclbin: ${OBJDIR}/kernels.xo
	@echo "[MAKE][$$(date +%H:%M:%S)] compile .xclbin"
	$(VV) -l -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/u280.cfg \
			--optimize 3 \
	       	${OBJDIR}/kernels.xo -o ${BINDIR}/kernels.xclbin
	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
	@mv _x ${FPGA_PATH2EMU}/
	@mv *.log $(LOGDIR)/
${OBJDIR}/kernels.xo: ${FPGA_PATH2SRC}/kernels.cpp
	@echo "[MAKE][$$(date +%H:%M:%S)] compile .xo"
	$(VV) -c -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/u280.cfg \
	       	-k CCL \
	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/kernels.cpp -o ${OBJDIR}/kernels.xo

#
# Compile TEST files
#
test: ${OBJDIR}/test_host.o ${OBJDIR}/test_kernels.o ${OBJDIR}/test_kernels.xo ${BINDIR}/test_kernels.xclbin all
	@echo "[MAKE TEST][$$(date +%H:%M:%S)] compile binary"
	$(CC) $(CFLAGS) ${OBJDIR}/test_host.o ${OBJDIR}/test_kernels.o -o ${BINDIR}/${EXE} $(LDFLAGS)

${OBJDIR}/test_kernels.o: ${SRCDIR}/test_kernels.cpp
	@echo "[MAKE TEST][$$(date +%H:%M:%S)] compile kernel"
	$(CC) $(CFLAGS) -c ${SRCDIR}/test_kernels.cpp -o ${OBJDIR}/test_kernels.o

${OBJDIR}/test_host.o: ${SRCDIR}/test_host.cpp
	@echo "[MAKE TEST][$$(date +%H:%M:%S)] compile host"
	$(CC) $(CFLAGS) -c ${SRCDIR}/test_host.cpp -o ${OBJDIR}/test_host.o

# Compile with V++
# ${BINDIR}/test_kernels.xclbin: ${OBJDIR}/test_kernels.xo
# 	@echo "[MAKE TEST][$$(date +%H:%M:%S)] compile .xclbin"
# 	$(VV) -g -l -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/test_u280.cfg \
# 	       	${OBJDIR}/test_kernels.xo -o ${BINDIR}/test_kernels.xclbin
# 	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
# 	@mv _x ${FPGA_PATH2EMU}/
# 	@mv *.log $(LOGDIR)/
# ${OBJDIR}/test_kernels.xo: ${FPGA_PATH2SRC}/test_kernels.cpp
# 	@echo "[MAKE TEST][$$(date +%H:%M:%S)] compile .xo"
# 	$(VV) -g -c -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/test_u280.cfg \
# 	       	-k CCL \
# 	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/test_kernels.cpp -o ${OBJDIR}/test_kernels.xo
# RELEASE
${BINDIR}/test_kernels.xclbin: ${OBJDIR}/test_kernels.xo
	@echo "[MAKE TEST][$$(date +%H:%M:%S)] compile .xclbin"
	$(VV) -l -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/test_u280.cfg \
			--optimize 3 \
	       	${OBJDIR}/test_kernels.xo -o ${BINDIR}/test_kernels.xclbin
	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
	@mv _x ${FPGA_PATH2EMU}/
	@mv *.log $(LOGDIR)/
${OBJDIR}/test_kernels.xo: ${FPGA_PATH2SRC}/test_kernels.cpp
	@echo "[MAKE TEST][$$(date +%H:%M:%S)] compile .xo"
	$(VV) -c -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/test_u280.cfg \
	       	-k CCL \
	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/test_kernels.cpp -o ${OBJDIR}/test_kernels.xo

#
# Compile DDR files
#
ddr: ${OBJDIR}/ddr_host.o ${OBJDIR}/ddr_kernels.o ${OBJDIR}/ddr_kernels.xo ${BINDIR}/ddr_kernels.xclbin all
	@echo "[MAKE DDR][$$(date +%H:%M:%S)] compile binary"
	$(CC) $(CFLAGS) ${OBJDIR}/ddr_host.o ${OBJDIR}/ddr_kernels.o -o ${BINDIR}/${EXE} $(LDFLAGS)

${OBJDIR}/ddr_kernels.o: ${SRCDIR}/ddr_kernels.cpp
	@echo "[MAKE DDR][$$(date +%H:%M:%S)] compile kernel"
	$(CC) $(CFLAGS) -c ${SRCDIR}/ddr_kernels.cpp -o ${OBJDIR}/ddr_kernels.o

${OBJDIR}/ddr_host.o: ${SRCDIR}/ddr_host.cpp
	@echo "[MAKE DDR][$$(date +%H:%M:%S)] compile host"
	$(CC) $(CFLAGS) -c ${SRCDIR}/ddr_host.cpp -o ${OBJDIR}/ddr_host.o

# Compile with V++
# ${BINDIR}/ddr_kernels.xclbin: ${OBJDIR}/ddr_kernels.xo
# 	@echo "[MAKE DDR][$$(date +%H:%M:%S)] compile .xclbin"
# 	$(VV) -g -l -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/ddr_u280.cfg \
# 	       	${OBJDIR}/ddr_kernels.xo -o ${BINDIR}/ddr_kernels.xclbin
# 	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
# 	@mv _x ${FPGA_PATH2EMU}/
# 	@mv *.log $(LOGDIR)/
# ${OBJDIR}/ddr_kernels.xo: ${FPGA_PATH2SRC}/ddr_kernels.cpp
# 	@echo "[MAKE DDR][$$(date +%H:%M:%S)] compile .xo"
# 	$(VV) -g -c -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/ddr_u280.cfg \
# 	       	-k CCL \
# 	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/ddr_kernels.cpp -o ${OBJDIR}/ddr_kernels.xo
# RELEASE
${BINDIR}/ddr_kernels.xclbin: ${OBJDIR}/ddr_kernels.xo
	@echo "[MAKE DDR][$$(date +%H:%M:%S)] compile .xclbin"
	$(VV) -l -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/ddr_u280.cfg \
			--optimize 3 \
	       	${OBJDIR}/ddr_kernels.xo -o ${BINDIR}/ddr_kernels.xclbin
	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
	@mv _x ${FPGA_PATH2EMU}/
	@mv *.log $(LOGDIR)/
${OBJDIR}/ddr_kernels.xo: ${FPGA_PATH2SRC}/ddr_kernels.cpp
	@echo "[MAKE DDR][$$(date +%H:%M:%S)] compile .xo"
	$(VV) -c -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/ddr_u280.cfg \
	       	-k CCL \
	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/ddr_kernels.cpp -o ${OBJDIR}/ddr_kernels.xo

#
# Compile PAR files
#
par: ${OBJDIR}/par_host.o ${OBJDIR}/par_kernels.o ${OBJDIR}/par_kernels.xo ${BINDIR}/par_kernels.xclbin all
	@echo "[MAKE PAR][$$(date +%H:%M:%S)] compile binary"
	$(CC) $(CFLAGS) ${OBJDIR}/par_host.o ${OBJDIR}/par_kernels.o -o ${BINDIR}/${EXE} $(LDFLAGS)

${OBJDIR}/par_kernels.o: ${SRCDIR}/par_kernels.cpp
	@echo "[MAKE PAR][$$(date +%H:%M:%S)] compile kernel"
	$(CC) $(CFLAGS) -c ${SRCDIR}/par_kernels.cpp -o ${OBJDIR}/par_kernels.o

${OBJDIR}/par_host.o: ${SRCDIR}/par_host.cpp
	@echo "[MAKE PAR][$$(date +%H:%M:%S)] compile host"
	$(CC) $(CFLAGS) -c ${SRCDIR}/par_host.cpp -o ${OBJDIR}/par_host.o

# Compile with V++
# ${BINDIR}/par_kernels.xclbin: ${OBJDIR}/par_kernels.xo
# 	@echo "[MAKE PAR][$$(date +%H:%M:%S)] compile .xclbin"
# 	$(VV) -g -l -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/par_u280.cfg \
# 	       	${OBJDIR}/par_kernels.xo -o ${BINDIR}/par_kernels.xclbin
# 	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
# 	@mv _x ${FPGA_PATH2EMU}/
# 	@mv *.log $(LOGDIR)/
# ${OBJDIR}/par_kernels.xo: ${FPGA_PATH2SRC}/par_kernels.cpp
# 	@echo "[MAKE PAR][$$(date +%H:%M:%S)] compile .xo"
# 	$(VV) -g -c -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/par_u280.cfg \
# 	       	-k CCL \
# 	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/par_kernels.cpp -o ${OBJDIR}/par_kernels.xo
# RELEASE
${BINDIR}/par_kernels.xclbin: ${OBJDIR}/par_kernels.xo
	@echo "[MAKE PAR][$$(date +%H:%M:%S)] compile .xclbin"
	$(VV) -l -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/par_u280.cfg \
			--optimize 3 \
	       	${OBJDIR}/par_kernels.xo -o ${BINDIR}/par_kernels.xclbin
	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
	@mv _x ${FPGA_PATH2EMU}/
	@mv *.log $(LOGDIR)/
${OBJDIR}/par_kernels.xo: ${FPGA_PATH2SRC}/par_kernels.cpp
	@echo "[MAKE PAR][$$(date +%H:%M:%S)] compile .xo"
	$(VV) -c -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/par_u280.cfg \
	       	-k CCL \
	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/par_kernels.cpp -o ${OBJDIR}/par_kernels.xo

#
# Compile LARGE files
#
large: ${OBJDIR}/large_host.o ${OBJDIR}/large_kernels.o ${OBJDIR}/large_kernels.xo ${BINDIR}/large_kernels.xclbin all
	@echo "[MAKE LARGE][$$(date +%H:%M:%S)] compile binary"
	$(CC) $(CFLAGS) ${OBJDIR}/large_host.o ${OBJDIR}/large_kernels.o -o ${BINDIR}/${EXE} $(LDFLAGS)

${OBJDIR}/large_kernels.o: ${SRCDIR}/large_kernels.cpp
	@echo "[MAKE LARGE][$$(date +%H:%M:%S)] compile kernel"
	$(CC) $(CFLAGS) -c ${SRCDIR}/large_kernels.cpp -o ${OBJDIR}/large_kernels.o

${OBJDIR}/large_host.o: ${SRCDIR}/large_host.cpp
	@echo "[MAKE LARGE][$$(date +%H:%M:%S)] compile host"
	$(CC) $(CFLAGS) -c ${SRCDIR}/large_host.cpp -o ${OBJDIR}/large_host.o

# Compile with V++
# ${BINDIR}/large_kernels.xclbin: ${OBJDIR}/large_kernels.xo
# 	@echo "[MAKE LARGE][$$(date +%H:%M:%S)] compile .xclbin"
# 	$(VV) -g -l -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/large_u280.cfg \
# 	       	${OBJDIR}/large_kernels.xo -o ${BINDIR}/large_kernels.xclbin
# 	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
# 	@mv _x ${FPGA_PATH2EMU}/
# 	@mv *.log $(LOGDIR)/
# ${OBJDIR}/large_kernels.xo: ${FPGA_PATH2SRC}/large_kernels.cpp
# 	@echo "[MAKE LARGE][$$(date +%H:%M:%S)] compile .xo"
# 	$(VV) -g -c -t ${XCL_EMULATION_MODE_CHANGED} \
# 	       	--platform ${FPGA_PLATFORM} \
# 	       	--config ${FPGA_PATH2CONF}/large_u280.cfg \
# 	       	-k CCL \
# 	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/large_kernels.cpp -o ${OBJDIR}/large_kernels.xo
# RELEASE
${BINDIR}/large_kernels.xclbin: ${OBJDIR}/large_kernels.xo
	@echo "[MAKE LARGE][$$(date +%H:%M:%S)] compile .xclbin"
	$(VV) -l -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/large_u280.cfg \
			--optimize 3 \
	       	${OBJDIR}/large_kernels.xo -o ${BINDIR}/large_kernels.xclbin
	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
	@mv _x ${FPGA_PATH2EMU}/
	@mv *.log $(LOGDIR)/
${OBJDIR}/large_kernels.xo: ${FPGA_PATH2SRC}/large_kernels.cpp
	@echo "[MAKE LARGE][$$(date +%H:%M:%S)] compile .xo"
	$(VV) -c -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/large_u280.cfg \
	       	-k CCL \
	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/large_kernels.cpp -o ${OBJDIR}/large_kernels.xo

# TODO GET RID OF LOG FILES
all: 
#	@mv *.log $(LOGDIR)/.
#	@mv *.json $(ETCDIR)/.
#	@mv -f _x $(HOMEDIR)/vpp_out

clean:
	@rm -rf *.log out/* _x xilinx* *.csv xrt.run_summary *.str *.jou .Xil
