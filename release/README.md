# release/

Prebuilt files so the demo runs without rebuilding anything.

Planned contents:

- `system_wrapper.bit`: final bitstream
- `system_wrapper.xsa`: hardware export (for Vitis and PetaLinux)
- `sd_card/`: `BOOT.BIN`, `image.ub`, `boot.scr`, copied to the SD card's boot partition
- `mini_npu_project.zip`: archived Vivado project (File -> Project -> Archive)
- The compiled Linux application

These are generated files committed on purpose. If a `.gitignore` pattern matches one, add it with `git add -f`.
