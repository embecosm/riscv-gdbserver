// GDB RSP server PicoRV32 CPU model: definition

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

#include "Picorv32Impl.h"
#include "Vtestbench_testbench.h"
#include "Vtestbench_picorv32__C1_EF1_EH1.h"


//! Constructor.

//! Initialize the clock, instantiate the Verilator model and set up VCD
//! tracing if requested.

//! @param[in] wantVcd  TRUE if we want a VCD generated, false otherwise.

Picorv32Impl::Picorv32Impl (bool  wantVcd) :
  mWantVcd (wantVcd),
  mCpuTime (0),
  mClk (0)
{
  mCpu = new Vtestbench;

  // Open VCD file if requested

  if (mWantVcd)
    {
      Verilated::traceEverOn (true);
      mTfp = new VerilatedVcdC;
      mCpu->trace (mTfp, 99);
      mTfp->open ("gdbserver.vcd");
    }
}	// Picorv32Impl::Picorv32Impl ()


//! Destructor.

// Delete the Verilator model and close the VCD if it was requested.

Picorv32Impl::~Picorv32Impl ()
{
  // Close VCD file if requested

  if (mWantVcd)
    mTfp->close ();

  delete mCpu;
}


// ! Step one single clock of the processor

void
Picorv32Impl::clockStep ()
{
  mCpu->clk = mClk;
  mCpu->eval ();
  mClk++;

  if (mWantVcd)
    {
      mCpuTime += 5;			// in ns
      mTfp->dump (mCpuTime);
    }
}	// Picorv32Impl::clockStep ()


// ! If trap is set, then get the processor in the right state to
// ! redo that instruction properly

void
Picorv32Impl::clearTrapAndRestartInstruction ()
{
  // do nothing if trap is not set
  if (haveTrap ())
  {
    // the trap happened on the instruction we want to continue from
    uint32_t prev_pc = mCpu->testbench->uut->readPc();
    // unfortunately, when we come out of trap, the instruction at the current
    // pc effectively becomes a NOP, so we actually need to set pc to the
    // previous instruction and then execute it. We then end up at the
    // desired pc that will execute properly.
    mCpu->testbench->uut->writePc (prev_pc-4);
    mCpu->testbench->uut->clearTrapAndContinue ();
    // loop until the processor has come out of the trap and changed pc to
    // the value we wrote above
    do
    {
      clockStep ();
    }
    while (prev_pc == readProgramAddr ());
    // pc now is at the prev instruction, which will effectively work as a NOP
    // and get us to the instruction we actually want to continue from, so
    // let's step it
    step ();
    // we are now ready to properly execute the instruction that trapped

  }

}	// Picorv32Impl::clearTrapAndRestartInstruction ()


//! Step one instruction execution

bool
Picorv32Impl::step ()
{
  uint32_t prev_pc = readProgramAddr ();
  do
  {
    clockStep ();
  }
  while (prev_pc == readProgramAddr () && haveTrap () == 0);
  return haveTrap () == 1;
}	// Picorv32Impl::step ()


//! Are we in reset?

bool
Picorv32Impl::inReset (void) const
{
  int res = mCpu->testbench->inReset ();
  return res == 1;

}	// inReset ()


//! Have we hit a trap?

bool
Picorv32Impl::haveTrap (void) const
{
  int  res = mCpu->testbench->haveTrap ();
  return res == 1;

}	// haveTrap ()


//! Read from memory

uint8_t
Picorv32Impl::readMem (uint32_t addr) const
{
  uint8_t res = mCpu->testbench->readMem (addr);
  return res;

}	// Picorv32Impl::readMem ()


//! Write to memory

void
Picorv32Impl::writeMem (uint32_t addr,
	       uint8_t  val)
{
  mCpu->testbench->writeMem (addr, val);

}	// Picorv32Impl::writeMem ()


//! Read a register

uint32_t
Picorv32Impl::readReg (unsigned int regno) const
{
  return mCpu->testbench->uut->readReg (regno);

}	// Picorv32Impl::readReg ()


//! Write a register

void
Picorv32Impl::writeReg (unsigned int regno,
	       uint32_t     val)
{
  mCpu->testbench->uut->writeReg (regno, val);

}	// Picorv32Impl::writeReg ()


//! Read the PC

uint32_t
Picorv32Impl::readProgramAddr () const
{
  return  mCpu->testbench->uut->readPc ();
}	// Picorv32Impl::readProgramAddr ()


//! Write the PC

void
Picorv32Impl::writeProgramAddr (uint32_t val)
{
  mCpu->testbench->uut->writePc (val);
  while (inReset ()) {
    // keep stepping the clock and writing PC while in reset, so that we are
    // at the desired start address once out of reset.
    clockStep ();
    mCpu->testbench->uut->writePc (val);
  }

}	// Picorv32Impl::writeProgramAddr ()


//! Provide a time stamp (needed for $time)

//! We count in nanoseconds.

//! @return  The time in seconds

double
Picorv32Impl::timeStamp ()
{
  return static_cast<double> (mCpuTime) / 1.0e-9;

}	// Picorv32Impl::timeStamp ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
