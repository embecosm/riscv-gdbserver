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


#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

// Standard headers

#include <cstdint>

// Binutils headers, which need config header info

#include "config.h"
#include "bfd.h"
#include "dis-asm.h"


// Wrapper class for the binutils disassembler

class Disassembler
{

public:

  // Constructor and destructor

  Disassembler ();
  ~Disassembler ();

  // The operation to disassemble.

  void disassemble (uint32_t       insn,
		    std::ostream & stream);

private:

  //! Buffer for instruction to be disassembled.

  bfd_byte mVals[sizeof (uint32_t)];

  //! Structure providing all the info needed by the disassembler.

  disassemble_info mDisasmInfo;

  //! Large string for printing to using sprintf

  char mDisasStr [256];

};	// class Disassembler

#endif	// DISASSEMBLER_H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
