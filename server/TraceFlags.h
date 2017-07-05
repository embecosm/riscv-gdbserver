// GDB RSP server: definition

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

#ifndef TRACE_FLAGS__H
#define TRACE_FLAGS__H


//! Class for trace flags

//! The flags themselves are bits in an unsigned integer

class TraceFlags
{
public:

  // Constructor and destructor

  TraceFlags (unsigned int  _Flags = TRACE_NONE);
  ~TraceFlags ();

  // Accessors

  bool traceRsp () const;
  void traceRsp (const bool  flagState);
  bool traceConn () const;
  bool traceBreak () const;
  bool traceVcd () const;
  bool traceSilent () const;
  void setSilent ();
  void allFlags (const unsigned int  flags);
  unsigned int allFlags () const;


private:

  // Definition of flag values

  static const unsigned int TRACE_MASK   = 0x8000000f;	//!< Trace flag mask
  static const unsigned int TRACE_NONE   = 0x00000000;	//!< Trace nothing
  static const unsigned int TRACE_RSP    = 0x00000001;	//!< Trace RSP packets
  static const unsigned int TRACE_CONN   = 0x00000002;	//!< Trace connection
  static const unsigned int TRACE_BREAK  = 0x00000004;	//!< Trace breakpoints
  static const unsigned int TRACE_VCD    = 0x00000008;	//!< Generate VCD

  static const unsigned int TRACE_SILENT = 0x80000000;  //!< Reduce messages

  //! The trace flags

  unsigned int  flags;

};	// TraceFlags ()

#endif	// TRACE_FLAGS__H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
