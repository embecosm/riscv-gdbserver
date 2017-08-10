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

#include <iostream>
#include <strings.h>

#include "TraceFlags.h"


using std::cerr;
using std::endl;


// ****************************************************************************
// ****************************************************************************
//
// class TraceFlags::iterator definitions
//
// ****************************************************************************
// ****************************************************************************


//! Allocate the static vector of flags.

std::vector<TraceFlags::FlagInfo> TraceFlags::sFlagInfo;


//! Constructor for the iterator subclass

//! @param[in] flagNum  The bit location at which to start.

TraceFlags::iterator::iterator (uint8_t flagNum = 0)
  : mFlagNum (flagNum)
{

}	// TraceFlags::iterator::iterator ()


//! Prefix increment operator for iterator

TraceFlags::iterator &
TraceFlags::iterator::operator++ ()
{
  mFlagNum++;
  return *this;

}	// TraceFlags::iterator::operator++ ()


//! Postfix increment operator for iterator

TraceFlags::iterator
TraceFlags::iterator::operator++ (int junk __attribute__ ((unused)) )
{
  iterator retval = *this;
  ++(*this);

  return retval;

}	// TraceFlags::iterator::operator++ ()


//! Equality operator for iterator

//! @param[in]  Second operand of the operator

bool
TraceFlags::iterator::operator== (iterator other) const
{
  return mFlagNum == other.mFlagNum;

}	// TraceFlags::iterator::operator== ()


//! Inequality operator for iterator

//! @param[in]  Second operand of the operator

bool
TraceFlags::iterator::operator!= (iterator other) const
{
  return !(*this == other);

}	// TraceFlags::iterator::operator!= ()


//! Indirection operator for iterator

//! @return The name of the flag associated with the iterator.

const char *
TraceFlags::iterator::operator* () const
{
  if (mFlagNum < sFlagInfo.size ())
    return sFlagInfo[mFlagNum].flagName;
  else
    return "";

}	// TraceFlags::iterator::operator* ()


// ****************************************************************************
// ****************************************************************************
//
// class TraceFlags definitions
//
// ****************************************************************************
// ****************************************************************************


//! Constructor for the trace flags.

TraceFlags::TraceFlags () :
  mFlags (0)
{
  // Initialize the vector of flag info if not yet done

  if (sFlagInfo.empty ())
    {
      sFlagInfo.push_back ({ TRACE_RSP,    "rsp"    });
      sFlagInfo.push_back ({ TRACE_CONN,   "conn"   });
      sFlagInfo.push_back ({ TRACE_BREAK,  "break"  });
      sFlagInfo.push_back ({ TRACE_VCD,    "vcd"    });
      sFlagInfo.push_back ({ TRACE_SILENT, "silent" });
      sFlagInfo.push_back ({ TRACE_DISAS,  "disas"  });
    }
}	// TraceFlags::TraceFlags ()


//! Destructor for the trace flags.

TraceFlags::~TraceFlags ()
{

}	// TraceFlags::~TraceFlags ()


//! Is RSP tracing enabled?

//! @return  TRUE if the RSP tracing flag is set, FALSE otherwise

bool
TraceFlags::traceRsp () const
{
  return (mFlags & TRACE_RSP) == TRACE_RSP;

}	// TraceFlags::traceRsp ()


//! Is connection tracing enabled?

//! @return  TRUE if the CONN tracing flag is set, FALSE otherwise

bool
TraceFlags::traceConn () const
{
  return (mFlags & TRACE_CONN) == TRACE_CONN;

}	// TraceFlags::traceConn ()


//! Is breakpoint tracing enabled?

//! @return  TRUE if the BREAK tracing flag is set, FALSE otherwise

bool
TraceFlags::traceBreak () const
{
  return (mFlags & TRACE_BREAK) == TRACE_BREAK;

}	// TraceFlags::traceBreak ()


//! Is VCD tracing enabled?

//! @return  TRUE if the VCD tracing flag is set, FALSE otherwise

bool
TraceFlags::traceVcd () const
{
  return (mFlags & TRACE_VCD) == TRACE_VCD;

}	// TraceFlags::traceVcd ()


//! Is silent running enabled?

//! @return  TRUE if the SILENT tracing flag is set, FALSE otherwise

bool
TraceFlags::traceSilent () const
{
  return (mFlags & TRACE_SILENT) == TRACE_SILENT;

}	// TraceFlags::traceSilent ()


//! Is disassembly enabled?

//! @return  TRUE if the DISAS tracing flag is set, FALSE otherwise

bool
TraceFlags::traceDisas () const
{
  return (mFlags & TRACE_DISAS) == TRACE_DISAS;

}	// TraceFlags::traceDisas ()


//! Is this a real flag

//! @param[in] flagName  Case insensitive name to check.
//! @return  TRUE if this is a valid flag name, FALSE otherwise.

bool
TraceFlags::isFlag (const char *flagName) const
{
  return  flagLookup (flagName) != TRACE_BAD;

}	// TraceFlags::isFlag ()


//! Set a named flag

//! The flag is assumed to exist, and if it does not bad things will happen!
//! Use isFlag () to check.

//! @param[in] flagName  The name of the flag (case insensitive)
//! @param[in] flagVal   Value to set

void
TraceFlags::flag (const char * flagName,
		  const bool   flagVal)
{
  uint32_t  flagBit = flagLookup (flagName);

  if (flagBit == TRACE_BAD)
    {
      cerr << "*** ABORT *** Attempt to set bad trace flag" << endl;
      abort ();
    }

  if (flagVal)
    mFlags |= flagBit;
  else
    mFlags &= ~flagBit;

}	// TraceFlags::flag ()


//! Get a named flag

//! The flag is assumed to exist, and if it does not bad things will happen!
//! Use isFlag () to check.

//! @param[in] flagName  The name of the flag (case insensitive)
//! return  Value of the flag.

bool
TraceFlags::flag (const char * flagName) const
{
  uint32_t  flagBit = flagLookup (flagName);

  if (flagBit == TRACE_BAD)
    {
      cerr << "*** ABORT *** Attempt to set bad trace flag" << endl;
      abort ();
    }

  return (mFlags & flagBit) == flagBit;

}	// TraceFlags::flag ()


//! Begin iteration

//! @return  iterator pointing to start of flags

TraceFlags::iterator
TraceFlags::begin ()
{
  return iterator (0);

}	// TraceFlags::begin ()


//! End iteration

//! @return  iterator pointing after end of flags

TraceFlags::iterator
TraceFlags::end ()
{
  return iterator (sFlagInfo.size ());

}	// TraceFlags::end ()


//! Look up a flag's bit string

//! @param[in]  flagName  The name of the flag (case insensitive)
//! return  flagBit   The flag bit or TRACE_BAD if the flag does not exist.

uint32_t
TraceFlags::flagLookup (const char * flagName) const
{
  for (auto it = sFlagInfo.begin () ; it != sFlagInfo.end (); it++)
    if (0 == strcasecmp (flagName, it->flagName))
      return it->flagBit;

  return TRACE_BAD;

}	// TraceFlags::flagLookup ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
