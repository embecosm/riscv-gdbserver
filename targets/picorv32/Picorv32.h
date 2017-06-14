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

#ifndef PICORV32__H
#define PICORV32__H

#include "ITarget.h"


class Picorv32Impl;


//! The top level PicoRV32 implementation class.

//! We hide the details away in a separate implementation class, so that this
//! header has minimal other dependencies.

class Picorv32 : public ITarget final
{
 public:

  // Constructor and destructor

  Picorv32 ();
  ~Picorv32 ();

  virtual ResumeRes  resume (ResumeType step,
			     SyscallInfo *syscall_info = nullptr) = 0;
  virtual ResumeRes  resume (ResumeType step,
                             std::chrono::duration <double>  timeout,
                             SyscallInfo *syscall_info = nullptr) = 0;

  virtual ResumeRes  terminate (void) = 0;
  virtual ResumeRes  reset (void) = 0;

  virtual uint64_t  getCycleCount (void) const = 0;
  virtual uint64_t  getInstrCount (void) const = 0;

  // Read contents of a target register.

  virtual std::size_t  readRegister (const int  reg,
				     uint32_t & value) const = 0;

  // Write data to a target register.

  virtual std::size_t  writeRegister (const int  reg,
				      const uint32_t  value) = 0;

  // Read data from memory.

  virtual std::size_t  read (const uint32_t  addr,
			     uint8_t * buffer,
			     const std::size_t  size) const = 0;

  // Write data to memory.

  virtual std::size_t  write (const uint32_t  addr,
			      const uint8_t * buffer,
			      const std::size_t  size) = 0;

  // Insert and remove a matchpoint (breakpoint or watchpoint) at the given
  // address.  Return value indicates whether the operation was successful.

  virtual bool  insertMatchpoint (const uint32_t & addr,
				  const MatchType  matchType) = 0;
  virtual bool  removeMatchpoint (const uint32_t & addr,
				  const MatchType  matchType) = 0;

  // Generic pass through of command

  virtual bool command (const std::string  cmd,
			std::ostream & stream) = 0;

private:

  //! The implementation class for PicoRV32

  Picorv32Impl * mPicorv32Impl;

};	// class Picorv232


#endif	// PICORV32__H

// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
