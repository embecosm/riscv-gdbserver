// Generic GDB RSP server interface: definition.

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
// stream operators to allow its public scoped enumerations to be output.

#include "ITarget.h"


//! Output operator for ResumeType enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The ResumeType value to output.
//! @return  The stream with the item appended.

std::ostream &
operator<< (std::ostream & s,
	    ITarget::ResumeType  p)
{
  const char * name;

  switch (p)
    {
    case ITarget::ResumeType::STEP:     name = "step";     break;
    case ITarget::ResumeType::CONTINUE: name = "continue"; break;
    case ITarget::ResumeType::STOP:     name = "stop";     break;
    default:                            name = "unknown";  break;
    }

  return  s << name;

}	// operator<< ()


//! Output operator for ResumeRes enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The ResumeRes value to output.
//! @return  The stream with the item appended.

std::ostream &
operator<< (std::ostream & s,
	    ITarget::ResumeRes  p)
{
  const char * name;

  switch (p)
    {
    case ITarget::ResumeRes::NONE:        name = "none";        break;
    case ITarget::ResumeRes::SUCCESS:     name = "success";     break;
    case ITarget::ResumeRes::FAILURE:     name = "failure";     break;
    case ITarget::ResumeRes::INTERRUPTED: name = "interrupted"; break;
    case ITarget::ResumeRes::TIMEOUT:     name = "timeout";     break;
    case ITarget::ResumeRes::SYSCALL:     name = "syscall";     break;
    default:                              name = "unknown";     break;
    }

  return  s << name;

}	// operator<< ()


//! Output operator for MatchType enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The MatchType value to output.
//! @return  The stream with the item appended.

std::ostream &
operator<< (std::ostream & s,
	    ITarget::MatchType  p)
{
  const char * name;

  switch (p)
    {
    case ITarget::MatchType::BREAK:        name = "breakpoint";          break;
    case ITarget::MatchType::BREAK_HW:     name = "hardware breakpoint"; break;
    case ITarget::MatchType::WATCH_WRITE:  name = "write watchpoint";    break;
    case ITarget::MatchType::WATCH_READ:   name = "read watchpoint";     break;
    case ITarget::MatchType::WATCH_ACCESS: name = "access watchpoint";   break;
    default:                               name = "unknown";             break;
    }

  return  s << name;

}	// operator<< ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
