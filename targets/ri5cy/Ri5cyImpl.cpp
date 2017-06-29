// GDB RSP server RI5CY CPU model: definition

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

#include <iostream>
#include <cstdint>
#include <cstdlib>

#include "Ri5cyImpl.h"
#include "verilated_vcd_c.h"
#include "Vtop.h"
#include "Vtop_top.h"
#include "Vtop_ram.h"
#include "Vtop_sp_ram__A16.h"

using std::chrono::duration;
using std::chrono::system_clock;
using std::chrono::time_point;


//! Constructor.

//! Initialize the counters and instantiate the Verilator model. Take the
//! model through its reset sequence.

Ri5cyImpl::Ri5cyImpl (bool  wantVcd) :
  mCoreHalted (false),
  mCycleCnt (0),
  mInstrCnt (0),
  mWantVcd (wantVcd),
  mCpuTime (0)
{
  mCpu = new Vtop;

  // Open VCD file if requested

  if (mWantVcd)
    {
      Verilated::traceEverOn (true);
      mTfp = new VerilatedVcdC;
      mCpu->trace (mTfp, 99);
      mTfp->open ("gdbserver.vcd");
    }

  // Reset and halt the model

  resetModel ();

}	// Ri5cyImpl::Ri5cyImpl ()


//! Destructor.

//! Close VCD and delete the Verilator model.

Ri5cyImpl::~Ri5cyImpl ()
{
  // Close VCD file if requested

  if (mWantVcd)
    mTfp->close ();

  delete mCpu;

}	// Ri5cyImpl::~Ri5cyImpl ()


//! Resume execution with no timeout

//! @todo

//! @param[in]  step         The type of resume to carry out. Initially just
//!                          STEP and CONTINUE should suffice.
//! @param[out] syscallInfo  Used to return details of a syscall to be
//!                          handled. Not currently supported.
//! @return Why the target stopped.

ITarget::ResumeRes
Ri5cyImpl::resume (ITarget::ResumeType step,
		   SyscallInfo * syscallInfo)
{
  return resume (step, duration <double>::zero(), syscallInfo);

}	// Ri5cyImpl::resume ()


//! Resume execution with timeout

//! @todo

//! @param[in]  step         The type of resume to carry out. Initially just
//!                          STEP and CONTINUE should suffice.
//! @param[in]  timeout      Maximum time for execution to continue.
//! @param[out] syscallInfo  Used to return details of a syscall to be
//!                          handled. Not currently supported.
//! @return Why the target stopped.

ITarget::ResumeRes
Ri5cyImpl::resume (ITarget::ResumeType step,
		   std::chrono::duration <double>  timeout,
		   SyscallInfo * syscallInfo)
{
  // time_point <system_clock, duration <double> > timeout_end =
  //   system_clock::now () + timeout;

  if (ITarget::ResumeType::STEP == step)
    return stepInstr (syscallInfo);
  else
    return runToBreak (timeout, syscallInfo);

}	// Ri5cyImpl::resume ()


//! Terminate.

//! This has no meaning for an embedded system, so it does nothing.

ITarget::ResumeRes
Ri5cyImpl::terminate ()
{
  return  ITarget::ResumeRes::NONE;

}	// Ri5cyImpl::terminate ()


//! Reset

//! The only different between WARM and COLD is that we reset the counters. In
//! both cases we put the processor through its reset sequence.

//! @param[in]  type  Type of reset (can be warm or cold)
//! @return  Whether the reset was successful, which is always SUCCESS

ITarget::ResumeRes
Ri5cyImpl::reset (ITarget::ResetType  type)
{
  if (type == ITarget::ResetType::COLD)
    {
      mCycleCnt = 0;
      mInstrCnt = 0;
    }

  resetModel ();

  return ITarget::ResumeRes::SUCCESS;

}	// reset ()


//! Accessor for the cycle count

//! The value is set during the execution of the model.

//! @return  The number of cycles executed since startup or the last cold
//!          reset.

uint64_t
Ri5cyImpl::getCycleCount () const
{
  return mCycleCnt;

}	// Ri5cyImpl::getCycleCount ()


//! Accessor for the instruction count

//! The value is set during the execution of the model.

//! @return  The number of instructions executed since startup or the last cold
//!          reset.

uint64_t
Ri5cyImpl::getInstrCount () const
{
  return mInstrCnt;

}	// Ri5cyImpl::getInstrCount ()


//! Read a register

//! We assume that the core is halted. If not we have a problem.  We use the
//! debug unit and need a different mechanism for the program counter (which
//! is the NEXT program counter).

//! @param[in]  reg    The register to read
//! @param[out] value  The value read
//! @return  The size of the register read in bytes (always 4)

