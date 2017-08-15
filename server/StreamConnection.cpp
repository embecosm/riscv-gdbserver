// Remote Serial Protocol connection: implementation

// Copyright (C) 2017  Embecosm Limited <info@embecosm.com>

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
// ----------------------------------------------------------------------------

#include <iostream>
#include <iomanip>

#include <cerrno>
#include <csignal>
#include <cstring>

#include <sys/select.h>
#include <unistd.h>

#include "StreamConnection.h"
#include "Utils.h"

using std::cerr;
using std::cout;
using std::dec;
using std::endl;
using std::flush;
using std::hex;
using std::setfill;
using std::setw;


//! Constructor when using a port number

//! Sets up various parameters

//! @param[in] _portNum     the port number to connect to
//! @param[in] _traceFlags  flags controlling tracing
StreamConnection::StreamConnection (TraceFlags *_traceFlags) :
  AbstractConnection (_traceFlags)
{
  // Nothing.
}	// StreamConnection ()


//! Destructor

//! Close the connection if it is still open
StreamConnection::~StreamConnection ()
{
  this->rspClose ();		// Don't confuse with any other close ()
}	// ~StreamConnection ()


//! Get a new client connection.

//! Blocks until the client connection is available.

//! A lot of this code is copied from remote_open in gdbserver remote-utils.c.

//! This involves setting up a socket to listen on a socket for attempted
//! connections from a single GDB instance (we couldn't be talking to multiple
//! GDBs at once!).

//! The service is specified either as a port number in the Or1ksim
//! configuration (parameter rsp_port in section debug, default 51000) or as a
//! service name in the constant OR1KSIM_RSP_SERVICE.

//! If there is a catastrophic communication failure, service will be
//! terminated using sc_stop.

//! The protocol used for communication is specified in OR1KSIM_RSP_PROTOCOL.

//! @return  TRUE if the connection was established or can be retried. FALSE
//!          if the error was so serious the program must be aborted.
bool
StreamConnection::rspConnect ()
{
  // There's no way to connect, we rely on stdin / stdout being passe into
  // the process, we're connected from the start.  As we currently always
  // report that we're connected this should never be called.
  return false;
}	// rspConnect ()


//! Close a client connection if it is open. Nothing to do with stdin or
//! stdout.
void
StreamConnection::rspClose ()
{
}	// rspClose ()


//! Report if we are connected to a client.

//! @return  TRUE if we are connected, FALSE otherwise
bool
StreamConnection::isConnected ()
{
  // TODO: We're only closed if stdin or stdout are closed.  Is this
  // something we care about?  For now just say we're always connected.
  return true;

}	// isConnected ()

//! Put a single character out on the RSP connection

//! Utility routine. This should only be called if the client is open, but we
//! check for safety.

//! @param[in] c         The character to put out

//! @return  TRUE if char sent OK, FALSE if not (communications failure)

bool
StreamConnection::putRspCharRaw (char  c)
{
  // Write until successful (we retry after interrupts) or catastrophic
  // failure.
  while (true)
    {
      switch (write (STDOUT_FILENO, &c, sizeof (c)))
	{
	case -1:
	  // Error: only allow interrupts or would block
	  if ((EAGAIN != errno) && (EINTR != errno))
	    {
	      cerr << "Warning: Failed to write to RSP client: "
			<< "Closing client connection: "
			<<  strerror (errno) << endl;
	      return  false;
	    }

	  break;

	case 0:
	  break;		// Nothing written! Try again

	default:
	  return  true;		// Success, we can return
	}
    }
}	// putRspCharRaw ()


//! Get a single character from the RSP connection

//! Utility routine. This should only be called if the client is open, but we
//! check for safety.

//! @param[in] blocking  True if the read should block.
//! @return  The character received or -1 on failure, or if the read would
//!          block, and blocking is true.

int
StreamConnection::getRspCharRaw (bool blocking)
{
  // Blocking read until successful (we retry after interrupts) or
  // catastrophic failure.

  for (;;)
    {
      unsigned char  c;
      int res;
      struct timeval timeout;
      fd_set readfds;

      timeout.tv_sec = 0;
      timeout.tv_usec = 0;

      FD_ZERO (&readfds);
      FD_SET (STDIN_FILENO, &readfds);

      res = select (STDIN_FILENO + 1,
                    &readfds, NULL, NULL,
                    (blocking ? NULL : &timeout));

      switch (res)
  	{
  	case -1:
  	  // Error: only allow interrupts

  	  if (EINTR != errno)
  	    {
  	      cerr << "Warning: Failed to read from RSP client: "
  		   << "Closing client connection: "
  		   <<  strerror (errno) << endl;
  	      return  -1;
  	    }
  	  break;

  	case 0:
          // Timeout, only happens in the blocking case.
  	  return  -1;

  	default:
          if (read (STDIN_FILENO, &c, sizeof (c)) == -1)
            return -1;
  	  return  c & 0xff;	// Success, we can return (no sign extend!)
  	}
    }
}	// getRspCharRaw ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
