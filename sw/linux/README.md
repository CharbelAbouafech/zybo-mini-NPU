# sw/linux/

The main application, running on PetaLinux 2023.1. It is built with the PetaLinux SDK cross-compiler.

Planned contents:

- Mini-NPU mode: splits each layer into 4x4 blocks, programs the mini-NPU and AXI DMA registers, runs the MNIST MLP
- ARM-only mode: the same MLP in plain C (-O3), as the baseline
- Benchmark: median of many runs (`clock_gettime`), plus the mini-NPU CYCLES register for pure compute time
- Self-check: compares every output with the golden model results in the generated headers

How it talks to the hardware:

- Registers: `/dev/uioN` (generic-uio), mapped with `mmap()`
- DMA buffers: reserved memory region exposed through UIO (physically contiguous, uncached)
- Interrupts: blocking `read()` on `/dev/uioN`

Copy to the board with `scp` over Ethernet, or put it on the SD card.
