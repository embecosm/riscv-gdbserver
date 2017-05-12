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
// ----------------------------------------------------------------------------


#include "TraceFlags.h"


// ----------------------------------------------------------------------------
// This class is to hold information about tracing.
// ----------------------------------------------------------------------------

//! Constructor for the trace flags.

//! @param[in] _traceFlags  The trace flags
TraceFlags::TraceFlags (unsigned int  _flags) :
  flags (_flags)
{

}	// TraceFlags ()


//! Destructor for the trace flags.

//! @param[in] _traceFlags  The trace flags
TraceFlags::~TraceFlags ()
{

}	// ~TraceFlags ()


//! Is RSP tracing enabled?

//! @return  TRUE if the RSP tracing flag is set, FALSE otherwise
bool
TraceFlags::traceRsp ()
{
  return (flags & TRACE_RSP) == TRACE_RSP;

}	// traceRsp ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
