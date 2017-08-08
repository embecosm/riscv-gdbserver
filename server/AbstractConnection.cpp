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

#include "AbstractConnection.h"
#include "Utils.h"

using std::cerr;
using std::cout;
using std::dec;
using std::endl;
using std::flush;
using std::hex;
using std::setfill;
using std::setw;


//! Get the next packet from the RSP connection

//! Modeled on the stub version supplied with GDB. This allows the user to
//! replace the character read function, which is why we get stuff a character
//! at at time.

//! Unlike the reference implementation, we don't deal with sequence
//! numbers. GDB has never used them, and this implementation is only intended
//! for use with GDB 6.8 or later. Sequence numbers were removed from the RSP
//! standard at GDB 5.0.

//! @param[in] pkt  The packet for storing the result.

//! @return  TRUE to indicate success, FALSE otherwise (means a communications
//!          failure)
bool
AbstractConnection::getPkt (RspPacket *pkt)
{
  // Keep getting packets, until one is found with a valid checksum
  while (true)
    {
      int            bufSize = pkt->getBufSize ();
      unsigned char  checksum;		// The checksum we have computed
      int            count;		// Index into the buffer
      int 	     ch;		// Current character


      // Wait around for the start character ('$'). Ignore all other
      // characters
      ch = getRspChar ();
      while (ch != '$')
	{
	  if (-1 == ch)
	    {
	      return  false;		// Connection failed
	    }
	  else
	    {
 	      ch = getRspChar ();
	    }
	}

      // Read until a '#' or end of buffer is found
      checksum =  0;
      count    =  0;
      while (count < bufSize - 1)
	{
	  ch = getRspChar ();

	  if (-1 == ch)
	    {
	      return  false;		// Connection failed
	    }

	  // If we hit a start of line char begin all over again
	  if ('$' == ch)
	    {
	      checksum =  0;
	      count    =  0;

	      continue;
	    }

	  // Break out if we get the end of line char
	  if ('#' == ch)
	    {
	      break;
	    }

	  // Update the checksum and add the char to the buffer
	  checksum         = checksum + (unsigned char)ch;
	  pkt->data[count] = (char)ch;
	  count++;
	}

      // Mark the end of the buffer with EOS - it's convenient for non-binary
      // data to be valid strings.
      pkt->data[count] = 0;
      pkt->setLen (count);

      // If we have a valid end of packet char, validate the checksum. If we
      // don't it's because we ran out of buffer in the previous loop.
      if ('#' == ch)
	{
	  unsigned char  xmitcsum;	// The checksum in the packet

	  ch = getRspChar ();
	  if (-1 == ch)
	    {
	      return  false;		// Connection failed
	    }
	  xmitcsum = Utils::char2Hex (ch) << 4;

	  ch = getRspChar ();
	  if (-1 == ch)
	    {
	      return  false;			// Connection failed
	    }

	  xmitcsum += Utils::char2Hex (ch);

	  // If the checksums don't match print a warning, and put the
	  // negative ack back to the client. Otherwise put a positive ack.
	  if (checksum != xmitcsum)
	    {
	      cerr << "Warning: Bad RSP checksum: Computed 0x"
			<< setw (2) << setfill ('0') << hex
			<< checksum << ", received 0x" << xmitcsum
			<< setfill (' ') << dec << endl;
	      if (!putRspChar ('-'))		// Failed checksum
		{
		  return  false;		// Comms failure
		}
	    }
	  else
	    {
	      if (!putRspChar ('+'))		// successful transfer
		{
		  return  false;		// Comms failure
		}
	      else
		{
		  if (traceFlags->traceRsp())
		    {
		      cout << "RSP trace: getPkt: " << *pkt << endl;
		    }

		  return  true;			// Success
		}
	    }
	}
      else
	{
	  cerr << "Warning: RSP packet overran buffer" << endl;
	}
    }

}	// getPkt ()


//! Put the packet out on the RSP connection

