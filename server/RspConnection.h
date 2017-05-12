// Remote Serial Protocol connection: definition

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
// ----------------------------------------------------------------------------

#ifndef RSP_CONNECTION__H
#define RSP_CONNECTION__H

#include "RspPacket.h"
#include "TraceFlags.h"


//! The default service to use if port number = 0 and no service specified
#define DEFAULT_RSP_SERVICE  "or1ksim-rsp"


//! Class implementing the RSP connection listener

//! This class is entirely passive. It is up to the caller to determine that a
//! packet will become available before calling ::getPkt ().
class RspConnection
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

  // Public interface: get packets from the stream and put them out
  bool  getPkt (RspPacket *pkt);
  bool  putPkt (RspPacket *pkt);


private:

  //! The port number to listen on
  int  portNum;

  //! Trace flags
  TraceFlags *traceFlags;

  //! The client file descriptor
  int  clientFd;

  // Internal routines to handle individual chars
  bool  putRspChar (char  c);
  int   getRspChar ();

};	// RspConnection ()

#endif	// RSP_CONNECTION__H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