std::size_t
Ri5cyImpl::readRegister (const int  reg,
			 uint32_t & value)
{
  if (!mCoreHalted)
    {
      cerr << "*** ABORT ***: Attempt to read register from running core"
	   << endl;
      exit (EXIT_FAILURE);
    }

  uint16_t dbg_addr;

  if ((REG_R0 <= reg) && (reg <= REG_R31))
      dbg_addr = DBG_GPR0 + reg * 4;	// General register
  else if (REG_PC == reg)
      dbg_addr = DBG_NPC;		// Next PC
  else
    {
      cerr << "*** ABORT ***: Attempt to read non-existent register" << endl;
      exit (EXIT_FAILURE);
    }

  // Set up to read via debug

  mCpu->rstn_i        = 1;

  mCpu->debug_req_i   = 1;
  mCpu->debug_addr_i  = dbg_addr;
  mCpu->debug_we_i    = 0;

  // Wait for the grant signal to indicate the read has been accepted.

  do
    {
      mCpu->clk_i = 0;
      mCpu->eval ();

      if (mWantVcd)
	{
	  mCpuTime += 10;			// in ns
	  mTfp->dump (mCpuTime);
	}

      mCpu->clk_i = 1;
      mCpu->eval ();

      if (mWantVcd)
	{
	  mCpuTime += 10;			// in ns
	  mTfp->dump (mCpuTime);
	}

      mCycleCnt++;
    }
  while (mCpu->debug_gnt_o == 0);

  // Read data is available when rvalid is asserted.  This might be
  // immediately available, so test at the top of the loop.

  mCpu->debug_req_i = 0;		// Stop requesting

  while (mCpu->debug_rvalid_o == 0)
    {
      mCpu->clk_i = 0;
      mCpu->eval ();

      if (mWantVcd)
	{
	  mCpuTime += 10;			// in ns
	  mTfp->dump (mCpuTime);
	}

      mCpu->clk_i = 1;
      mCpu->eval ();

      if (mWantVcd)
	{
	  mCpuTime += 10;			// in ns
	  mTfp->dump (mCpuTime);
	}

      mCycleCnt++;
    }

  value = mCpu->debug_rdata_o;
  return 4;

}	// Ri5cyImpl::readRegister ()


//! Write a register

//! We assume that the core is halted. If not we have a problem.  We use the
//! debug unit and need a different mechanism for the program counter (which
//! is the NEXT program counter).

//! @param[in]  reg    The register to write
//! @param[out] value  The value to write
//! @return  The size of the register read in bytes (always 4)

std::size_t
Ri5cyImpl::writeRegister (const int  reg,
			  const uint32_t  value)
{
  if (!mCoreHalted)
    {
      cerr << "*** ABORT ***: Attempt to write register to running core"
	   << endl;
      exit (EXIT_FAILURE);
    }

  uint16_t dbg_addr;

  if ((REG_R0 <= reg) && (reg <= REG_R31))
    dbg_addr = DBG_GPR0 + reg * 4;    // General register
  else if (REG_PC == reg)
    dbg_addr = DBG_NPC;               // Next PC
  else
  {
    cerr << "*** ABORT ***: Attempt to write non-existent register" << endl;
    exit (EXIT_FAILURE);
  }

  // Set up to write via debug

  mCpu->rstn_i        = 1;

  mCpu->debug_req_i   = 1;
  mCpu->debug_addr_i  = dbg_addr;
  mCpu->debug_wdata_i = value;
  mCpu->debug_we_i    = 1;

  // Wait for the grant signal to indicate the read has been accepted.

  do
    {
      mCpu->clk_i = 0;
      mCpu->eval ();
      mCpu->clk_i = 1;
      mCpu->eval ();
      mCycleCnt++;
    }
  while (mCpu->debug_gnt_o == 0);

  mCpu->debug_req_i = 0;		// Stop requesting

  return 4;

}	// Ri5cyImpl::writeRegister ()


//! Read data from memory

//! You can't write memory via the debug registers. So we need a Verilator
//! task in top.sv to manually read/write memory and then we can use
//! that. With the processor halted it *should* be safe.

//! Otherwise we may have to put the memory external to the core instead of in
//! top.sv, but that would be painful to implement.

//! @param[in]  addr    Address to read from
//! @param[out] buffer  Buffer into which read data is placed
//! @param[in]  size    Number of bytes to read
//! @return  Number of bytes read

std::size_t
Ri5cyImpl::read (const uint32_t  addr,
		 uint8_t * buffer,
		 const std::size_t  size) const
{
  size_t i;

  for (i = 0; i < size; i++)
    buffer[i] = mCpu->top->ram_i->sp_ram_i->readByte (addr + i);

  return i;

}	// Ri5cyImpl::read ()


//! Write data to memory.

//! @todo

//! For discussion, @see read ()

//! @param[in] addr    Address to write to
//! @param[in] buffer  Buffer of data to write
//! @param[in] size    Number of bytes to write
//! @return  Number of bytes written

std::size_t
Ri5cyImpl::write (const uint32_t  addr,
		  const uint8_t * buffer,
		  const std::size_t  size)
{
  size_t  i;

  for (i = 0; i < size; i++)
    mCpu->top->ram_i->sp_ram_i->writeByte (addr + i, buffer[i]);

  return i;

}	// Ri5cyImpl::write ()


