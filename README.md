# PIM-NDP-Sim

## Introduction
PIM-NDP-Sim is a simulator that provides simulation integrated a RISC-V near-data processing (NDP) core and processing-in-memory (PIM).
We extend the NDP simulation from [MARSS-RISCV](https://github.com/bucaps/marss-riscv), and incorporate [AiMulator](https://github.com/comsys-lab/AiMulator/tree/PIM-NDP-Sim) for PIM simulation by extending it from [Ramulator 2.0](https://github.com/CMU-SAFARI/ramulator2).

## Table of Contents
- [Build & Run](#build--run)
- [Simulation Overview](#simulation-overview)
- [Future Work](#future-work)
- [Contributors](#contributors)
- [Acknowledgment](#acknowledgment)
- [License](#license)

## Build & Run
The build tools have been modified to allow all integrated projects to be built simultaneously.
While the changes are minor, the core build process follows that of the base projects.

### Dependencies
The following dependencies (required by MARSS-RISCV) must be installed:
```console
$ sudo apt-get update
$ sudo apt-get install build-essential
$ sudo apt-get install libssl-dev
$ sudo apt-get install libsdl1.2-dev
$ sudo apt-get install libcurl4-openssl-dev
```

### Getting Started 
Clone the repository.
Since the PIM simulator is included as a submodule from [AiMulator](https://github.com/comsys-lab/AiMulator/tree/PIM-NDP-Sim), ensure you include the `--recursive` flag.
```console
$ git clone https://github.com/comsys-lab/PIM-NDP-Sim.git --recursive
$ cd ./PIM-NDP-Sim/
```

#### Build
To build the executable, run:
```console
$ make -j
```
The `Makefile` will automatically build the AiMulator submodule and create the necessary directories for execution.
To build a debug version, append `DEBUG=1` to the make command:
```console
$ make -j DEBUG=1
```
The debug build will be located in the `build-debug` directory.

#### Prerequisite before Run
Because the RISC-V simulation in MARSS-RISCV is built on top of [TinyEMU](https://github.com/dearchap/tinyemu), you must prepare the bootloader, kernel, and userland images.
Use the following commands to download and place them in the correct directory.
For further details, refer to the [MARSS-RISCV guide](https://github.com/bucaps/marss-riscv?tab=readme-ov-file#preparing-the-bootloader-kernel-and-userland-image).

```console
$ wget https://cs.binghamton.edu/~marss-riscv/marss-riscv-images.tar.gz
$ tar -xvzf marss-riscv-images.tar.gz
$ cd marss-riscv-images/riscv64-unknown-linux-gnu/
$ xz -d -k -T 0 riscv64.img.xz
$ cd ../..
$ cp -r marss-riscv-images/riscv64-unknown-linux-gnu/ configs/
```

#### Run
```console
$ cp build/pim-ndp-sim .
$ ./pim-ndp-sim -rw -ctrlc -sim-mem-model aimulator -sim-file-path log configs/riscv64_ndp_pim.cfg
```
Executing these commands initializes TinyEMU.
You can then develop kernel source code and execute it within the TinyEMU environment.
Detailed simulation methodologies are described in the following section.

*Note: The debug executable is named `pim-ndp-sim-debug`.*
*For additional runtime options, please refer to the [MARSS-RISCV documentation](https://github.com/bucaps/marss-riscv?tab=readme-ov-file#preparing-the-bootloader-kernel-and-userland-image).*

## Simulation Overview

<p align="center">
  <img width="800" src="./figures/simulator_overview.svg" alt="Simulator Overview">
</p>

The figure above illustrates the simulator architecture and simulation workflow.
We model NDP cores as host CPU cores; however, a PIM/NDP system can also be viewed as a host CPU equipped with FPGA-based accelerators.
Consequently, users can treat NDP kernels as standard programs running on the CPU. 
Note that we do not support bare-metal execution; the simulation environment requires GNU C libraries (glibc).

When running kernels in TinyEMU, the [MARSS-RISCV simulation mode](https://github.com/bucaps/marss-riscv?tab=readme-ov-file#running-full-system-simulations) must be enabled.
To do this, the kernel must include `SIM_START()` and `SIM_STOP()`.
We provide two types of [tools](https://github.com/comsys-lab/PIM-NDP-Sim/tree/main/tools) to facilitate kernel definition:

1. **PIM Memory Management Kernels (PIM Manager):** These ensure the NDP simulator identifies the PIM region within TinyEMU and reflects it during compilation.
2. **PIM Computation Kernels:** These consist of a basic GEMV kernel and an LLM layer kernel.
Both are based on AiMulator, which models the SK hynix AiM architecture.

We have defined customized RISC-V instructions to support PIM-specific load/store semantics.
As PIM architectures primarily target memory-bound GEMV operations, we provide a GEMV kernel implemented using these custom instructions and the `volatile` keyword.
Additionally, we provide a single hidden layer of the [Qwen3-32B dense model](https://github.com/huggingface/transformers/blob/main/src/transformers/models/qwen3), configured for LLM serving (online inference) with the following parameters:
- **Batch size:** 1
- **Sequence length:** 1024

## Future Work
- **PIM/NDP Co-simulation:** Define a mechanism to handle customized instructions as they pass through the NDP core pipeline stages.
- **Vector Extensions:** Integrate RISC-V vector extensions (RVV) for enhanced NDP simulation.

## Contributors
- Jongmin Kim ({jmkim99@korea.ac.kr})
- Dongwon Yang ({yang2919@korea.ac.kr})

## Acknowledgment
- For MARSS-RISCV, please refer to the [official repository](https://github.com/umd-memsys/DRAMSim3).

## License
- We follow the licenses of the base simulators.
- This project is licensed under the MIT License - refer to the [LICENSE.md](LICENSE.md) file for details.
- The SLIRP library has a two clause BSD license.
