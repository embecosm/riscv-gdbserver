// GDB RSP server RI5CY CPU model: definition

// Copyright (C) 2017  Embecosm Limited <info@embecosm.com>

// Contributor Ian Bolton <ian.bolton@embecosm.com>

// This file is part of the RISC-V GDB server

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "config.h"
#include <cstring>
#include "bfd.h"
#include "dis-asm.h"
//#include "disassemble.h"
#include <stdint.h>


class Disassembler
{

public:
  Disassembler ();
  ~Disassembler () { }

  void disassemble_riscv (uint32_t insn);

private:
  bfd_byte vals[4];
  struct disassemble_info disasm_info;
};

#endif	// DISASSEMBLER_H
