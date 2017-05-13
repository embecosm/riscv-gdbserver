# riscv-gdbserver
GDB Server for interacting with RISC-V models, boards and FPGAs

## Status
The current implementation is a work-in-progress and expects a checkout
our modified picorv32 repo to be in the root of this repo. Since no
Makefiles have been generated yet, you will need to run autoreconf
as the first step of building.

As it stands, the riscv-gdbserver binary produced is untested and
needs more work.

## How to build
Ensure you have verilator installed before you begin, since that is
required to build our RISCV model.

To create a riscv-gdbserver binary after a fresh checkout of this repo:

```
autoreconf --install
git clone ssh://git@github.com/embecosm/picorv32
cd picorv32/scripts/gdbserver
make
cd ../../..
./configure --with-model-headers=../picorv32/scripts/gdbserver/obj_dir --with-verilator-headers=/usr/local/share/verilator/include CXXFLAGS=-std=gnu++11
make
```

## Notes on documentation
Since this is being developed from the ground-up, it does not yet have a doc directory.
It will be added, and populated with useful documentation, at the appropriate time.
