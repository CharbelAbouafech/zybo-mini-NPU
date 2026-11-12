# python/

Training, quantization, golden model and export.

Planned contents:

- Train a float MLP (784-64-32-10, output padded to 12) on MNIST with PyTorch
- Quantize it to INT8 with explicit rules (see `../docs/quantization.md`)
- Golden model: integer-only numpy version of the exact hardware math. This is the definition of a correct answer.
- Export:
  - C headers for `../sw/` (weights in block order, INT32 biases, scale/shift per layer, test images, expected outputs)
  - `.mem` test vectors for `../sim/`

Setup: a virtual environment with CPU-only PyTorch. `venv/` and the MNIST download folder `data/` are not committed.
