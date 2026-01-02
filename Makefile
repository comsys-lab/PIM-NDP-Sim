# --------------------------------------------------
# DEBUG CONFIGURATION
# Usage: make (release, default) or make DEBUG=1 (debug)
# --------------------------------------------------
DEBUG ?= 0

ifeq ($(DEBUG), 1)
    # Debug Mode
    BUILD_TYPE := Debug
    OPT_FLAGS := -O0 -g3 -ggdb -fno-inline
    CFLAGS_DEFS := -DDEBUG_BUILD
    BUILD_DIR := build-debug
    PROG_NAME := pim-ndp-sim-debug
else
    # Release Mode
    BUILD_TYPE := Release
    OPT_FLAGS := -O2 -g
    CFLAGS_DEFS :=
    BUILD_DIR := build
    PROG_NAME := pim-ndp-sim
endif

# --------------------------------------------------
# BUILD DIRECTORY CONFIGURATION
# --------------------------------------------------
SRC_DIR := src

# Set the XLEN value
CONFIG_XLEN=64

# if set, network filesystem is enabled. libcurl and libcrypto
# (openssl) must be installed.
CONFIG_FS_NET=y

# SDL support (optional)
CONFIG_SDL=y

# user space network redirector
CONFIG_SLIRP=y

CROSS_PREFIX=
EXE=
CC=$(CROSS_PREFIX)gcc
CXX=$(CROSS_PREFIX)g++
STRIP=$(CROSS_PREFIX)strip

# UPDATED: Use OPT_FLAGS instead of hardcoded -O2 -g
CFLAGS=$(OPT_FLAGS) -Wall -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -MMD
CFLAGS+=-D_GNU_SOURCE -DCONFIG_VERSION=\"$(shell cat $(SRC_DIR)/VERSION)\"
CFLAGS+=-DMAX_XLEN=$(CONFIG_XLEN) $(CFLAGS_DEFS)
LDFLAGS=

PROGS+= $(BUILD_DIR)/$(PROG_NAME)$(EXE) $(BUILD_DIR)/sim-stats-display
ifdef CONFIG_FS_NET
PROGS+=$(BUILD_DIR)/build_filelist $(BUILD_DIR)/splitimg
endif

# --------------------------------------------------
# DRAMSIM3 connections for the simulator
# --------------------------------------------------
DRAMSIM3_WRAPPER_C_CONNECTOR_LIB+=$(BUILD_DIR)/libdramsim_wrapper_c_connector.so
DRAMSIM3_WRAPPER_C_CONNECTOR_OBJ:=$(BUILD_DIR)/obj/riscvsim/memory_hierarchy/dramsim_wrapper_c_connector.o
DRAMSIM3_WRAPPER_LIB+=$(BUILD_DIR)/libdramsim_wrapper_lib.so
DRAMSIM3_WRAPPER_OBJ:=$(BUILD_DIR)/obj/riscvsim/memory_hierarchy/dramsim_wrapper.o
DRAMSIM3_LIB_SO+=$(SRC_DIR)/DRAMsim3/libdramsim3.so
DRAMSIM3_FMT_LIB_DIR=$(SRC_DIR)/DRAMsim3/ext/fmt/include
DRAMSIM3_INI_LIB_DIR=$(SRC_DIR)/DRAMsim3/ext/headers
DRAMSIM3_JSON_LIB_DIR=$(SRC_DIR)/DRAMsim3/ext/headers
DRAMSIM3_ARGS_LIB_DIR=$(SRC_DIR)/DRAMsim3/ext/headers
DRAMSIM3_INC=-I$(SRC_DIR)/DRAMsim3/src/ -I$(DRAMSIM3_FMT_LIB_DIR) -I$(DRAMSIM3_INI_LIB_DIR) -I$(DRAMSIM3_ARGS_LIB_DIR) -I$(DRAMSIM3_JSON_LIB_DIR)

# --------------------------------------------------
# Ramulator connections for the simulator
# --------------------------------------------------
RAMULATOR_WRAPPER_C_CONNECTOR_LIB+=$(BUILD_DIR)/libramulator_wrapper_c_connector.so
RAMULATOR_WRAPPER_C_CONNECTOR_OBJ:=$(BUILD_DIR)/obj/riscvsim/memory_hierarchy/ramulator_wrapper_c_connector.o
RAMULATOR_WRAPPER_LIB+=$(BUILD_DIR)/libramulator_wrapper_lib.so
RAMULATOR_WRAPPER_OBJ:=$(BUILD_DIR)/obj/riscvsim/memory_hierarchy/ramulator_wrapper.o
RAMULATOR_LIB_SO+=$(SRC_DIR)/ramulator/libramulator.so
RAMULATOR_INC+=-I$(SRC_DIR)/ramulator/src

