# riscv-gdbserver
GDB Server for interacting with RISC-V models, boards and FPGAs

## Status
The current implementation is a work-in-progress and expects a checkout
our modified picorv32 repo to be in the root of this repo. Since no
Makefiles have been generated yet, you will need to run autoreconf
during building.

As of 20th May, it is now possible to breakpoint and continue.

## How to build
Ensure you have verilator installed before you begin, since that is
required to build our RISCV model.

To create a riscv-gdbserver binary after a fresh checkout of this repo:

```
git clone ssh://git@github.com/embecosm/picorv32
cd picorv32/scripts/gdbserver
make
cd ../../..
autoreconf --install
./configure --with-verilator-headers=/usr/local/share/verilator/include \
           CXXFLAGS="-std=gnu++11 -g" \
           VTESTBENCH=`pwd`/picorv32/scripts/gdbserver/obj_dir
make
```

## Example GDB session interacting with riscv-gdbserver

Start the riscv-gdbserver, supplying a port as the only parameter:

```
server/riscv-gdbserver 51000
```

In a separate terminal, invoke gdb with the binary you wish to run:

```
riscv32-unknown-elf-gdb dhry.elf
```

At the gdb prompt, enter the following:

```
target remote :51000
load
display /i $pc
stepi
```

This connects to the gdbserver, loads the dhry.elf binary and executes the first instruction.

You can now continue stepping through with the 'stepi' command, or use 'continue' to run until the end of the program.

To see the RSP packets being sent and received, type the following at the gdb prompt:

```
set debug remote 1
```

Please note that continuing (or stepping) after a breakpoint is currently not working.

## Notes on documentation
Since this is being developed from the ground-up, it does not yet have a doc directory.
It will be added, and populated with useful documentation, at the appropriate time.
