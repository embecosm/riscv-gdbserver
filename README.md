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

git clone https://github.com/embecosm/picorv32.git
cd picorv32/scripts/gdbserver
make
cd ../../..
autoreconf --install
./configure --with-verilator-headers=/usr/local/share/verilator/include \
           CXXFLAGS="-std=gnu++11 -g" \
           VTESTBENCH=`pwd`/picorv32/scripts/gdbserver/obj_dir
make
```

(if you have write permission to this repo you could clone with
`git clone ssh://git@github.com/embecosm/picorv32`).

If you wish to build outside the source tree, that also works, but be sure
to amend VTESTBENCH in the configure command.

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

You can now continue stepping through with the 'stepi' command.

To see the RSP packets being sent and received, type the following at the gdb prompt:

```
set debug remote 1
```

If you wish to set a breakpoint at main and continue until you hit it:

```
break main
continue
```

After the breakpoint has been hit, you can 'continue' running until the end of the
program or just do further 'stepi' commands first.

## Notes on documentation
Since this is being developed from the ground-up, it does not yet have a doc directory.
It will be added, and populated with useful documentation, at the appropriate time.
