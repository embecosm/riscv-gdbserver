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

void
Cpu::step ()
{
  mCpu->clk = mClk;
  mCpu->eval ();
  mClk++;

}	// Cpu::step ()


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


uint32_t
Cpu::readReg (unsigned int regno) const
{
  return mCpu->testbench->uut->readReg(regno);
}

void
Cpu::writeReg (unsigned int regno,
	       uint32_t     val)
{
  mCpu->testbench->uut->writeReg(regno, val);
}

uint32_t
Cpu::readProgramAddr () const
{
  return  mCpu->testbench->uut->readPc();
}

void
Cpu::writeProgramAddr (uint32_t     val)
{
  mCpu->testbench->uut->writePc(val);
}


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
