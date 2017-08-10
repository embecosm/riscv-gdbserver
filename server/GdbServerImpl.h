// GDB RSP server implementation: definition

// Copyright (C) 2009, 2013, 2017  Embecosm Limited <info@embecosm.com>

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

#ifndef GDB_SERVER_IMPL_H
#define GDB_SERVER_IMPL_H

#include <chrono>
#include <cstdio>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string>

// General interface to targets

#include "ITarget.h"

// Class headers

#include "Disassembler.h"
#include "GdbServer.h"
#include "MpHash.h"
#include "RspConnection.h"
#include "RspPacket.h"
#include "TraceFlags.h"



//! Module implementing a GDB RSP server.

//! A loop listens for RSP requests, which are converted to requests to read
//! and write registers, read and write memory, or control the CPU

class GdbServerImpl
{
public:

  // Constructor and destructor

  GdbServerImpl (AbstractConnection * _conn,
		 ITarget * _cpu,
		 TraceFlags * _traceFlags,
		 GdbServer::KillBehaviour _killBehaviour);
  ~GdbServerImpl ();

  // Main loop to listen for and service RSP requests.

  void  rspServer ();

  // Callback for target to use

  bool command (const std::string  cmd,
		std::ostream & stream);


private:

  //! Definition of GDB target signals.

  enum class TargetSignal : int {
    NONE    =   0,
    INT     =   2,
    TRAP    =   5,
    XCPU    =  24,
    UNKNOWN = 143
  };

  // stream operator has to be a friend to access private members

  friend std::ostream & operator<< (std::ostream & s,
				    GdbServerImpl::TargetSignal  p);

  // For now these are hard-coded constants, but they need to be made
  // configurable.

  //! Total number of regs. 32 general regs + PC

  static const int RISCV_NUM_REGS = 33;

  //! Total bytes taken by regs. 4 bytes for each

  static const int RISCV_NUM_REG_BYTES = RISCV_NUM_REGS * 4;

  //! Minimum packet size for RSP. Must be large enough for any initial
  //! dialogue. Should at least allow all the registers ASCII encloded + end of
  //! string marker.

  static const int RSP_PKT_SIZE = (RISCV_NUM_REG_BYTES * 2 + 1) < 256
				    ? 256 : RISCV_NUM_REG_BYTES * 2 + 1;

  //! Constant for a thread id

  static const int  DUMMY_TID = 1;

  //! Constant for a breakpoint (EBREAK). Remember we are little-endian.

  static const uint32_t  BREAK_INSTR = 0x100073;

  //! Constant which is the sample period (in instruction steps) during
  //! "continue" etc.

  static const int RUN_SAMPLE_PERIOD = 10000;

  //! Our associated simulated CPU
  ITarget * cpu;

  //! Our trace flags
  TraceFlags *traceFlags;

  //! Our associated RSP interface
  AbstractConnection *rsp;

  //! The packet pointer. There is only ever one packet in use at one time, so
  //! there is no need to repeatedly allocate and delete it.
  RspPacket *pkt;

  //! We track the last type of packet for when we have to create an F request
  //! and later need to either continue or step after receiving the F reply.
  char lastPacketType;

  //! Hash table for matchpoints
  MpHash *mpHash;

  //! Timeout for continue.
  std::chrono::duration<double>  timeout;

  //! How to behave when we get a kill (k) packet.
  GdbServer::KillBehaviour killBehaviour;

  //! The diassembler

  Disassembler *mDisassembler;

  // Main RSP request handler
  void  rspClientRequest ();

  // Handle the various RSP requests
  int   stringLength (uint32_t addr);
  void  rspSyscallRequest ();
  void  rspSyscallReply ();
  void  rspReportException (TargetSignal  sig = TargetSignal::TRAP);
  void  rspReadAllRegs ();
  void  rspWriteAllRegs ();
  void  rspReadMem ();
  void  rspWriteMem ();
  void  rspReadReg ();
  void  rspWriteReg ();
  void  rspQuery ();
  void  rspCommand ();
  void  rspSetCommand (const char* cmd);
  void  rspShowCommand (const char* cmd);
  void  rspSet ();
  void  rspRestart ();
  void  rspVpkt ();
  void  rspWriteMemBin ();
  void  rspRemoveMatchpoint ();
  void  rspInsertMatchpoint ();

};	// GdbServerImpl ()

#endif	// GDB_SERVER_IMPL_H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
