# src/

Synthesizable RTL for the mini-NPU, plus the board constraints.

| File | Purpose |
|---|---|
| `mini_npu_top.v` | Top wrapper: AXI4-Lite slave (registers), AXI4-Stream in/out |
| `npu_regs.v` | Register file (CTRL, STATUS, DIM_MK, DIM_N, DIM_B, QUANT_SCALE, QUANT_SHIFT, BIAS0-3, IRQ_EN, CYCLES) |
| `npu_fsm.v` | Control FSM: load weights, stream inputs, post-process, done |
| `npu_pe.v` | One processing element: INT8 x INT8 multiply, INT32 accumulate |
| `npu_systolic_array.v` | 4x4 weight-stationary systolic array (16 PEs) |
| `npu_accum.v` | Accumulator BRAM (B x 4 INT32 partial sums across K-blocks) |
| `npu_ppu.v` | Post-Processing Unit: bias add, fixed-point scale + shift, ReLU, clamp to INT8 |
| `zybo_z7_10.xdc` | Pin constraints (LEDs, BTN0, SW0) |

The file list is the plan and will change as the design does.

- One module per file, file name = module name.
- `create.tcl` adds every `*.v` in this folder to the Vivado project.
- Testbenches go in `../sim/`, not here.
