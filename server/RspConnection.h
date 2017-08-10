// Remote Serial Protocol connection: declaration

// Copyright (C) 2009, 2013  Embecosm Limited <info@embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

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

#ifndef RSP_CONNECTION_H
#define RSP_CONNECTION_H

#include "AbstractConnection.h"
#include "RspPacket.h"
#include "TraceFlags.h"

//! Class implementing the RSP connection listener

//! This class is entirely passive. It is up to the caller to determine that a
//! packet will become available before calling ::getPkt ().

class RspConnection : public AbstractConnection
{
public:

  // Constructors and destructor

  RspConnection (int         _portNum,
		 TraceFlags *_traceFlags);
  ~RspConnection ();

  // Public interface: manage client connections

  bool  rspConnect ();
  void  rspClose ();
  bool  isConnected ();

private:

  //! The port number to listen on

  int  portNum;

  //! The client file descriptor

  int  clientFd;

  // Implementation specific routines to handle individual chars.

  virtual bool  putRspCharRaw (char  c);
  virtual int   getRspCharRaw (bool blocking);

};	// RspConnection ()

#endif	// RSP_CONNECTION_H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
