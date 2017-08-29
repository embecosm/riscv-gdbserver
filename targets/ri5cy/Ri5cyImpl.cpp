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
#include <iomanip>
#include <cstdint>
#include <cstdlib>
#include <sstream>

#include "GdbServer.h"
#include "Ri5cyImpl.h"
#include "TraceFlags.h"
#include "verilated_vcd_c.h"
#include "Vtop.h"
#include "Vtop__Syms.h"

using std::chrono::duration;
using std::chrono::system_clock;
using std::chrono::time_point;
using std::cerr;
using std::cout;
using std::endl;
using std::ostringstream;

//! Constructor.

//! Initialize the counters and instantiate the Verilator model. Take the
//! model through its reset sequence.

//! @param[in] flags  The trace flags

Ri5cyImpl::Ri5cyImpl (TraceFlags * flags) :
  mServer (nullptr),
  mFlags (flags),
  mCoreHalted (false),
  mCycleCnt (0),
  mInstrCnt (0),
  mCpuTime (0),
  mLastPc (0)
{
  mCpu = new Vtop;

  // Open VCD file if requested

  if (mFlags->traceVcd ())
    {
      Verilated::traceEverOn (true);
      mTfp = new VerilatedVcdC;
      mCpu->trace (mTfp, 99);
      mTfp->open ("gdbserver.vcd");
    }

  if (mFlags->traceDisas ())
    {
      mDisasFile.open("trace-disas.txt");
    }

  // Reset and halt the model

  resetModel ();

}	// Ri5cyImpl::Ri5cyImpl ()


//! Destructor.

//! Close VCD and delete the Verilator model.

Ri5cyImpl::~Ri5cyImpl ()
{
  // Close VCD file if requested

  if (mFlags->traceVcd ())
    mTfp->close ();

  if (mFlags->traceDisas ())
    mDisasFile.close ();

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
		   duration <double>  timeout,
		   SyscallInfo * syscallInfo)
{
  switch (step)
    {
    case ITarget::ResumeType::STEP:

      return stepInstr (timeout, syscallInfo);

    case ITarget::ResumeType::CONTINUE:

      return runToBreak (timeout, syscallInfo);

    case ITarget::ResumeType::STOP:

      // Request to halt the processor (typically due to a gross timeout)

      haltModel ();
      return ITarget::ResumeRes::SUCCESS;

    default:

      // Eek!

      cerr << "*** ABORT ***: Unknown step type when resuming: "
	   << static_cast<int> (step) << endl;
      exit (EXIT_FAILURE);
    }
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

      // Reset the time to make it consistent with the other counters
      mCpuTime = 0;
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
  else if (CSR_MISA == reg)
      dbg_addr = DBG_CSR_MISA;          // MISA
  else
    {
      cerr << "Warning: Attempt to read non-existent register "
           << reg << ": zero returned."
	   << endl;
      value = 0;
      return 4;
    }

  // Read via debug

  value = readDebugReg (dbg_addr);
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
  else if (CSR_MISA == reg)
      dbg_addr = DBG_CSR_MISA;        // MISA.
  else
  {
    cerr << "Warning: Attempt to write non-existent register "
         << reg << ": zero returned."
         << endl;
    return 4;
  }

  // Write via debug

  writeDebugReg (dbg_addr, value);
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
    buffer[i] = mCpu->top->ram_i->dp_ram_i->readByte (addr + i);

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
    mCpu->top->ram_i->dp_ram_i->writeByte (addr + i, buffer[i]);

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


//! Record the server we are associated with.

//! @param[in] server  Our invoking server.

void
Ri5cyImpl::gdbServer (GdbServer *server)
{
  mServer = server;

}	// Ri5cyImpl::gdbServer ()


//! Provide a time stamp (needed for $time)

//! We count in nanoseconds since (cold) reset.

//! @return  The time in nanoseconds (as specified in Verilator user manual)

double
Ri5cyImpl::timeStamp ()
{
  return mCpuTime;

}	// Ri5cyImpl::timeStamp ()


//! Helper method to clock the model

//! Clock the model through one full cycle, saving to VCD if requested.  It is
//! up to the caller to set any other signals.

void
Ri5cyImpl::clockModel ()
{
  mCpu->clk_i = 0;
  mCpu->eval ();

  mCpuTime += CLK_PERIOD_NS / 2;

  if (mFlags->traceVcd ())
    mTfp->dump (mCpuTime);

  mCpu->clk_i = 1;
  mCpu->eval ();

  mCpuTime += CLK_PERIOD_NS / 2;

  if (mFlags->traceVcd ())
    mTfp->dump (mCpuTime);

  mCycleCnt++;

  // Disassembly trace.
  //
  // We can only do this once we have the server available to do
  // disassembly. This means it can't be done during the reset
  // sequence, since that is part of the constructor.
  //
  // Instruction decode must be valid at the point we read the instruction.

  Vtop_riscv_id_stage* id_stage = mCpu->top->riscv_core_i->id_stage_i;

  if ((mFlags->traceDisas ())
   && (nullptr != mServer)
   && (id_stage->id_valid_o))
    {
      uint64_t currentPc = id_stage->pc_id_i;

      if (mLastPc != currentPc)
        {
          stringstream iss;
          ostringstream oss;
          iss << "disas 0x" << std::hex << id_stage->instr;
          mServer->command (iss.str(), oss);

          mDisasFile << std::setfill(' ') << std::setw(9) << mCpuTime << "  ";
          mDisasFile << std::hex << std::setfill('0') << std::setw(8) << currentPc;
          mDisasFile << " " << std::setw(8) << id_stage->instr << "  " << oss.str ();
          mDisasFile << std::dec << endl;

	  if (mFlags->traceDflush ())
	    mDisasFile.flush ();

          mLastPc = currentPc;
        }
    }
}	// Ri5cyImpl::clockModel ()


//! Helper method to reset the model

//! Take the verilator model through its reset sequence.

void
Ri5cyImpl::resetModel ()
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
    clockModel ();

  // Now out of reset

  mCpu->rstn_i = 1;

  // Halt the model, then set to halt on exceptions of interest

  haltModel ();
  writeDebugReg (DBG_IE, DBG_IE_BP | DBG_IE_ILL);

}	// Ri5cyImpl::resetModel ()


