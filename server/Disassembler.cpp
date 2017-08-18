// GDB RSP server RISC-V disassembler class: definition

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

#include <cstring>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "Disassembler.h"

// disass_fprintf is called multiple times for one insn by print_insn_riscv -
// once for each token printed. We hold on to disass_str for the duration of one
// print_insn_riscv call to accumulate all the tokens.

std::stringstream *disass_str;

extern "C" int disass_fprintf(char* stream, const char* format, ...)
{
  va_list args;

  va_start(args, format);
  int result = vsprintf(stream, format, args);
  va_end(args);
  (*disass_str) << stream;
  return result;
}

//! Constructor

//! Set up the diassembly info structure.

Disassembler::Disassembler ()
{
  init_disassemble_info (&mDisasmInfo, mDisasStr,
			 reinterpret_cast <fprintf_ftype> (disass_fprintf));
  disassemble_init_for_target (&mDisasmInfo);

  mDisasmInfo.buffer        = mVals;
  mDisasmInfo.buffer_vma    = 0;
  mDisasmInfo.buffer_length = sizeof (uint32_t);

}	// Disassembler::Disassembler ()


//! Destructor

Disassembler::~Disassembler ()
{
}	// Disassembler::Disassembler ()


//! Disassemble one instruction

//!@ param[in] insn  Instruction to disassemble

void
Disassembler::disassemble (uint32_t       insn,
			   std::ostream & stream)
{
  memcpy (mVals, &insn, sizeof (insn));
  disass_str = new std::stringstream();
  print_insn_riscv (0,
		    static_cast<disassemble_info *> (&mDisasmInfo));
  stream << disass_str->str();
  delete disass_str;

}	// Disassembler::disassemble ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
