#======================
#
# Targets and config
#
#======================

# Binary name
EXE=app_${XCL_EMULATION_MODE}

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
CFLAGS  := -g -Wno-unknown-pragmas \
	   -pthread \
	   -std=c++17 \
	   -Wall \
	   -O0 \
	   -I$(ARGPARS)/include \
	   -I${INCDIR} \
	   -I${XILINX_HLS}/include \
	   -I${XILINX_XRT}/include $(ROOTCFLAGS) #-g -Wall

# For V++
VV = v++

#======================
#
# Targets
#
#======================

# Compile with G++
TARGET: ${OBJDIR}/host.o ${OBJDIR}/kernels.o ${OBJDIR}/kernels.xo ${BINDIR}/kernels.xclbin all
	$(CC) $(CFLAGS) ${OBJDIR}/host.o ${OBJDIR}/kernels.o -o ${BINDIR}/${EXE} $(LDFLAGS)
	@rm -rf ${FPGA_PATH2EMU}/_x .Xil
	@mv _x ${FPGA_PATH2EMU}/
	@mv *.log $(LOGDIR)/

${OBJDIR}/kernels.o: ${SRCDIR}/kernels.cpp
	$(CC) $(CFLAGS) -c ${SRCDIR}/kernels.cpp -o ${OBJDIR}/kernels.o

${OBJDIR}/host.o: ${SRCDIR}/host.cpp
	$(CC) $(CFLAGS) -c ${SRCDIR}/host.cpp -o ${OBJDIR}/host.o

# Compile with V++
${BINDIR}/kernels.xclbin: ${OBJDIR}/kernels.xo
	$(VV) -g -l -t ${XCL_EMULATION_MODE} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/u280.cfg \
	       	${OBJDIR}/kernels.xo -o ${BINDIR}/kernels.xclbin

${OBJDIR}/kernels.xo: ${FPGA_PATH2SRC}/kernels.cpp
	$(VV) -g -c -t ${XCL_EMULATION_MODE} \
	       	--platform ${FPGA_PLATFORM} \
	       	--config ${FPGA_PATH2CONF}/u280.cfg \
	       	-k CCL \
	       	-I ${FPGA_PATH2INC} ${FPGA_PATH2SRC}/kernels.cpp -o ${OBJDIR}/kernels.xo

# TODO GET RID OF LOG FILES
all: 
#	@mv *.log $(LOGDIR)/.
#	@mv *.json $(ETCDIR)/.
#	@mv -f _x $(HOMEDIR)/vpp_out

clean:
	@rm -rf *.log out/* _x xilinx* *.csv xrt.run_summary *.str *.jou
