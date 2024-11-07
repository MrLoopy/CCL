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
CFLAGS  := -g \
	   -Wno-unused-label \
	   -Wno-unknown-pragmas \
	   -pthread \
	   -std=c++17 \
	   -Wall \
	   -O0 \
	   -I$(ARGPARS)/include \
	   -I${INCDIR} \
	   -isystem ${XILINX_HLS}/include \
	   -I${XILINX_XRT}/include $(ROOTCFLAGS) #-g -Wall
#	    -I${XILINX_HLS}/include \

# For V++
VV = v++

#======================
#
# Targets
#
#======================

# Compile with G++
TARGET: ${OBJDIR}/host.o ${OBJDIR}/kernels.o ${OBJDIR}/kernels.xo ${BINDIR}/kernels.xclbin all
	@echo "[MAKE] compile binary"
	$(CC) $(CFLAGS) ${OBJDIR}/host.o ${OBJDIR}/kernels.o -o ${BINDIR}/${EXE} $(LDFLAGS)

${OBJDIR}/kernels.o: ${SRCDIR}/kernels.cpp
	@echo "[MAKE] compile kernel"
	$(CC) $(CFLAGS) -c ${SRCDIR}/kernels.cpp -o ${OBJDIR}/kernels.o

${OBJDIR}/host.o: ${SRCDIR}/host.cpp
	@echo "[MAKE] compile host"
	$(CC) $(CFLAGS) -c ${SRCDIR}/host.cpp -o ${OBJDIR}/host.o

# Compile with V++
${BINDIR}/kernels.xclbin: ${OBJDIR}/kernels.xo
	@echo "[MAKE] compile .xclbin"
	$(VV) -g -l -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/u280.cfg \
	       	${OBJDIR}/kernels.xo -o ${BINDIR}/kernels.xclbin
	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
	@mv _x ${FPGA_PATH2EMU}/
	@mv *.log $(LOGDIR)/

${OBJDIR}/kernels.xo: ${FPGA_PATH2SRC}/kernels.cpp
	@echo "[MAKE] compile .xo"
	$(VV) -g -c -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/u280.cfg \
	       	-k CCL \
	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/kernels.cpp -o ${OBJDIR}/kernels.xo

#
# Compile alternative files
#
test: ${OBJDIR}/test_host.o ${OBJDIR}/test_kernels.o ${OBJDIR}/test_kernels.xo ${BINDIR}/test_kernels.xclbin all
	@echo "[MAKE TEST] compile binary"
	$(CC) $(CFLAGS) ${OBJDIR}/test_host.o ${OBJDIR}/test_kernels.o -o ${BINDIR}/${EXE} $(LDFLAGS)

${OBJDIR}/test_kernels.o: ${SRCDIR}/test_kernels.cpp
	@echo "[MAKE TEST] compile kernel"
	$(CC) $(CFLAGS) -c ${SRCDIR}/test_kernels.cpp -o ${OBJDIR}/test_kernels.o

${OBJDIR}/test_host.o: ${SRCDIR}/test_host.cpp
	@echo "[MAKE TEST] compile host"
	$(CC) $(CFLAGS) -c ${SRCDIR}/test_host.cpp -o ${OBJDIR}/test_host.o

# Compile with V++
${BINDIR}/test_kernels.xclbin: ${OBJDIR}/test_kernels.xo
	@echo "[MAKE TEST] compile .xclbin"
	$(VV) -g -l -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/test_u280.cfg \
	       	${OBJDIR}/test_kernels.xo -o ${BINDIR}/test_kernels.xclbin
	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
	@mv _x ${FPGA_PATH2EMU}/
	@mv *.log $(LOGDIR)/

${OBJDIR}/test_kernels.xo: ${FPGA_PATH2SRC}/test_kernels.cpp
	@echo "[MAKE TEST] compile .xo"
	$(VV) -g -c -t ${XCL_EMULATION_MODE_CHANGED} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/test_u280.cfg \
	       	-k CCL \
	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/test_kernels.cpp -o ${OBJDIR}/test_kernels.xo

# TODO GET RID OF LOG FILES
all: 
#	@mv *.log $(LOGDIR)/.
#	@mv *.json $(ETCDIR)/.
#	@mv -f _x $(HOMEDIR)/vpp_out

clean:
	@rm -rf *.log out/* _x xilinx* *.csv xrt.run_summary *.str *.jou .Xil
