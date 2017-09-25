// GDB RSP server GDBSIM CPU model wrapper: declaration

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

#ifndef GDBSIM_H
#define GDBSIM_H

#include "ITarget.h"


class GdbSimImpl;


//! The top level GDBSIM class.

//! We hide the details away in a separate implementation class, so that this
//! header has minimal other dependencies.

class GdbSim final : public ITarget
{
 public:

  // Constructor and destructor

  GdbSim (const TraceFlags * flags);
  ~GdbSim ();

  virtual ResumeRes  resume (ResumeType step);
  virtual ResumeRes  resume (ResumeType step,
                             std::chrono::duration <double>  timeout);

  virtual ResumeRes  terminate (void);
  virtual ResumeRes  reset (ITarget::ResetType  type);

  virtual uint64_t  getCycleCount (void) const;
  virtual uint64_t  getInstrCount (void) const;

  // Read contents of a target register.

  virtual std::size_t  readRegister (const int  reg,
				     uint_reg_t & value) const;

  // Write data to a target register.

  virtual std::size_t  writeRegister (const int  reg,
				      const uint_reg_t  value);

  // Read data from memory.

  virtual std::size_t  read (const uint32_t  addr,
			     uint8_t * buffer,
			     const std::size_t  size) const;

  // Write data to memory.

  virtual std::size_t  write (const uint32_t  addr,
			      const uint8_t * buffer,
			      const std::size_t  size);

  // Insert and remove a matchpoint (breakpoint or watchpoint) at the given
  // address.  Return value indicates whether the operation was successful.

  virtual bool  insertMatchpoint (const uint32_t  addr,
				  const MatchType  matchType);
  virtual bool  removeMatchpoint (const uint32_t  addr,
				  const MatchType  matchType);

  // Generic pass through of command

  virtual bool command (const std::string  cmd,
			std::ostream & stream);

  // Identify the server

  void gdbServer (GdbServer *server);

  // Verilator support

  virtual double timeStamp ();


 private:

  //! The implementation class for GdbSim

  GdbSimImpl * mGdbSimImpl;

};	// class GdbSim


#endif	// GDBSIM_H

// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
