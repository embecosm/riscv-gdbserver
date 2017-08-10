// Syscall information class: declaration.

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

#ifndef SYSCALL_INFO_H
#define SYSCALL_INFO_H


#include <cstddef>
#include <cstdint>
#include <iostream>


//! A class to capture all the details of a host I/O call.

//! The target can request various system calls to be made on the host on its
//! behalf by GDB.

class SyscallInfo
{
 public:

  //! Supported syscalls

  enum Syscall : int {
    OPEN,
    CLOSE,
    READ,
    WRITE,
    LSEEK,
    RENAME,
    UNLINK,
    STAT,
    FSTAT,
    GETTIMEOFDAY,
    ISATTY,
    SYSTEM
  };

  // Constructor and destructor

  SyscallInfo (void);
  ~SyscallInfo (void);

  // Create a File-I/O request with the provided arguments

  void  makeRequest (Syscall  syscall,
		     uint32_t  arg0 = 0,
		     uint32_t  arg1 = 0,
		     uint32_t  arg2 = 0);

  // Build a File-I/O result with the provided return and optional errno.

  void  makeResult (uint32_t  retCode);
  void  makeResult (uint32_t  retCode,
		    uint32_t  retErrno);

  // Accessors for request details

  Syscall  sysCall (void) const;
  uint32_t arg (std::size_t index) const;

  // Accessors for result details

  uint32_t retCode (void) const;
  bool     haveErrno (void) const;
  uint32_t retErrno (void) const;

 private:

  //! The syscall requested.

  Syscall  mSyscall;

  //! The arguments to the syscall.

  uint32_t mArgs[3];

  //! The return code if we have one.

  uint32_t mRetCode;

  //! Do we have an error number?

  bool     mHaveErrno;

  //! The error number if we have one.

  uint32_t mErrno;
};


// Output operator for scoped enumeration

std::ostream &
operator<< (std::ostream & s,
	    SyscallInfo::Syscall  p);


#endif //SYSCALL_INFO_H


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
