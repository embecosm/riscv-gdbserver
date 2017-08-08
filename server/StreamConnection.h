// Remote Serial Protocol connection: declaration

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

#ifndef STREAM_CONNECTION__H
#define STREAM_CONNECTION__H

#include "AbstractConnection.h"
#include "TraceFlags.h"

//! Class implementing the RSP connection listener

//! This class is entirely passive. It is up to the caller to determine
//! that a packet will become available before calling ::getPkt ().

class StreamConnection : public AbstractConnection
{
public:

  // Constructors and destructor

  StreamConnection (TraceFlags *_traceFlags);
  ~StreamConnection ();

  // Public interface: manage client connections

  virtual bool  rspConnect ();
  virtual void  rspClose ();
  virtual bool  isConnected ();

private:

  // Implementation specific routines to handle individual chars.

  virtual bool  putRspCharRaw (char  c);
  virtual int   getRspCharRaw (bool blocking);

};	// StreamConnection ()

#endif	// STREAM_CONNECTION__H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
