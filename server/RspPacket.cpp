// RSP packet: implementation

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

#include <iomanip>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <cstdio>

#include "RspPacket.h"
#include "Utils.h"


using std::ostream;
using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::setfill;
using std::setw;


//! Constructor

//! Allocate the new data buffer

//! @param[in]  _rspConnection  The RSP connection we will use
//! @param[in]  _bufSize        Size of data buffer to allocate
RspPacket::RspPacket (int  _bufSize) :
  bufSize (_bufSize)
{
  data = new char [_bufSize];

}	// RspPacket ();


//! Destructor

//! Give back the data buffer
RspPacket::~RspPacket ()
{
  delete [] data;

}	// ~RspPacket ()


//! Pack a string into a packet.

//! A convenience version of this method.

//! @param  str  The string to copy into the data packet before sending
void
RspPacket::packStr (const char *str)
{
  std::size_t slen = strlen (str);

  // Construct the packet to send, so long as string is not too big, otherwise
  // truncate. Add EOS at the end for convenient debug printout
  if (slen >= bufSize)
    {
      cerr << "Warning: String \"" << str
		<< "\" too large for RSP packet: truncated\n" << endl;
      slen = bufSize - 1;
    }

  strncpy (data, str, slen);
  data[slen] = 0;
  len        = slen;

}	// packStr ()


//! Pack a const string as a hex encoded string into a packet for qRcmd.

//! The reply to qRcmd packets can be O followed by hex encoded ASCII.

//! @param  str  The string to copy into the data packet before sending
void
RspPacket::packHexstr (const char *str)
{
  std::size_t slen = strlen (str);

  // Construct the packet to send, so long as string is not too big, otherwise
  // truncate. Add EOS at the end for convenient debug printout
  if (slen >= (bufSize / 2 - 1))
    {
      cerr << "Warning: String \"" << str
		<< "\" too large for RSP packet: truncated\n" << endl;
      slen = bufSize / 2 - 1;
    }

  // Construct the string the hard way
  data[0] = 'O';
  for (std::size_t i = 0; i < slen; i++)
    {
      int nybble_hi = str[i] >> 4;
      int nybble_lo = str[i] & 0x0f;

      data[i * 2 + 1] = nybble_hi + (nybble_hi > 9 ? 'a' - 10 : '0');
      data[i * 2 + 2] = nybble_lo + (nybble_lo > 9 ? 'a' - 10 : '0');
    }
  len       = slen * 2 + 1;
  data[len] = 0;

}	// packStr ()


//! Pack a const string as a hex encoded string into a packet for qRcmd.

//! The reply to qRcmd packets can be O followed by hex encoded ASCII and the
//! client will print them on standard output. If there is no initial O, then
//! the code is silently put into a buffer by the client.

//! @param  str        The string to copy into the data packet before sending
//! @param  toStdoutP  TRUE if the client should send to stdout, FALSE if the
//!                    result should silently go into a buffer.

void
RspPacket::packRcmdStr (const char *str,
			const bool toStdoutP)
{
  std::size_t slen = strlen (str);

  // Construct the packet to send, so long as string is not too big, otherwise
  // truncate. Add EOS at the end for convenient debug printout
  if (slen >= (bufSize / 2 - 1))
    {
      cerr << "Warning: String \"" << str
		<< "\" too large for RSP packet: truncated\n" << endl;
      slen = bufSize / 2 - 1;
    }

  // Construct the string the hard way
  int offset;
  if (toStdoutP)
    {
      data[0] = 'O';
	  offset = 1;
    }
  else
    {
      offset = 0;
    }

  for (unsigned int i = 0; i < slen; i++)
    {
      uint8_t nybble_hi = str[i] >> 4;
      uint8_t nybble_lo = str[i] & 0x0f;

      data[i * 2 + offset + 0] =
	  static_cast<char> (nybble_hi + (nybble_hi > 9 ? 'a' - 10 : '0'));
      data[i * 2 + offset + 1] =
	  static_cast<char> (nybble_lo + (nybble_lo > 9 ? 'a' - 10 : '0'));
    }
  len       = slen * 2 + offset;
  data[len] = 0;

}	// packRcmdStr ()


//! Get the data buffer size

//! @return  The data buffer size
int
RspPacket::getBufSize ()
{
  return  bufSize;

}	// getBufSize ()


//! Get the current number of chars in the data buffer

//! @return  The number of chars in the data buffer
int
RspPacket::getLen ()
{
  return  len;

}	// getLen ()


//! Set the number of chars in the data buffer

//! @param[in] _len  The number of chars to be set
void
RspPacket::setLen (int  _len)
{
  len = _len;

}	// setLen ()


//! Output stream operator

//! @param[out] s  Stream to output to
//! @param[in]  p  Packet to output
ostream &
operator<< (ostream   &s,
	    RspPacket &p)
{
  return  s << "RSP packet: " << std::dec << std::setw (3) << p.getLen()
	    << std::setw (0) << " chars, \"" << p.data << "\"";

}	// operator<< ()
