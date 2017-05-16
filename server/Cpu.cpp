// GDB RSP server CPU model wrapper: definition

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

#include <cstdint>

#include "Cpu.h"
#include "Vtestbench_testbench.h"
#include "Vtestbench_picorv32__C1_EF1_EH1.h"

//! Constructor. Instantiate the Verilator model and initialize the clock.

Cpu::Cpu ()
{
  mCpu = new Vtestbench;
  mClk = 0;
}


//! Destructor. Delete the Verilator model.

Cpu::~Cpu ()
{
  delete mCpu;
}


//! Step one clock edge

bool
Cpu::step ()
{
  mCpu->clk = mClk;
  mCpu->eval ();
  mClk++;
  return haveTrap() == 1;
}	// Cpu::step ()


//! Are we in reset?

bool
Cpu::inReset (void) const
{
  int  res = mCpu->testbench->inReset ();
  return  res == 1;

}	// inReset ()


//! Have we hit a trap?

bool
Cpu::haveTrap (void) const
{
  int  res = mCpu->testbench->haveTrap ();
  return  res == 1;

}	// haveTrap ()


//! Read from memory

uint8_t
Cpu::readMem (uint32_t addr) const
{
  uint8_t res = mCpu->testbench->readMem (addr);
  return res;

}	// Cpu::readMem ()


//! Write to memory

void
Cpu::writeMem (uint32_t addr,
	       uint8_t  val)
{
  mCpu->testbench->writeMem(addr, val);

}	// Cpu::writeMem ()


//! Read a register

uint32_t
Cpu::readReg (unsigned int regno) const
{
  return mCpu->testbench->uut->readReg(regno);
}


//! Write a register

void
Cpu::writeReg (unsigned int regno,
	       uint32_t     val)
{
  mCpu->testbench->uut->writeReg(regno, val);
}


//! Read the PC

uint32_t
Cpu::readProgramAddr () const
{
  return  mCpu->testbench->uut->readPc();
}


//! Write the PC

void
Cpu::writeProgramAddr (uint32_t     val)
{
  mCpu->testbench->uut->writePc(val);
  while (inReset()) {
    // keep stepping and writing PC while in reset, so that we are
    // at the desired start address once out of reset.
    step();
    mCpu->testbench->uut->writePc(val);
  }
}


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
