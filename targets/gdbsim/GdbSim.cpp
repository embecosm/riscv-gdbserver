// GDB RSP server GDBSIM CPU model wrapper: definition

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

#include "GdbServer.h"
#include "GdbSim.h"
#include "GdbSimImpl.h"
#include "TraceFlags.h"


//! Constructor.

//! We only instantiate the implementation class.

//! @param[in] wantVcd  TRUE if we want a VCD generated, false otherwise.

GdbSim::GdbSim (const TraceFlags * flags) :
  ITarget (flags)
{
  mGdbSimImpl = new GdbSimImpl (flags);

}	// GdbSim::GdbSim ()


//! Destructor

//! Free the implementation class

GdbSim::~GdbSim ()
{
  delete mGdbSimImpl;

}	// GdbSim::~GdbSim ()


//! Resume execution

//! Wrapper for the implementation class

//! @param[in] step         Type of resumption required
//! @return The type of termination encountered.

ITarget::ResumeRes
GdbSim::resume (ResumeType  step)
{
  return mGdbSimImpl->resume (step);

}	// GdbSim::resume ()


//! Resume execution

//! Wrapper for the implementation class

//! @param[in] step         Type of resumption required
//! @param[in] timeout      Timeout requested
//! @return The type of termination encountered.

ITarget::ResumeRes
GdbSim::resume (ResumeType  step,
	       std::chrono::duration <double>  timeout)
{
  return mGdbSimImpl->resume (step, timeout);

}	// GdbSim::resume ()


//! Terminate execution

//! Wrapper for the implementation class.

//! @return  Result of termination

ITarget::ResumeRes
GdbSim::terminate (void)
{
  return  mGdbSimImpl->terminate ();

}	// GdbSim::terminate ()


//! Reset execution

//! Wrapper for the implementation class.

//! @return  Result of reset

ITarget::ResumeRes
GdbSim::reset (ITarget::ResetType  type)
{
  return mGdbSimImpl->reset (type);

}	// GdbSim::reset ()


//! Get count of cycles since startup or reset

//! Wrapper for the implementation class.

//! @return  Cycle count since startup or reset

uint64_t
GdbSim::getCycleCount (void) const
{
  return mGdbSimImpl->getCycleCount ();

}	// GdbSim::getCycleCount ()


//! Get count of instructions since startup or reset

//! Wrapper for the implementation class.

//! @return  Instruction count since startup or reset

uint64_t
GdbSim::getInstrCount (void) const
{
  return mGdbSimImpl->getInstrCount ();

}	// GdbSim::getInstrCount ()


//! Read a register

//! Wrapper for the implementation class.

//! @param[in] reg     Register to read
//! @param[out] value  Where to put the result of the read
//! @return  Number of bytes read into the register

std::size_t
GdbSim::readRegister (const int  reg,
		     uint_reg_t & value) const
{
  return mGdbSimImpl->readRegister (reg, value);

}	// GdbSim::readRegister ()


//! Write a register

//! Wrapper for the implementation class.

//! @param[in] reg    Register to write
//! @param[in] value  Value to write
//! @return  Number of bytes written into the register

std::size_t
GdbSim::writeRegister (const int  reg,
		      const uint_reg_t  value)
{
  return mGdbSimImpl->writeRegister (reg, value);

}	// GdbSim::writeRegister ()


//! Read from memory

//! Wrapper for the implementation class.

//! @param[in]  addr    Address to read
//! @param[out] buffer  Where to put the result of the read
//! @param[in]  size    Number of bytes to read
//! @return  Number of bytes read

std::size_t
GdbSim::read (const uint32_t addr,
	     uint8_t * buffer,
	     const std::size_t  size) const
{
  return mGdbSimImpl->read (addr, buffer, size);

}	// GdbSim::read ()


//! Write to memory

//! Wrapper for the implementation class.

//! @param[in]  addr    Address to write
//! @param[in]  buffer  Data to write
//! @param[in]  size    Number of bytes to write
//! @return  Number of bytes written

std::size_t
GdbSim::write (const uint32_t  addr,
	      const uint8_t * buffer,
	      const std::size_t size)
{
  return mGdbSimImpl->write (addr, buffer, size);

}	// GdbSim::write ()


//! Insert a matchpoint

//! Wrapper for the implementation class.

//! @param[in] addr  Address at which to set the matchpoint
//! @param[in] matchType  Type of matchpoint (breakpoint or watchpoint)
//! @return  TRUE if watchpoint set, FALSE otherwise.

bool
GdbSim::insertMatchpoint (const uint32_t  addr,
			 const MatchType  matchType)
{
  return mGdbSimImpl->insertMatchpoint (addr, matchType);

}	// GdbSim::insertMatchpoint ()


//! Remove a matchpoint

//! Wrapper for the implementation class.

//! @param[in] addr  Address from which to clear the matchpoint
//! @param[in] matchType  Type of matchpoint (breakpoint or watchpoint)
//! @return  TRUE if watchpoint set, FALSE otherwise.

bool
GdbSim::removeMatchpoint (const uint32_t  addr,
			 const MatchType matchType)
{
  return mGdbSimImpl->removeMatchpoint (addr, matchType);

}	// GdbSim::removeMatchpoint ()


//! Pass a command through to the target

//! Wrapper for the implementation class.

//! @param[in]  cmd  The command being passed
//! @param[out] stream  Stream on which to write any response.
//! @return  TRUE if the command succeeded, FALSE otherwise.

bool
GdbSim::command (const std::string  cmd,
		std::ostream & stream)
{
  return  mGdbSimImpl->command (cmd, stream);

}	// GdbSim::command ()


//! Wrapper for the implementation class

//! @param[in] server  The server to use

void
GdbSim::gdbServer (GdbServer * server)
{
  mGdbSimImpl->gdbServer (server);

}	// GdbSim::gdbServer ()


//! Return a timestamp.

//! This is needed to support the $time function in Verilog.  This in turn is
//! needed for VCD output.

//! Pass through to the implementation class.

//! @return  The current simulation time in seconds.

double
GdbSim::timeStamp ()
{
  return mGdbSimImpl->timeStamp ();

}	// GdbSim::timeStamp ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
