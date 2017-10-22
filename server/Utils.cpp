// GDB Server Utilties: implementation

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

#include <iostream>

#include "Utils.h"

using std::cout;
using std::hex;
using std::endl;
using std::string;
using std::vector;


//!Utility to give the value of a hex char

//! @param[in] ch  A character representing a hexadecimal digit. Done as -1,
//!                for consistency with other character routines, which can
//!                use -1 as EOF.

//! @return  The value of the hex character, or -1 if the character is
//!          invalid.
uint8_t
Utils::char2Hex (int  c)
{
  return  ((c >= 'a') && (c <= 'f')) ? c - 'a' + 10 :
          ((c >= '0') && (c <= '9')) ? c - '0' :
          ((c >= 'A') && (c <= 'F')) ? c - 'A' + 10 : -1;

}	// char2Hex ()


//! Utility mapping a value to hex character

//! @param[in] d  A hexadecimal digit. Any non-hex digit returns a NULL char
char
Utils::hex2Char (uint8_t  d)
{
  static const char map [] = "0123456789abcdef"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                             "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

  return  map[d];

}	// hex2Char ()


//! Convert a value to a hex digit string

//! The supplied value is converted to a (numBytes * 2) digit hex string. The
//! string is null terminated for convenience.

//! Rather bizarrely, GDB seems to expect the bytes in the string to be
//! ordered according to target endianism

//! @param[in]  val              the value to convert
//! @param[out] buf              the buffer for the text string
//! @param[in]  numBytes         the number of significant bytes in val
//! @param[in]  isLittleEndianP  true if this is a little endian architecture.
void
Utils::val2Hex (uint64_t  val,
		char     *buf,
		int       numBytes,
		bool      isLittleEndianP)
{
  if (isLittleEndianP)
    {
      for (int  n = 0 ; n < numBytes; n++)
	{
	  unsigned char  byte = val & 0xff;

	  buf [n * 2    ] = hex2Char ((byte >> 4) & 0xf);
	  buf [n * 2 + 1] = hex2Char ( byte       & 0xf);

	  val = val / 256;
	}
    }
  else
    {
      for (int  n = numBytes - 1 ; n >= 0; n--)
	{
	  unsigned char  byte = val & 0xff;

	  buf [n * 2    ] = hex2Char ((byte >> 4) & 0xf);
	  buf [n * 2 + 1] = hex2Char ( byte       & 0xf);

	  val = val / 256;
	}
    }

  buf[numBytes * 2] = '\0';		// Useful to terminate as string

}	// val2Hex ()


//! Convert a hex digit string to a register value

//! The supplied (numBytes * 2) digit hex string

//! Rather bizarrely, GDB seems to expect the bytes in the string to be
//! ordered according to target endianism

//! @param[in] buf              the buffer with the hex string
//! @param[in] numBytes         the number of significant bytes in val
//! @param[in] isLittleEndianP  true if this is a little endian architecture.

//! @return  The value to convert
uint64_t
Utils::hex2Val (char *buf,
		int   numBytes,
		bool  isLittleEndianP)
{
  uint64_t  val       = 0;		// The result

  if (isLittleEndianP)
    {
      for (int  n = numBytes - 1; n >= 0; n--)
	{
	  val = (val << 4) | char2Hex (buf[n * 2    ]);
	  val = (val << 4) | char2Hex (buf[n * 2 + 1]);
	}
    }
  else
    {
      for (int  n = 0; n < numBytes; n++)
	{
	  val = (val << 4) | char2Hex (buf[n * 2    ]);
	  val = (val << 4) | char2Hex (buf[n * 2 + 1]);
	}
    }

  return val;

}	// hex2Val ()


//! Convert an ASCII character string to pairs of hex digits

//! Both source and destination are null terminated.

//! @param[out] dest  Buffer to store the hex digit pairs (null terminated)
//! @param[in]  src   The ASCII string (null terminated)                      */
void
Utils::ascii2Hex (char *dest,
		  char *src)
{
  int  i;

  // Step through converting the source string
  for (i = 0; src[i] != '\0'; i++)
    {
      char  ch = src[i];

      dest[i * 2]     = hex2Char(ch >> 4 & 0xf);
      dest[i * 2 + 1] = hex2Char(ch      & 0xf);
    }

  dest[i * 2] = '\0';

}	// ascii2hex ()


//! Convert pairs of hex digits to an ASCII character string

//! Both source and destination are null terminated.

//! @param[out] dest  The ASCII string (null terminated)
//! @param[in]  src   Buffer holding the hex digit pairs (null terminated)
void
Utils::hex2Ascii (char *dest,
		  char *src)
{
  int  i;

  // Step through convering the source hex digit pairs
  for (i = 0; src[i * 2] != '\0' && src[i * 2 + 1] != '\0'; i++)
    {
      dest[i] = ((char2Hex (src[i * 2]) & 0xf) << 4) |
	         (char2Hex (src[i * 2 + 1]) & 0xf);
    }

  dest[i] = '\0';

}	// hex2ascii ()


//! "Unescape" RSP binary data

//! '#', '$' and '}' are escaped by preceding them by '}' and oring with 0x20.

//! This function reverses that, modifying the data in place.

//! @param[in] buf  The array of bytes to convert
//! @para[in]  len   The number of bytes to be converted

//! @return  The number of bytes AFTER conversion
int
Utils::rspUnescape (char *buf,
		    int   len)
{
  int  fromOffset = 0;		// Offset to source char
  int  toOffset   = 0;		// Offset to dest char

  while (fromOffset < len)
    {
      // Is it escaped
      if ( '}' == buf[fromOffset])
	{
	  fromOffset++;
	  buf[toOffset] = buf[fromOffset] ^ 0x20;
	}
      else
	{
	  buf[toOffset] = buf[fromOffset];
	}

      fromOffset++;
      toOffset++;
    }

  return  toOffset;

}	// rspUnescape () */


//! Split a string into delimited tokens

//! @param[in]  s      The string of tokes
//! @param[in]  delim  The delimiter characters
//! @param[out] elems  Vector the individual tokens

//! @return  The vector of tokens

vector<string> &
Utils::split (const string        & s,
	      const string        & delim,
	      vector<string> & elems)
{
  elems.clear();
  size_t current;
  size_t next = s.npos;

  do
    {
      next = s.find_first_not_of(delim, next + 1);
      if(next == s.npos)
        {
	  break;
        }
      current = next;
      next = s.find_first_of(delim, current);
      elems.push_back(s.substr(current, next - current));
    }
  while (next != s.npos);

  return elems;

}	// split ()