# --------------------------------------------------
# Custom AiMulator using CMake Integration
# --------------------------------------------------
AIMULATOR_DIR = $(SRC_DIR)/AiMulator
AIMULATOR_LIB_NAME = libaimulator.so
AIMULATOR_LIB_SO = $(AIMULATOR_DIR)/$(AIMULATOR_LIB_NAME)
AIMULATOR_WRAPPER_LIB_NAME = libaimulator_wrapper_lib.so
AIMULATOR_WRAPPER_LIB = $(BUILD_DIR)/$(AIMULATOR_WRAPPER_LIB_NAME)
AIMULATOR_WRAPPER_OBJ = $(BUILD_DIR)/obj/riscvsim/memory_hierarchy/aimulator_wrapper.o
AIMULATOR_C_CONNECTOR_LIB_NAME = libaimulator_wrapper_c_connector.so
AIMULATOR_C_CONNECTOR_LIB = $(BUILD_DIR)/$(AIMULATOR_C_CONNECTOR_LIB_NAME)
AIMULATOR_C_CONNECTOR_OBJ = $(BUILD_DIR)/obj/riscvsim/memory_hierarchy/aimulator_wrapper_c_connector.o
AIMULATOR_INCLUDE = -I$(AIMULATOR_DIR)/src -I$(AIMULATOR_DIR)/ext/spdlog/include -I$(AIMULATOR_DIR)/ext/yaml-cpp/include -I$(AIMULATOR_DIR)/ext/argparse/include

# Top-level simulator object file
SIM_OBJ_FILE=$(BUILD_DIR)/obj/riscvsim.o

# Simulator object files for each module
SIM_UTILS:=$(addprefix $(BUILD_DIR)/obj/riscvsim/utils/, sim_exception.o sim_trace.o cpu_latches.o evict_policy.o circular_queue.o sim_params.o sim_stats.o sim_log.o)
SIM_DECODER_OBJS:=$(addprefix $(BUILD_DIR)/obj/riscvsim/decoder/, riscv_isa_string_generator.o riscv_isa_decoder.o riscv_isa_execute.o)
SIM_BPU_OBJS:=$(addprefix $(BUILD_DIR)/obj/riscvsim/bpu/, ras.o bht.o btb.o adaptive_predictor.o bpu.o)
SIM_MEM_HY_OBJS:=$(addprefix $(BUILD_DIR)/obj/riscvsim/memory_hierarchy/, temu_mem_map_wrapper.o dram.o memory_hierarchy.o memory_controller.o cache.o )
SIM_IN_CORE_OBJS:=$(addprefix $(BUILD_DIR)/obj/riscvsim/core/, inorder_frontend.o inorder_backend.o inorder.o)
SIM_CORE_OBJS:=$(addprefix $(BUILD_DIR)/obj/riscvsim/core/, riscv_sim_cpu.o)
SIM_OO_CORE_OBJS:=$(addprefix $(BUILD_DIR)/obj/riscvsim/core/, ooo_frontend.o ooo_branch.o ooo_lsu.o ooo_backend.o ooo.o)
SIM_OBJS:=$(SIM_UTILS) $(SIM_DECODER_OBJS) $(SIM_BPU_OBJS) $(SIM_MEM_HY_OBJS) $(SIM_CORE_OBJS) $(SIM_IN_CORE_OBJS) $(SIM_OO_CORE_OBJS)

.DEFAULT_GOAL := all

all: $(PROGS)

EMU_OBJS:=$(addprefix $(BUILD_DIR)/obj/, virtio.o pci.o fs.o cutils.o iomem.o simplefb.o \
    json.o machine.o rtc_timer.o temu.o)

ifdef CONFIG_SLIRP
CFLAGS+=-DCONFIG_SLIRP
EMU_OBJS+=$(addprefix $(BUILD_DIR)/obj/slirp/, bootp.o ip_icmp.o mbuf.o slirp.o tcp_output.o cksum.o ip_input.o misc.o socket.o tcp_subr.o udp.o if.o ip_output.o sbuf.o tcp_input.o tcp_timer.o)
endif

EMU_OBJS+=$(BUILD_DIR)/obj/fs_disk.o
EMU_LIBS=-lrt -lpthread -lm

