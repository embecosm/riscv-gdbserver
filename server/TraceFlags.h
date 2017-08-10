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

#include <iterator>
#include <vector>


//! Class for trace flags

//! The interface only uses names of flags. Flags themselves are bits in an
//! unsigned integer. We provide an enumerator to look at all flags.

class TraceFlags
{
public:

  // Iterator subclass. This iterates over the textual names of the trace
  // flags.

  class iterator : public std::iterator <std::input_iterator_tag, uint8_t,
					 uint8_t, const char ** , uint8_t>
  {
  public:

    // Constructor

    explicit iterator (uint8_t flagNum);

    // Operators

    iterator & operator++ ();
    iterator operator++ (int junk);
    bool operator== (iterator other) const;
    bool operator!= (iterator other) const;
    const char * operator* () const;

  private:

    uint8_t  mFlagNum;			// The flag we are after
  };

  // Constructor and destructor

  TraceFlags ();
  ~TraceFlags ();

  // Accessors

  bool traceRsp () const;
  bool traceConn () const;
  bool traceBreak () const;
  bool traceVcd () const;
  bool traceSilent () const;
  bool traceDisas () const;
  bool isFlag (const char *flagName) const;
  void flag (const char *flagName,
	     const bool  val);
  bool flag (const char *flagName) const;

  // Iterators

  iterator begin ();
  iterator end ();


private:

  // Definition of flag values

  static const unsigned int TRACE_RSP    = 0x00000001;	//!< Trace RSP packets
  static const unsigned int TRACE_CONN   = 0x00000002;	//!< Trace connection
  static const unsigned int TRACE_BREAK  = 0x00000004;	//!< Trace breakpoints
  static const unsigned int TRACE_VCD    = 0x00000008;	//!< Generate VCD
  static const unsigned int TRACE_SILENT = 0x00000010;  //!< Reduce messages
  static const unsigned int TRACE_DISAS  = 0x00000020;  //!< Reduce messages

  static const unsigned int TRACE_NONE   = 0x00000000;	//!< Trace nothing
  static const unsigned int TRACE_BAD    = 0xffffffff;	//!< Invalid flag bit

  struct FlagInfo
  {
    const uint32_t  flagBit;
    const char *    flagName;
  };

  //! All the info about flags.

  //! We'd really like this to be a static const, but you can't do that with
  //! initialized structures in a class.  In practice there will only ever be
  //! one instance, so it doesn't matter.

  static std::vector<FlagInfo> sFlagInfo;

  //! The trace flags

  uint32_t  mFlags;

  // Helper functions

  uint32_t  flagLookup (const char * flagName) const;

};	// TraceFlags ()

#endif	// TRACE_FLAGS__H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
