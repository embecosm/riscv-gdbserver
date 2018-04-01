// GDB RSP server GDBSIM IMPL CPU model: declaration

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

#ifndef GDBSIM_IMPL_H
#define GDBSIM_IMPL_H

#include <cstdint>
#include <fstream>

#include "ITarget.h"
#include "gdb/remote-sim.h"
#include "gdb/callback.h"

//! The GDBSIM implementation class.

//! This class has all the operational implementation

class GdbSimImpl final
{
 public:

  GdbSimImpl (const TraceFlags * flags);
  ~GdbSimImpl ();

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

  //! The trace flags with which we were called.

  const TraceFlags * mFlags;

  //! OS-level callback functions for write, flush, etc.
  host_callback gdb_callback;

  //! Handle for simulator description.
  SIM_DESC gdbsim_desc;

  //! Our invoking server

  GdbServer * mServer;

  //! Have we reset before?

  bool mHaveReset;

  ITarget::ResumeRes doOneStep (std::chrono::duration <double>);
  ITarget::ResumeRes doRunToBreak (std::chrono::duration <double>);
};

#endif	// GDBSIM_IMPL_H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