ifdef CONFIG_FS_NET
CFLAGS+=-DCONFIG_FS_NET
EMU_OBJS+=$(addprefix $(BUILD_DIR)/obj/, fs_net.o fs_wget.o fs_utils.o block_net.o)
EMU_LIBS+=-lcurl -lcrypto
endif # CONFIG_FS_NET

ifdef CONFIG_SDL
EMU_LIBS+=-lSDL
EMU_OBJS+=$(BUILD_DIR)/obj/sdl.o
CFLAGS+=-DCONFIG_SDL
endif

EMU_OBJS+=$(addprefix $(BUILD_DIR)/obj/, riscv_machine.o softfp.o riscv_cpu.o)
CFLAGS+=-DCONFIG_RISCV_MAX_XLEN=64

# --------------------------------------------------
# Build rules
# --------------------------------------------------

# Create build directories
$(shell mkdir -p $(BUILD_DIR)/obj/riscvsim/utils)
$(shell mkdir -p $(BUILD_DIR)/obj/riscvsim/decoder)
$(shell mkdir -p $(BUILD_DIR)/obj/riscvsim/bpu)
$(shell mkdir -p $(BUILD_DIR)/obj/riscvsim/memory_hierarchy)
$(shell mkdir -p $(BUILD_DIR)/obj/riscvsim/core)
$(shell mkdir -p $(BUILD_DIR)/obj/slirp)
$(shell mkdir -p log)

$(DRAMSIM3_WRAPPER_C_CONNECTOR_LIB): $(DRAMSIM3_WRAPPER_LIB) $(DRAMSIM3_WRAPPER_C_CONNECTOR_OBJ)
	$(CXX) -shared -o $(DRAMSIM3_WRAPPER_C_CONNECTOR_LIB) $(DRAMSIM3_WRAPPER_C_CONNECTOR_OBJ) -L$(BUILD_DIR) -ldramsim_wrapper_lib -Wl,-rpath=$(BUILD_DIR)

$(DRAMSIM3_WRAPPER_LIB): $(DRAMSIM3_LIB_SO) $(DRAMSIM3_WRAPPER_OBJ)
	$(CXX) -shared -o $(DRAMSIM3_WRAPPER_LIB) $(DRAMSIM3_WRAPPER_OBJ) -L$(SRC_DIR)/DRAMsim3 -ldramsim3 -Wl,-rpath=$(SRC_DIR)/DRAMsim3

$(DRAMSIM3_LIB_SO):
	cd $(SRC_DIR)/DRAMsim3/ && $(MAKE) libdramsim3.so && cd ../..

$(RAMULATOR_WRAPPER_C_CONNECTOR_LIB): $(RAMULATOR_WRAPPER_LIB) $(RAMULATOR_WRAPPER_C_CONNECTOR_OBJ)
	$(CXX) -shared -o $(RAMULATOR_WRAPPER_C_CONNECTOR_LIB) $(RAMULATOR_WRAPPER_C_CONNECTOR_OBJ) -L$(BUILD_DIR) -lramulator_wrapper_lib -Wl,-rpath=$(BUILD_DIR)

$(RAMULATOR_WRAPPER_LIB): $(RAMULATOR_LIB_SO) $(RAMULATOR_WRAPPER_OBJ)
	$(CXX) -shared -o $(RAMULATOR_WRAPPER_LIB) $(RAMULATOR_WRAPPER_OBJ) -L$(SRC_DIR)/ramulator -lramulator -Wl,-rpath=$(SRC_DIR)/ramulator

$(RAMULATOR_LIB_SO):
	cd $(SRC_DIR)/ramulator && $(MAKE) libramulator.so && cd ../..

# Custom Ramulator 2 build rules
$(AIMULATOR_LIB_SO):
	mkdir -p $(AIMULATOR_DIR)/build
	cd $(AIMULATOR_DIR)/build && cmake .. -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) && $(MAKE) -j$(shell nproc)
	@if [ ! -f $(AIMULATOR_LIB_SO) ]; then echo "Error: $(AIMULATOR_LIB_SO) not found after build"; exit 1; fi

$(AIMULATOR_WRAPPER_OBJ): $(SRC_DIR)/riscvsim/memory_hierarchy/aimulator_wrapper.cpp $(AIMULATOR_LIB_SO)
	@mkdir -p $(dir $@)
	$(CXX) $(OPT_FLAGS) -DMAX_XLEN=$(CONFIG_XLEN) -fpic -shared -std=c++20 $(AIMULATOR_INCLUDE) -DSPDLOG_COMPILED_LIB -c -o $@ $<

