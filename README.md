# riscv-gdbserver
GDB Server for interacting with RISC-V models, boards and FPGAs

## Status
This repository is part of the complete Embecosm RISCV tool chain.

## How to build
This tool should be built using the scripts in the RISCV tool chain repo.
Check that out using:
```
git clone https://github.com/embecosm/riscv-toolchain.git
```
or if you have write permission to the repo, you can use SSH:
```
git clone git@github.com:embecosm/riscv-toolchain.git
```

Then follow the instructions in the README for that repo. If you wish to just
build the gdbserver, then you can disentangle the individual commands from the
scripts in the `riscv-toolchain` repo.

## Example GDB session interacting with riscv-gdbserver

Start the riscv-gdbserver, specifying the core to use and the port on which to
connect:
```
riscv-gdbserver -c ri5cy 51000
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

This connects to the gdbserver, loads the `dhry.elf` binary and executes the
first instruction.

You can now continue stepping through with the `stepi` command.

To see the RSP packets being sent and received, type the following at the gdb prompt:
```
set debug remote 1
```

If you wish to set a breakpoint at main and continue until you hit it:
```
break main
continue
```

After the breakpoint has been hit, you can `continue` running until the end of
the program or just do further `stepi` commands first.

## Notes on documentation

Since this is being developed from the ground up, it does not yet have a doc
directory.  It will be added in the future
