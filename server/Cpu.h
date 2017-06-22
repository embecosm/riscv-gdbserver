// GDB RSP server CPU model wrapper: declaration

// Copyright (C) 2017  Embecosm Limited <info@embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
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

#ifndef CPU__H
#define CPU__H

#include <cstdint>

#include "Vtestbench.h"
#include "verilated.h"

class Cpu final
{
 public:

  // Constructor and destructor

  Cpu ();
  ~Cpu ();

  // Accessors

  void clearTrapAndRestartInstruction (void);
  bool step (void);
  bool inReset (void) const;
  bool haveTrap (void) const;
  uint8_t readMem (uint32_t addr) const;
  void writeMem (uint32_t addr,
		 uint8_t  val);
  uint32_t readReg (unsigned int regno) const;
  void writeReg (unsigned int regno,
		 uint32_t     val);
  uint32_t readProgramAddr () const;
  void writeProgramAddr (uint32_t addr);
  uint32_t getLifetimeCounter();

 private:

  //! Top level Verilator model.

  Vtestbench * mCpu;

  //! Clock

  unsigned int  mClk = 0;

  //! For advancing the clock

  void clockStep (void);
};

#endif

// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