$(AIMULATOR_C_CONNECTOR_OBJ): $(SRC_DIR)/riscvsim/memory_hierarchy/aimulator_wrapper_c_connector.cpp $(AIMULATOR_LIB_SO)
	@mkdir -p $(dir $@)
	$(CXX) $(OPT_FLAGS) -DMAX_XLEN=$(CONFIG_XLEN) -fpic -shared -std=c++20 $(AIMULATOR_INCLUDE) -DSPDLOG_COMPILED_LIB -c -o $@ $<

$(AIMULATOR_WRAPPER_LIB): $(AIMULATOR_LIB_SO) $(AIMULATOR_WRAPPER_OBJ)
	$(CXX) -shared -o $@ $(AIMULATOR_WRAPPER_OBJ) -L$(AIMULATOR_DIR) -laimulator -Wl,-rpath=$(AIMULATOR_DIR)

$(AIMULATOR_C_CONNECTOR_LIB): $(AIMULATOR_WRAPPER_LIB) $(AIMULATOR_C_CONNECTOR_OBJ)
	$(CXX) -shared -o $@ $(AIMULATOR_C_CONNECTOR_OBJ) -L$(BUILD_DIR) -laimulator_wrapper_lib -Wl,-rpath=$(BUILD_DIR)

# --------------------------------------------------
# Main Executable
# --------------------------------------------------
$(SIM_OBJ_FILE): $(SIM_OBJS)
	$(LD) -r -o $@ $^ $(SIM_LIBS)

$(BUILD_DIR)/sim-stats-display: $(BUILD_DIR)/obj/stats_display.o
	$(CC) -o $(BUILD_DIR)/sim-stats-display $(BUILD_DIR)/obj/stats_display.o -lrt

$(BUILD_DIR)/$(PROG_NAME)$(EXE): $(SIM_OBJ_FILE) $(DRAMSIM3_WRAPPER_C_CONNECTOR_LIB) $(RAMULATOR_WRAPPER_C_CONNECTOR_LIB) $(AIMULATOR_C_CONNECTOR_LIB) $(EMU_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(EMU_LIBS) -L$(BUILD_DIR) -ldramsim_wrapper_c_connector -Wl,-rpath=$(BUILD_DIR) -L$(BUILD_DIR) -lramulator_wrapper_c_connector -Wl,-rpath=$(BUILD_DIR) -L$(BUILD_DIR) -laimulator_wrapper_c_connector -Wl,-rpath=$(BUILD_DIR)

$(BUILD_DIR)/obj/riscv_cpu.o: $(SRC_DIR)/riscv_cpu.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DMAX_XLEN=$(CONFIG_XLEN) -c -o $@ $<

$(BUILD_DIR)/build_filelist: $(BUILD_DIR)/obj/build_filelist.o $(BUILD_DIR)/obj/fs_utils.o $(BUILD_DIR)/obj/cutils.o
	$(CC) $(LDFLAGS) -o $@ $^ -lm

$(BUILD_DIR)/splitimg: $(BUILD_DIR)/obj/splitimg.o
	$(CC) $(LDFLAGS) -o $@ $^

# Pattern rules for C files
$(BUILD_DIR)/obj/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/obj/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(OPT_FLAGS) -DMAX_XLEN=$(CONFIG_XLEN) $(DRAMSIM3_INC) $(RAMULATOR_INC) -fpic -shared -c -std=c++11 -o $@ $<

# Add separate pattern rule for slirp directory (if it's outside src/)
$(BUILD_DIR)/obj/slirp/%.o: slirp/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(RAMULATOR_WRAPPER_C_CONNECTOR_OBJ) $(RAMULATOR_WRAPPER_OBJ)
	cd $(SRC_DIR)/ramulator && $(MAKE) clean && cd ../..
	rm -f $(DRAMSIM3_WRAPPER_C_CONNECTOR_OBJ) $(DRAMSIM3_WRAPPER_OBJ)
	cd $(SRC_DIR)/DRAMsim3 && $(MAKE) clean && cd ../..
	rm -f $(AIMULATOR_C_CONNECTOR_OBJ) $(AIMULATOR_WRAPPER_OBJ)
	rm -f $(AIMULATOR_WRAPPER_LIB) $(AIMULATOR_C_CONNECTOR_LIB)
	rm -rf $(AIMULATOR_DIR)/build
	rm -rf $(AIMULATOR_DIR)/ext
	rm -f $(AIMULATOR_LIB_SO)

-include $(wildcard $(BUILD_DIR)/obj/*.d)
-include $(wildcard $(BUILD_DIR)/obj/slirp/*.d)
-include $(wildcard $(BUILD_DIR)/obj/riscvsim/*/*.d)
