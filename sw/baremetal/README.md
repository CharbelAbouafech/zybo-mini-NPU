# sw/baremetal/

Bare-metal smoke tests (Vitis 2023.1 classic IDE, standalone). They check new hardware quickly before it is used from Linux.

| App | Tests |
|---|---|
| `loopback/` | AXI DMA loopback: MM2S -> AXI4-Stream FIFO -> S2MM, data compare, MB/s |
| `one_block/` | One 4x4 block through the mini-NPU, compared with the golden model |

- Only each app's `src/` is committed. The platform and BSP are recreated from the `.xsa` (Create Platform Project -> Application Project).
- Cache handling: flush source buffers before MM2S, invalidate destination buffers before and after S2MM.
