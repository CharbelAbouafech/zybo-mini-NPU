# Mini-NPU

A custom INT8 systolic-array accelerator on the Zynq-7010 (Digilent Zybo Z7-10), controlled from Linux, verified bit-exact against a software golden model, and benchmarked against the ARM Cortex-A9.

ECE 520/L System on Chip Design, Fall 2026

## Overview
_TODO_

## Architecture
_TODO: block diagram, PS/PL split, AXI interfaces, register map_

## Quantization
_TODO: summary, details in `docs/quantization.md`_

## Verification
_TODO: golden model, testbenches, on-board tests_

## Results
_TODO: correctness, accuracy, mini-NPU vs ARM-only timing, DMA vs compute share, resource usage, timing slack_

## How to run
_TODO: rebuild the Vivado project, run the simulations, build PetaLinux, run on the board_

## Repository layout
| Folder | Contents |
|---|---|
| `src/` | RTL and constraints |
| `sim/` | Testbenches and test vectors |
| `scripts/` | `create.tcl`, `simulate.tcl`, block design Tcl |
| `sw/baremetal/` | Vitis smoke tests |
| `sw/linux/` | Main Linux application and ARM-only baseline |
| `linux/` | PetaLinux configuration |
| `python/` | Training, golden model, export |
| `docs/` | Diagrams, specs, results |
| `release/` | Prebuilt bitstream, `.xsa`, SD card files |

## Demo
_TODO: video link_
