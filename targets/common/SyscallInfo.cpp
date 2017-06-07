// Syscall information class: definition.

// Copyright (C) 2008-2017  Embecosm Limited <www.embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

// This file is part of the RISC-V GDB server

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.


// Even though ITarget is an abstract class, it requires implementation of the
// stream operators to allow its public scoped enumerations to be output.  It
// also requires implementation of the concrete SyscallInfo class.


#include <iostream>

#include "SyscallInfo.h"


using std::cerr;
using std::endl;


//! Constructor.

//! Does nothing.

SyscallInfo::SyscallInfo (void)
{
}	// SyscallInfo ()


//! Create a syscall request

//! @param[in] syscall  The syscall of the request
//! @param[in] arg0     First argument (if any) to the request
//! @param[in] arg1     Second argument (if any) to the request
//! @param[in] arg2     Third argument (if any) to the request

void
SyscallInfo::makeRequest (SyscallInfo::Syscall syscall,
			  uint32_t  arg0,
			  uint32_t  arg1,
			  uint32_t  arg2)
{
  mSyscall = syscall;
  mArgs[0] = arg0;
  mArgs[1] = arg1;
  mArgs[2] = arg2;

}	// makeRequest ()


//! Create a syscall result with just a return code

//! @param[in] retCode  The return code.

void
SyscallInfo::makeResult (uint32_t retCode)
{
  mRetCode   = retCode;
  mHaveErrno = false;

}	// makeResult ()


//! Create a syscall result with a return code and error nnumber

//! Note that we cannot just use a default value for the argument, since there
//! is a difference between a syscall with no error number and a syscall with
//! an error number of zero.

//! @param[in] retCode  The return code.

void
SyscallInfo::makeResult (uint32_t retCode,
			 uint32_t retErrno)
{
  mRetCode   = retCode;
  mHaveErrno = true;
  mErrno     = retErrno;

}	// makeResult ()


//! Accessor for the syscall

//! @return  The current syscall.

SyscallInfo::Syscall
SyscallInfo::sysCall (void) const
{
  return mSyscall;

}	// sysCall ()


//! Accessor for syscall arguments

//! @param[in] index  Which argument we want
//! @return  Value of the argument requested.

uint32_t
SyscallInfo::arg (std::size_t index) const
{
  if (index < 3)
    return mArgs[index];
  else
    {
      cerr << "Warning: Attempt to access syscall arg # " << index
	   << ": zero returned as result" << endl;
      return 0;
    }

}	// arg ()


//! Accessor for the return code.

//! @return  Value of the return code.

uint32_t
SyscallInfo::retCode (void) const
{
  return mRetCode;

}	// retCode ()


//! Accessor for whether we have an error code.

//! @return  TRUE if we have an error code.

bool
SyscallInfo::haveErrno (void) const
{
  return mHaveErrno;

}	// haveErrno ()


//! Accessor for the error number.

//! @return  Value of the error number.

uint32_t
SyscallInfo::retErrno (void) const
{
  if (haveErrno ())
    {
      cerr << "Warning: Attempt to access non-existant syscall errno"
	   << ": zero returned as result" << endl;
      return 0;
    }
  else
    return mErrno;

}	// retErrno ()


//! Output operator for Syscall enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The MatchType value to output.
//! @return  The stream with the item appended.

std::ostream &
operator<< (std::ostream & s,
	    SyscallInfo::Syscall  p)
{
  const char * name;

  switch (p)
    {
    case SyscallInfo::Syscall::OPEN:         name = "open";         break;
    case SyscallInfo::Syscall::CLOSE:        name = "close";        break;
    case SyscallInfo::Syscall::READ:         name = "read";         break;
    case SyscallInfo::Syscall::WRITE:        name = "write";        break;
    case SyscallInfo::Syscall::LSEEK:        name = "lseek";        break;
    case SyscallInfo::Syscall::RENAME:       name = "rename";       break;
    case SyscallInfo::Syscall::UNLINK:       name = "unlink";       break;
    case SyscallInfo::Syscall::STAT:         name = "stat";         break;
    case SyscallInfo::Syscall::FSTAT:        name = "fstat";        break;
    case SyscallInfo::Syscall::GETTIMEOFDAY: name = "gettimeofday"; break;
    case SyscallInfo::Syscall::SYSTEM:       name = "system";       break;

    default:                                 name = "unknown";      break;
    }

  return  s << name;

}	// operator<< ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
