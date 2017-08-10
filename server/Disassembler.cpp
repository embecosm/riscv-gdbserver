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

#include "Disassembler.h"


//! Constructor

//! Set up the diassembly info structure.

Disassembler::Disassembler ()
{
  init_disassemble_info (&mDisasmInfo, mDisasStr,
			 reinterpret_cast <fprintf_ftype> (sprintf));
  disassemble_init_for_target (&mDisasmInfo);

  mDisasmInfo.buffer        = mVals;
  mDisasmInfo.buffer_vma    = reinterpret_cast<bfd_vma> (mVals);
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
  print_insn_riscv (reinterpret_cast<bfd_vma> (mVals),
		    static_cast<disassemble_info *> (&mDisasmInfo));
  stream << mDisasStr;

}	// Disassembler::disassemble ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