//! Modeled on the stub version supplied with GDB. Put out the data preceded
//! by a '$', followed by a '#' and a one byte checksum. '$', '#', '*' and '}'
//! are escaped by preceding them with '}' and then XORing the character with
//! 0x20.

//! @param[in] pkt  The Packet to transmit

//! @return  TRUE to indicate success, FALSE otherwise (means a communications
//!          failure).
bool
AbstractConnection::putPkt (RspPacket *pkt)
{
  int  len = pkt->getLen ();
  int  ch;				// Ack char

  // Construct $<packet info>#<checksum>. Repeat until the GDB client
  // acknowledges satisfactory receipt.
  do
    {
      unsigned char checksum = 0;	// Computed checksum
      int           count    = 0;	// Index into the buffer

      if (!putRspChar ('$'))		// Start char
	{
	  return  false;		// Comms failure
	}


      // Body of the packet
      for (count = 0; count < len; count++)
	{
	  unsigned char  ch = pkt->data[count];

	  // Check for escaped chars
	  if (('$' == ch) || ('#' == ch) || ('*' == ch) || ('}' == ch))
	    {
	      ch       ^= 0x20;
	      checksum += (unsigned char)'}';
	      if (!putRspChar ('}'))
		{
		  return  false;	// Comms failure
		}

	    }

	  checksum += ch;
	  if (!putRspChar (ch))
	    {
	      return  false;		// Comms failure
	    }
	}

      if (!putRspChar ('#'))		// End char
	{
	  return  false;		// Comms failure
	}

      // Computed checksum
      if (!putRspChar (Utils::hex2Char (checksum >> 4)))
	{
	  return  false;		// Comms failure
	}
      if (!putRspChar (Utils::hex2Char (checksum % 16)))
	{
	  return  false;		// Comms failure
	}

      // Check for ack of connection failure
      ch = getRspChar ();
      if (-1 == ch)
	{
	  return  false;		// Comms failure
	}
    }
  while ('+' != ch);

  if (traceFlags->traceRsp())
    {
      cout << "RSP trace: putPkt: " << *pkt << endl;
    }

  return  true;

}	// putPkt ()


//! Put a single character out on the RSP connection

//! Potentially we can have an OS specific implemenation of the underlying
//! routine.

//! @param[in] c  The character to put out
//! @return  TRUE if char sent OK, FALSE if not (communications failure)

bool
AbstractConnection::putRspChar (char  c)
{
  return  putRspCharRaw (c);

}	// putRspChar ()



//! Get a single character from the RSP connection with buffering

//! Utility routine for use by other functions.  This is built on the raw
//! read function.

//! This function will first return the possibly buffered character (buffering
//! caused by calling 'haveBreak'.  If the character read is the break
//! character then we record this fact, and fetch the next character.

//! @return  The character received or -1 on failure

int
AbstractConnection::getRspChar ()
{
  int ch;

  if (mNumGetBufChars > 1)
    cerr << "Warning: Too many cached characters ("
	 << dec << mNumGetBufChars << ")" << endl;

  if (mNumGetBufChars > 0)
    {
      ch = mGetCharBuf;
      mNumGetBufChars = 0;
    }
  else
    ch = getRspCharRaw (true);

  return  ch;

}	// getRspChar ()


//! Have we received a break character.

//! Since we only check fo this between packets, we don't have to worry about
//! being in the middle of a packet.

//! @Note  We only peek, so no character is actually consumed from the input.

//! @return  TRUE if we have received a break character, FALSE otherwise.

bool
AbstractConnection::haveBreak ()
{
  if (!mHavePendingBreak
      && mNumGetBufChars == 0)
    {
      // Non-blocking read to possibly get a character.

      int nextChar = getRspCharRaw (false);

      if (nextChar != -1)
	{
	  if (nextChar == BREAK_CHAR)
	    mHavePendingBreak = true;
	  else
	    {
	      mGetCharBuf = nextChar;
	      mNumGetBufChars = 1;
	    }
	}
    }

  if (mHavePendingBreak)
    {
      mHavePendingBreak = false;
      return true;
    }
  else
    return false;

}	// haveBreak ()
