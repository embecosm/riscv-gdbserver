// GDB RSP server RI5CY CPU model: declaration

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

#ifndef RI5CY_IMPL_H
#define RI5CY_IMPL_H

#include <cstdint>
#include <iostream>
#include <fstream>

#include "ITarget.h"
#include "Vtop.h"


//! The RI5CY implementation class.

//! This class has all the operational implementation

class Ri5cyImpl final
{
 public:

  Ri5cyImpl (const TraceFlags * flags);
  ~Ri5cyImpl ();

  ITarget::ResumeRes  resume (ITarget::ResumeType step);
  ITarget::ResumeRes  resume (ITarget::ResumeType step,
			      std::chrono::duration <double>  timeout);

  ITarget::ResumeRes  terminate ();
  ITarget::ResumeRes  reset (ITarget::ResetType  type);

  uint64_t  getCycleCount () const;
  uint64_t  getInstrCount () const;

  // Read contents of a target register.

  std::size_t  readRegister (const int  reg,
			     uint_reg_t & value);

  // Write data to a target register.

  std::size_t  writeRegister (const int  reg,
			      const uint_reg_t  value);

  // Read data from memory.

  std::size_t  read (const uint32_t  addr,
		     uint8_t * buffer,
		     const std::size_t  size) const;

  // Write data to memory.

  std::size_t  write (const uint32_t  addr,
		      const uint8_t * buffer,
		      const std::size_t  size);

  // Insert and remove a matchpoint (breakpoint or watchpoint) at the given
  // address.  Return value indicates whether the operation was successful.

  bool  insertMatchpoint (const uint32_t  addr,
			  const ITarget::MatchType  matchType);
  bool  removeMatchpoint (const uint32_t  addr,
			  const ITarget::MatchType  matchType);

  // Generic pass through of command

  bool command (const std::string  cmd,
		std::ostream & stream);

  // Identify the server

  void gdbServer (GdbServer *server);

  // Verilog support functions

  double timeStamp ();


private:

  //! Clock period in ns (this value for 50 MHz clock)

  const unsigned int  CLK_PERIOD_NS = 20;

  //! How many cycles of reset

  const int RESET_CYCLES = 5;

  // Debug registers

  const uint16_t DBG_CTRL    = 0x0000;	//!< Debug control
  const uint16_t DBG_HIT     = 0x0004;	//!< Debug hit
  const uint16_t DBG_IE      = 0x0008;	//!< Debug interrupt enable
  const uint16_t DBG_CAUSE   = 0x000c;	//!< Debug cause (why entered debug)
  const uint16_t DBG_GPR0    = 0x0400;	//!< General purpose register 0
  const uint16_t DBG_GPR31   = 0x047c;	//!< General purpose register 41
  const uint16_t DBG_NPC     = 0x2000;	//!< Next PC
  const uint16_t DBG_PPC     = DBG_NPC + (sizeof (uint_reg_t) * 1);	//!< Prev PC

  const uint32_t DBG_CSR_MISA = 0x4301; //!< CSR MISA

  // Debug register flags

  const uint32_t DBG_CTRL_HALT = 0x00010000;	//!< Halt core
  const uint32_t DBG_CTRL_SSTE = 0x00000001;	//!< Single step core

  const uint32_t DBG_HIT_SLEEP = 0x00010000;	//!< Core is sleeping
  const uint32_t DBG_HIT_SSTH  = 0x00000001;	//!< Single step halted

  const uint32_t DBG_IE_ECALL  = 0x00000800;	//!< ECALL from M-Mode
  const uint32_t DBG_IE_SAF    = 0x00000080;	//!< Store access fault
  const uint32_t DBG_IE_SAM    = 0x00000040;	//!< Store address misaligned
  const uint32_t DBG_IE_LAF    = 0x00000020;	//!< Load access fault
  const uint32_t DBG_IE_LAM    = 0x00000010;	//!< Load address misaligned
  const uint32_t DBG_IE_BP     = 0x00000008;	//!< Breakpoint
  const uint32_t DBG_IE_ILL    = 0x00000004;	//!< Illegal instruction
  const uint32_t DBG_IE_IAF    = 0x00000002;	//!< Instruction access fault
  const uint32_t DBG_IE_IAM    = 0x00000001;	//!< Instr address misaligned

  // GDB register numbers

  const int REG_R0  = 0;		//!< GDB R0 regnum
  const int REG_R31 = 31;		//!< GDB R31 regnum
  const int REG_PC  = 32;		//!< GDB PC regnum

  // CSRs. CSRs start at register 65, which offsets the CSR numbers
  // specified in the privileged specification:
  // https://riscv.org/specifications/privileged-isa/

  const int CSR_MISA    = 0x342;

  //! Our invoking server

  GdbServer * mServer;

  //! The trace flags with which we were called.

  const TraceFlags * mFlags;

  //! Top level Verilator model.

  Vtop * mCpu;

  //! Is the core halted?

  bool  mCoreHalted;

  //! Cycle count

  uint64_t  mCycleCnt;

  //! Instruction count

  uint64_t  mInstrCnt;

  //! VCD trace file pointer

  VerilatedVcdC * mTfp;

  //! VCD time. This will be in ns and we have a 50MHz device

  vluint64_t  mCpuTime;

  //! Prev VCD time. This will be in ns and we have a 50MHz device

  vluint64_t  mPrevCpuTime;

  //! Prev code address

  uint32_t  mPrevAddr;

  //! Handle for address log

  std::ofstream addrLog;

  // Helper methods

  void clockModel ();
  void resetModel ();
  void haltModel ();
  void waitForHalt ();
  uint_reg_t readDebugReg (const uint16_t  dbg_reg);
  void writeDebugReg (const uint16_t  dbg_reg,
		      const uint_reg_t  dbg_val);
  ITarget::ResumeRes  stepInstr (std::chrono::duration <double>  timeout);
  ITarget::ResumeRes  runToBreak (std::chrono::duration <double>  timeout);

  bool stoppedAtSyscall ();
};

#endif	// RI5CY_IMPL_H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