//! Helper method to halt the model

//! For this we need to use the debug interface

void
Ri5cyImpl::haltModel ()
{
  // Write HALT into the debug control register, then wait until we see the
  // processor has halted.

  writeDebugReg (DBG_CTRL, readDebugReg (DBG_CTRL) | DBG_CTRL_HALT);
  waitForHalt ();

}	// Ri5cyImpl::haltModel ()


//! Helper method to wait until the model is halted.

void
Ri5cyImpl::waitForHalt ()
{
  while (DBG_CTRL_HALT != (readDebugReg (DBG_CTRL) & DBG_CTRL_HALT))
    clockModel ();

  mCoreHalted = true;

}	// Ri5cyImpl::waitForHalt ()


//! Helper function to read a debug register.

//! This only sets the debug signals. It is up to the caller to set any other
//! signals (e.g. reset).

//! @param[in] dbg_reg  The debug register to read.
//! @return  The value read.

uint32_t
Ri5cyImpl::readDebugReg (const uint16_t  dbg_reg)
{
  // Set up the register to read

  mCpu->debug_req_i   = 1;
  mCpu->debug_addr_i  = dbg_reg;
  mCpu->debug_we_i    = 0;

  // Wait for the grant signal to indicate the read has been accepted.

  do
    clockModel ();
  while (mCpu->debug_gnt_o == 0);

  // Read data is available when rvalid is asserted.  This might be
  // immediately available, so test at the top of the loop.

  mCpu->debug_req_i = 0;		// Stop requesting

  while (mCpu->debug_rvalid_o == 0)
    clockModel ();

  return mCpu->debug_rdata_o;

}	// Ri5cyImpl::readDebugReg ()


//! Helper function to write a debug register.

//! This only sets the debug signals. It is up to the caller to set any other
//! signals (e.g. reset).

//! @param[in] dbg_reg  The debug register to write.
//! @param[in] dbg_val  The value to write

