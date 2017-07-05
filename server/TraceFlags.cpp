// GDB RSP server: implementation

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

#include "TraceFlags.h"


//! Constructor for the trace flags.

//! @param[in] _traceFlags  The trace flags

TraceFlags::TraceFlags (unsigned int  _flags) :
  flags (_flags)
{

}	// TraceFlags::TraceFlags ()


//! Destructor for the trace flags.

//! @param[in] _traceFlags  The trace flags

TraceFlags::~TraceFlags ()
{

}	// TraceFlags::~TraceFlags ()


//! Is RSP tracing enabled?

//! @return  TRUE if the RSP tracing flag is set, FALSE otherwise

bool
TraceFlags::traceRsp () const
{
  return (flags & TRACE_RSP) == TRACE_RSP;

}	// TraceFlags::traceRsp ()


//! Set whether RSP tracing is enabled

//! @param[in] traceRsp  TRUE if the RSP tracing flag should be set, FALSE if
//!                      it should be cleared.

void
TraceFlags::traceRsp (const bool  flagState)
{
  if (flagState)
    flags |= TRACE_RSP;
  else
    flags &= ~TRACE_RSP;

}	// TraceFlags::traceRsp ()


//! Is connection tracing enabled?

//! @return  TRUE if the CONN tracing flag is set, FALSE otherwise

bool
TraceFlags::traceConn () const
{
  return (flags & TRACE_CONN) == TRACE_CONN;

}	// TraceFlags::traceConn ()


//! Is breakpoint tracing enabled?

//! @return  TRUE if the BREAK tracing flag is set, FALSE otherwise

bool
TraceFlags::traceBreak () const
{
  return (flags & TRACE_BREAK) == TRACE_BREAK;

}	// TraceFlags::traceBreak ()


//! Is VCD tracing enabled?

//! @return  TRUE if the VCD tracing flag is set, FALSE otherwise

bool
TraceFlags::traceVcd () const
{
  return (flags & TRACE_VCD) == TRACE_VCD;

}	// TraceFlags::traceVcd ()


//! Is silent running enabled?

//! @return  TRUE if the SILENT tracing flag is set, FALSE otherwise

bool
TraceFlags::traceSilent () const
{
  return (flags & TRACE_SILENT) == TRACE_SILENT;

}	// TraceFlags::traceSilent ()


//! Set silent running

void
TraceFlags::setSilent ()
{
  flags != TRACE_SILENT;

}	// TraceFlags::setSilent ()


//! Set all trace flags.

//! @param[in] _flags  The values to set.

void
TraceFlags::allFlags (const unsigned int  _flags)
{
  flags = _flags;

}	// TraceFlags::allFlags ()


//! Get all trace flags

//! @ return  The flags value.

unsigned int
TraceFlags::allFlags () const
{
  return  flags;

}	// TraceFlags::allFlags ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