//! Insert a matchpoint (breakpoint or watchpoint)

//! @todo

//! It is acceptable to always fail. GDB will then use memory breakpoints,
//! implemented by writing EBREAK to the location.

//! @param[in] addr       Address for the matchpoint
//! @param[in] matchType  Type of breakpoint or watchpoint
//! @return  TRUE if the operation was successful, false otherwise.

bool
Ri5cyImpl::insertMatchpoint (const uint32_t  addr __attribute__ ((unused)),
			     const ITarget::MatchType  matchType __attribute__ ((unused)))
{
  return  false;

}	// Ri5cyImpl::insertMatchpoint ()


//! Insert a matchpoint (breakpoint or watchpoint)

//! @todo

//! It is acceptable to always fail. GDB will then use memory breakpoints,
//! implemented by writing EBREAK to the location.

//! @param[in] addr       Address for the matchpoint
//! @param[in] matchType  Type of breakpoint or watchpoint
//! @return  TRUE if the operation was successful, false otherwise.

bool
Ri5cyImpl::removeMatchpoint (const uint32_t  addr __attribute__ ((unused)),
			     const ITarget::MatchType  matchType __attribute__ ((unused)) )
{
  return  false;

}	// Ri5cyImpl::removeMatchpoint ()


//! Generic pass through of command

//! @todo

//! For now we have no commands, so we should never receive this, and thus
//! always fail.

//!@param[in]  cmd     The command to process
//!@param[out] stream  A stream to write any output from the command
//!@return  TRUE if the command was handled successfully, FALSE otherwise. We
//!         awlways return FALSE.

bool
Ri5cyImpl::command (const std::string  cmd __attribute__ ((unused)),
		    std::ostream & stream __attribute__ ((unused)))
{
  return false;

}	// Ri5cyImpl::command ()


//! Provide a time stamp (needed for $time)

//! We count in nanoseconds.

//! @return  The time in seconds

double
Ri5cyImpl::timeStamp ()
{
  return static_cast<double> (mCpuTime) / 1.0e-9;

}	// Ri5cyImpl::timeStamp ()


//! Helper method to reset the model

//! Take the verilator model through its reset sequence.

void
Ri5cyImpl::resetModel (void)
{
  // Assert reset

  mCpu->rstn_i = 0;

  // Put debug into inactive state

  mCpu->debug_req_i   = 0;
  mCpu->debug_addr_i  = 0;
  mCpu->debug_we_i    = 0;
  mCpu->debug_wdata_i = 0;

  // @todo Fetch enable turns off fetching of new instructions.

  mCpu->fetch_enable_i = 0;

  for (int i = 0; i < RESET_CYCLES; i++)
    {
      mCpu->clk_i = 0;
      mCpu->eval ();

      if (mWantVcd)
	{
	  mCpuTime += 10;			// in ns
	  mTfp->dump (mCpuTime);
	}

      mCpu->clk_i = 1;
      mCpu->eval ();

      if (mWantVcd)
	{
	  mCpuTime += 10;			// in ns
	  mTfp->dump (mCpuTime);
	}

      mCycleCnt++;
    }

  haltModel ();

}	// Ri5cyImpl::resetModel ()


//! Helper method to halt the model

//! For this we need to use the debug interface

void
Ri5cyImpl::haltModel (void)
{
  mCpu->rstn_i        = 1;

  // Write HALT into the debug register

  mCpu->debug_req_i   = 1;
  mCpu->debug_addr_i  = DBG_CTRL;
  mCpu->debug_we_i    = 1;
  mCpu->debug_wdata_i = DBG_CTRL_HALT;

  // Write has succeeded when we get the grant signal asserted.

  do
    {
      mCpu->clk_i = 0;
      mCpu->eval ();

      if (mWantVcd)
	{
	  mCpuTime += 10;			// in ns
	  mTfp->dump (mCpuTime);
	}

      mCpu->clk_i = 1;
      mCpu->eval ();

      if (mWantVcd)
	{
	  mCpuTime += 10;			// in ns
	  mTfp->dump (mCpuTime);
	}

      mCycleCnt++;
    }
  while (mCpu->debug_gnt_o == 0);

  mCoreHalted = true;

}	// Ri5cyImpl::haltModel ()


//
ITarget::ResumeRes
Ri5cyImpl::stepInstr (SyscallInfo * syscallInfo __attribute__ ((unused)) )
{
  return ITarget::ResumeRes::NONE;

}	// Ri5cyImpl::stepInstr ()


ITarget::ResumeRes
Ri5cyImpl::runToBreak (std::chrono::duration <double>  timeout __attribute__ ((unused)),
		       SyscallInfo * syscallInfo __attribute__ ((unused)) )
{
  return ITarget::ResumeRes::NONE;

}	// Ri5cyImpl::runToBreak ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