void
Ri5cyImpl::writeDebugReg (const uint16_t  dbg_reg,
			  const uint32_t  dbg_val)
{
  mCpu->debug_req_i   = 1;
  mCpu->debug_addr_i  = dbg_reg;
  mCpu->debug_we_i    = 1;
  mCpu->debug_wdata_i = dbg_val;

  // Write has succeeded when we get the grant signal asserted.

  do
    clockModel ();
  while (mCpu->debug_gnt_o == 0);

  mCpu->debug_req_i = 0;		// Stop requesting

}	// Ri5cyImpl::writeDebugReg ()


//! Do a single step.

//! This is achieved using the debug unit's single step enable.


ITarget::ResumeRes
Ri5cyImpl::stepInstr (duration <double>  timeout,
		      SyscallInfo * syscallInfo)
{
  if (nullptr != syscallInfo)
    cerr << "Warning: syscalls not supported when stepping" << endl;

  bool haveTimeout = duration <double>::zero() != timeout;
  time_point <system_clock, duration <double> > timeout_end;

  if (haveTimeout)
    timeout_end = system_clock::now () + timeout;

  // @todo Fetch enable turns off fetching of new instructions.

  mCpu->fetch_enable_i = 1;

  writeDebugReg (DBG_CTRL, DBG_CTRL_SSTE);		// SSTE
  writeDebugReg (DBG_HIT, 0);				// Release
  waitForHalt ();

  // @todo For now only timeout if it took too long.

  if (haveTimeout && (system_clock::now () > timeout_end))
    return  ITarget::ResumeRes::TIMEOUT;
  else
    return  ITarget::ResumeRes::INTERRUPTED;

}	// Ri5cyImpl::stepInstr ()


ITarget::ResumeRes
Ri5cyImpl::runToBreak (duration <double>  timeout,
		       SyscallInfo * syscallInfo)
{
  if (nullptr != syscallInfo)
    cerr << "Warning: syscalls not supported when continuing" << endl;

  bool haveTimeout = duration <double>::zero() != timeout;
  time_point <system_clock, duration <double> > timeout_end;

  if (haveTimeout)
    timeout_end = system_clock::now () + timeout;

  mCpu->fetch_enable_i = 1;

  uint32_t newDbgCtrl;

  newDbgCtrl = readDebugReg (DBG_CTRL) & ~(DBG_CTRL_SSTE | DBG_CTRL_HALT);
  writeDebugReg (DBG_CTRL, newDbgCtrl);

  // @todo this is a type of waitForHalt

  while (DBG_CTRL_HALT != (readDebugReg (DBG_CTRL) & DBG_CTRL_HALT))
    if (haveTimeout && (system_clock::now () > timeout_end))
      {
	haltModel ();
	return ITarget::ResumeRes::TIMEOUT;
      }
    else
      clockModel ();

  // Find out where we stopped, so we can look for our Syscall pattern planted
  // within newlib/libgloss.
  uint32_t stoppedAddress = readDebugReg (DBG_PPC);

  // The pattern we've used in newlib/libgloss for each supported syscall
  // is an ebreak with a nop before and after it. (It would ordinarily have
  // been an ecall, which is less straightforward to handle when doing
  // file I/O within GDB on bare-metal.)
  if (mCpu->top->ram_i->dp_ram_i->readByte (stoppedAddress-4) == 0x13 &&
      mCpu->top->ram_i->dp_ram_i->readByte (stoppedAddress-3) == 0 &&
      mCpu->top->ram_i->dp_ram_i->readByte (stoppedAddress-2) == 0 &&
      mCpu->top->ram_i->dp_ram_i->readByte (stoppedAddress-1) == 0 &&
      mCpu->top->ram_i->dp_ram_i->readByte (stoppedAddress+4) == 0x13 &&
      mCpu->top->ram_i->dp_ram_i->readByte (stoppedAddress+5) == 0 &&
      mCpu->top->ram_i->dp_ram_i->readByte (stoppedAddress+6) == 0 &&
      mCpu->top->ram_i->dp_ram_i->readByte (stoppedAddress+7) == 0)
    return ITarget::ResumeRes::SYSCALL;
  else
    return ITarget::ResumeRes::INTERRUPTED;

}	// Ri5cyImpl::runToBreak ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
