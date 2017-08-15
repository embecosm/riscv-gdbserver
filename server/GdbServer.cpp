// GDB RSP server: definition

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

#include <cstring>

#include "GdbServer.h"
#include "GdbServerImpl.h"


//! Constructor for the GDB RSP server.

//! A wrapper for the implementation class

//! @param[in] rspPort      RSP port to use.
//! @param[in] _cpu         The simulated CPU
//! @param[in] _traceFlags  Flags controlling tracing

GdbServer::GdbServer (AbstractConnection * _conn,
			      ITarget * _cpu,
			      TraceFlags * _traceFlags,
			      GdbServer::KillBehaviour _killBehaviour)
{
  mServerImpl = new GdbServerImpl (_conn, _cpu, _traceFlags, _killBehaviour);

}	// GdbServer::GdbServer ()


//! Destructor

GdbServer::~GdbServer ()
{
  delete  mServerImpl;

}	// GdbServer::~GdbServer


//! Main loop to listen for RSP requests

//! Wrap the implementation class

int
GdbServer::rspServer ()
{
  return mServerImpl->rspServer ();

}	// GdbServer::rspServer ()


//! Callback for targets to use.

//! @param[in] cmd  Request as textual command
//! @param[out] stream  Response as text on a stream
//! @return  TRUE if the command was accepted, FALSE otherwise

bool
GdbServer::command (const std::string  cmd,
		    std::ostream & stream)
{
  return  mServerImpl->command (cmd, stream);

}	// GdbServer::rspServer ()


//! Output operator for KillBehavior enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The KillBehavior value to output.
//! @return  The stream with the item appended.

std::ostream &
operator<< (std::ostream & s,
	    GdbServer::KillBehaviour  p)
{
  const char * name;

  switch (p)
    {
    case GdbServer::KillBehaviour::RESET_ON_KILL: name = "reset";   break;
    case GdbServer::KillBehaviour::EXIT_ON_KILL:  name = "exit";    break;
    default:                                      name = "unknown"; break;
    }

  return  s << name;

}	// operator<< ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
