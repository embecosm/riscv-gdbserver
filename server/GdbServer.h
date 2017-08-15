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

#ifndef GDB_SERVER_H
#define GDB_SERVER_H


// Headers

#include <string>

// Classes needed for the declaration

class AbstractConnection;
class GdbServerImpl;
class ITarget;
class TraceFlags;


//! Module implementing a GDB RSP server.

//! This is the public interface, with the detailed implementation in
//! GdbServerImpl.

class GdbServer
{
public:

  //! How should we behave when GDB sends a kill (k) packet?

  enum KillBehaviour
    {
      //! Reset the target, but remain alive.
      RESET_ON_KILL,

      //! Stop the target, close the connection and return.
      EXIT_ON_KILL
    };

  // Constructor and destructor

  GdbServer (AbstractConnection * _conn,
	     ITarget * _cpu,
	     TraceFlags * _traceFlags,
	     KillBehaviour _killBehaviour);
  ~GdbServer ();

  // Main loop to listen for and service RSP requests.

  int rspServer ();

  // Callback for target to use

  bool command (const std::string  cmd,
		std::ostream & stream);


private:

  GdbServerImpl * mServerImpl;		// The actual implementation

};	// GdbServer ()

#endif	// GDB_SERVER_H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
