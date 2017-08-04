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

#ifndef ABSTRACT_CONNECTION__H
#define ABSTRACT_CONNECTION__H

#include "RspPacket.h"
#include "TraceFlags.h"


//! Class implementing the RSP connection listener

//! This class is entirely passive. It is up to the caller to determine that a
//! packet will become available before calling ::getPkt ().

class AbstractConnection
{
public:

  // Constructor and Destructor

  AbstractConnection (TraceFlags *_traceFlags);
  virtual ~AbstractConnection () = 0;

  // Public interface: manage client connections

  virtual bool  rspConnect () = 0;
  virtual void  rspClose () = 0;
  virtual bool  isConnected () = 0;

  // Public interface: get packets from the stream and put them out

  virtual bool  getPkt (RspPacket *pkt);
  virtual bool  putPkt (RspPacket *pkt);

  // Check for a break (ctrl-C)

  virtual bool  haveBreak ();

protected:

  //! Trace flags

  TraceFlags *traceFlags;

  // Internal OS specific routines to handle individual chars.

  virtual bool  putRspCharRaw (char  c) = 0;
  virtual int   getRspCharRaw (bool blocking) = 0;

private:

  //! The BREAK character

  static const int BREAK_CHAR = 3;

  //! Has a BREAK arrived?

  bool mHavePendingBreak;

  //! The buffered char for get RspChar
  int  mGetCharBuf;

  //! Count of how many buffered chars we have
  int  mNumGetBufChars;

  // Internal routines to handle individual chars

  bool  putRspChar (char  c);
  int   getRspChar ();
};	// AbstractConnection ()

// Default implementation of the destructor.

inline
AbstractConnection::~AbstractConnection ()
{
  // Nothing.
}

inline
AbstractConnection::AbstractConnection (TraceFlags *_traceFlags) :
  traceFlags (_traceFlags),
  mHavePendingBreak (false),
  mNumGetBufChars (0)
{
  // Nothing.
}

#endif	// ABSTRACT_CONNECTION__H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
