# linux/

PetaLinux 2023.1 project configuration (`mini_npu_linux`). It was built in an Ubuntu 22.04 VM from the Vivado `.xsa`.

Only the configuration is committed, never build outputs:

- `project-spec/configs/`: PetaLinux configuration
- `project-spec/meta-user/`: device tree additions (generic-uio nodes for the mini-NPU, AXI DMA and GPIO; reserved-memory region for DMA buffers; bootargs)

Not committed: `build/`, `images/`, `components/`, `.petalinux/`, `project-spec/hw-description/`.

The boot files for the SD card (`BOOT.BIN`, `image.ub`, `boot.scr`) are produced by `petalinux-build` and `petalinux-package`. A working set goes in `../release/`.
