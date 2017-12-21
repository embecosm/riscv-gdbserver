// Process syscall reply packet.

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

#ifndef SYSCALLREPLYPACKET_H
#define SYSCALLREPLYPACKET_H

class SyscallReplyPacket
{
public:
  //! Constructor.

  SyscallReplyPacket ()
    : mValid (false)
  {
    // Nothing.
  }

  //! Parse syscall reply packet in DATA, updating class member variables
  //! as appropriate.

  void parse (const char *data)
  {
    int retcode, error;

    // Reset to invalid.
    mValid = false;

    if (*data != 'F')
      return;
    ++data;

    /* Parse the return code.  */
    if (!parseValue (&data, &retcode))
      return;

    if (*data == '\0')
      {
        mValid = true;
        mRetCode = retcode;
        mCtrlC = false;
        return;
      }

    if (*data != ',')
      return;
    ++data;

    // Parse the error code.
    if (!parseValue (&data, &error))
      return;
    if (error < 0)
      return;
    if (error > 0)
      {
        // We have a non-zero error code, indicating an error occurred, in
        // this case the result should be -1.
        if (retcode != -1)
          return;

        // We place the negative errno in the result register.
        retcode = -error;
      }

    if (*data == '\0')
      {
        mValid = true;
        mRetCode = retcode;
        mCtrlC = false;
        return;
      }

    if (*data != ',')
      return;
    ++data;

    // If we get this far then we're expecting a Ctrl-C marker.
    if (*data != 'C')
      return;

    mValid = true;
    mRetCode = retcode;
    mCtrlC = true;
  }

  //! Return the parsed syscall return code.  This is only correct if VALID
  //! is true.

  int retcode () const
  {
    return mRetCode;
  }

  //! Return true if the syscall reply contains a Ctrl-C marker.  This is
  //! only correct if VALID is true.

  bool hasCtrlC () const
  {
    return mCtrlC;
  }

  // Return true if the syscall reply packet parsed correctly, otherwise,
  // return false.

  bool valid () const
  {
    return mValid;
  }

private:

  //! Parse one of the value sub-fields within the syscall reply packet.
  //! The parsed value is placed in the int pointed to by VALUEP, and the
  //! string pointed to by STRP is updated to point to the character just
  //! after the parsed value.  Return true if a value was parsed (VALUEP
  //! will have been updated), otherwise return false (VALUEP will be
  //! unchanged).

  static bool
  parseValue (const char **strp, int *valuep)
  {
    long int val;
    char *end;
    char **endptr = &end;

    val = strtol (*strp, endptr, 16);

    // Nothing was parsed.
    if (*endptr == *strp)
      return false;

    // Hit something strange after the number.
    if (**endptr != '\0' && **endptr != ',')
      return false;

    // Update where the end of the string is.
    *strp = *endptr;

    *valuep = (int) val;
    return true;
  }

  //! The result value parsed from the syscall reply packet, will be either
  //! the result, or the negative errno value if the syscall failed.
  int mRetCode;

  //! Will be true if the syscall reply contained a Ctrl-C marker.
  bool mCtrlC;

  //! Set true if the syscall reply packet parsed correctly, otherwise it
  //! will be set to false.
  bool mValid;
};

#endif /* SYSCALLREPLYPACKET_H */

// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
