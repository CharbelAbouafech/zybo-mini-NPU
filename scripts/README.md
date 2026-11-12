# scripts/

Tcl scripts that rebuild and simulate the Vivado project. The project itself is generated into `../proj/` and is not committed.

| Script | Purpose |
|---|---|
| `create.tcl` | Creates the `mini_npu` project for the Zybo Z7-10, adds `src/` and `sim/`, rebuilds the block design from `bd.tcl`, sets the wrapper as top |
| `bd.tcl` | Block design `system`, exported from Vivado with `write_bd_tcl`. Re-export after every block design change. |
| `simulate.tcl` | Runs one testbench or all of them and collects the PASS/FAIL results |

Usage, from the repo root:

```
vivado -mode batch -source scripts/create.tcl
vivado -mode batch -source scripts/simulate.tcl -tclargs all
```

`create.tcl` stops if `proj/` already exists; delete it first to rebuild from scratch.
