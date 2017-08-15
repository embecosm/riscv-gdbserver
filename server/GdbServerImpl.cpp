// GDB RSP server implementation: definition

// Copyright (C) 2009, 2013, 2017  Embecosm Limited <info@embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
// Contributor Ian Bolton <ian.bolton@embecosm.com>

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
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#include "GdbServerImpl.h"
#include "Utils.h"

using std::chrono::duration;
using std::chrono::system_clock;
using std::chrono::time_point;
using std::cout;
using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::localtime;
using std::ostringstream;
using std::setfill;
using std::setw;
using std::string;
using std::stringstream;
using std::vector;


//! Constructor for the GDB RSP server.

//! Allocate a packet data structure and a new RSP connection. By default no
//! timeout for run/continue. Set up a disassembler in case we need it.

//! @param[in] rspPort      RSP port to use.
//! @param[in] _cpu         The simulated CPU
//! @param[in] _traceFlags  Flags controlling tracing

GdbServerImpl::GdbServerImpl (AbstractConnection * _conn,
			      ITarget * _cpu,
			      TraceFlags * _traceFlags,
			      GdbServer::KillBehaviour _killBehaviour) :
  cpu (_cpu),
  traceFlags (_traceFlags),
  rsp (_conn),
  timeout (duration <double>::zero ()),
  killBehaviour (_killBehaviour)
{
  pkt           = new RspPacket (RSP_PKT_SIZE);
  mpHash        = new MpHash ();
  mDisassembler = new Disassembler ();

}	// GdbServerImpl ()


//! Destructor

GdbServerImpl::~GdbServerImpl ()
{
  delete  mDisassembler;
  delete  mpHash;
  delete  pkt;

}	// ~GdbServerImpl


//! Main loop to listen for RSP requests

//! This only terminates if there was an error.

void
GdbServerImpl::rspServer ()
{
  // Loop processing commands forever
  while (true)
    {
      // Make sure we are still connected.
      while (!rsp->isConnected ())
	{
	  // Reconnect and stall the processor on a new connection
	  if (!rsp->rspConnect ())
	    {
	      // Serious failure. Must abort execution.
	      cerr << "*** Unable to continue: ABORTING" << endl;
	      return;
	    }
	}

      // Get a RSP client request
      rspClientRequest ();
    }
}	// rspServer ()


//! Callback for targets to use.

//! Nothing implemented for now.

//! @param[in] cmd  Request as textual command
//! @param[out] stream  Response as text on a stream
//! @return  TRUE if the command was accepted, FALSE otherwise

bool
GdbServerImpl::command (const std::string  cmd,
			std::ostream & stream)
{
  vector<string> tokens;
  Utils::split (cmd.c_str (), " ", tokens);
  int numTok = tokens.size ();

  // Look for any commands we can handle

  if ((numTok == 2) && (string ("disas") == tokens[0]))
    {
      // disas <addr>

      uint32_t insn = strtol (tokens[1].c_str (), nullptr, 0);
      mDisassembler->disassemble (insn, stream);
      return true;
    }
  else
    return  false;

}	// GdbServerImpl::rspServer ()


//! Some F request packets want to know the length of the string
//! argument, so we have this simple function here to calculate that.

int
GdbServerImpl::stringLength (uint32_t addr)
{
  uint8_t ch;
  int count = 0;
  while (1 == cpu->read (addr + count, &ch, 1))
  {
    count++;
    if (ch == 0)
      break;
  }
  return count;
}


//! We achieve a syscall on the host by sending an F request packet to
//! the GDB client. The arguments for the call will have already been
//! put into registers via its newlib/libgloss implementation.

void
GdbServerImpl::rspSyscallRequest ()
{
  // Keep track of whether we were in the middle of a Continue or Step
  lastPacketType = pkt->data[0];

  // Get the args from the appropriate regs and send an F packet
  uint32_t a0, a1, a2, a3, a7;
  cpu->readRegister (10, a0);
  cpu->readRegister (11, a1);
  cpu->readRegister (12, a2);
  cpu->readRegister (13, a3);
  cpu->readRegister (17, a7);

  // Work out which syscall we've got
  switch (a7) {
    case 57   : sprintf (pkt->data, "Fclose,%x", a0);
                break;
    case 62   : sprintf (pkt->data, "Flseek,%x,%x,%x", a0, a1, a2);
                break;
    case 63   : sprintf (pkt->data, "Fread,%x,%x,%x", a0, a1, a2);
                break;
    case 64   : sprintf (pkt->data, "Fwrite,%x,%x,%x", a0, a1, a2);
                break;
    case 80   : sprintf (pkt->data, "Ffstat,%x,%x", a0, a1);
                break;
    case 169  : sprintf (pkt->data, "Fgettimeofday,%x,%x", a0, a1);
                break;
    case 1024 : sprintf (pkt->data, "Fopen,%x/%x,%x,%x", a0, stringLength (a0), a1, a2);
                break;
    case 1026 : sprintf (pkt->data, "Funlink,%x/%x", a0, stringLength (a0));
                break;
    case 1038 : sprintf (pkt->data, "Fstat,%x/%x,%x", a0, stringLength (a0), a1);
                break;
    default   : rspReportException (TargetSignal::TRAP);
                return;
  }

  // Send the packet
  pkt->setLen (strlen (pkt->data));
  rsp->putPkt (pkt);
}


//! The F reply is sent by the GDB client to us after a syscall has been
//! handled.

void
GdbServerImpl::rspSyscallReply ()
{
  uint32_t  retvalue;

  // Get the return value from the F reply
  if (1 != sscanf (pkt->data, "F%x", &retvalue))
  {
    cerr << "Freply received unexpected amount of variables" << endl;
  }

  // @todo: fstat currently returns -1 after resetting and re-loading within a
  //       single GDB session which causes GCC regression tests to fail, so we
  //       sidestep it here with a HACK.

  if (retvalue != -1)
    cpu->writeRegister (10, retvalue);
}


//! Deal with a request from the GDB client session

//! In general, apart from the simplest requests, this function replies on
//! other functions to implement the functionality.

//! @note It is the responsibility of the recipient to delete the packet when
//!       it is finished with. It is permissible to reuse the packet for a
//!       reply.

//! @param[in] pkt  The received RSP packet

void
GdbServerImpl::rspClientRequest ()
{
  if (!rsp->getPkt (pkt))
    {
      rsp->rspClose ();			// Comms failure
      return;
    }

  clock_t timeout_start;

  switch (pkt->data[0])
    {
    case '!':
      // Request for extended remote mode
      pkt->packStr ("OK");
      rsp->putPkt (pkt);
      return;

    case '?':
      // Return last signal ID
      rspReportException ();
      return;

    case 'A':
      // Initialization of argv not supported
      cerr << "Warning: RSP 'A' packet not supported: ignored" << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;

    case 'b':
      // Setting baud rate is deprecated
      cerr << "Warning: RSP 'b' packet is deprecated and not "
	   << "supported: ignored" << endl;
      return;

    case 'B':
      // Breakpoints should be set using Z packets
      cerr << "Warning: RSP 'B' packet is deprecated (use 'Z'/'z' "
	   << "packets instead): ignored" << endl;
      return;

    case 'F':
      // Handle the syscall reply then continue
      rspSyscallReply ();
      // intentionally carry on here rather than break

    case 'c':
    case 'C':
      // Continue.  We have two timeouts to worry about.  The first is any
      // timeout set by the user (through "monitor timeout", the second is a
      // timeout for checking for crtl-C.

      // @todo For now we use indentical code for 'C' (continue with signal)
      //       and just ignore the signal.
      // @todo We do not yet use the SyscallInfo arcgument when resuming.

      {
	// @todo We ought to have a constant for this.

	duration <double>  interruptTimeout (0.1);
	time_point <system_clock, duration <double> >  timeout_end =
	  system_clock::now () + timeout;

	// Check for break before resuming the machine.

	if (rsp->haveBreak ())
	  {
	    (void) cpu->resume (ITarget::ResumeType::STOP);
	    rspReportException (TargetSignal::INT);
	    return;
	  }

	for (;;)
	  {
	    ITarget::ResumeRes resType =
	      cpu->resume (ITarget::ResumeType::CONTINUE,
			   interruptTimeout, nullptr);

	    switch (resType)
	      {
	      case ITarget::ResumeRes::SYSCALL:

		// We have changed all support syscalls to have a
		// nop,ebreak,nop which was caught in Ri5cyImpl.cc and then
		// SYSCALL was returned (to get us to this point)
 		rspSyscallRequest ();
		return;

	      case ITarget::ResumeRes::INTERRUPTED:

		// At breakpoint

		rspReportException (TargetSignal::TRAP);
		return;

	      case ITarget::ResumeRes::TIMEOUT:

		// Check for timeout, unless the timeout was zero
		if ((duration <double>::zero () != timeout)
		    && (timeout_end < system_clock::now ()))
		  {
		    // Force the target to stop. Ignore return value.

		    (void) cpu->resume (ITarget::ResumeType::STOP);
		    rspReportException (TargetSignal::XCPU);	// Timeout
		    return;
		  }

		// Check for break
		if (rsp->haveBreak ())
		  {
		    // Force the target to stop. Ignore return value.

		    (void) cpu->resume (ITarget::ResumeType::STOP);
		    rspReportException (TargetSignal::INT);	// Interrupt
		    return;
		  }

		break;

	      default:

		// Should never occur.  We exit the gdbserver if this happens.

		cerr << "*** ABORT: Unrecognized continue return from resume: "
		     << "terminating" << resType << endl;
		exit (EXIT_FAILURE);
	      }
	  }
      }

    case 'd':
      // Disable debug using a general query
      cerr << "Warning: RSP 'd' packet is deprecated (define a 'Q' "
	   << "packet instead: ignored" << endl;
      return;

    case 'D':
      // Detach GDB. Do this by closing the client. The rules say that
      // execution should continue, so unstall the processor.
      pkt->packStr("OK");
      rsp->putPkt (pkt);
      rsp->rspClose ();
      return;

    case 'g':
      rspReadAllRegs ();
      return;

    case 'G':
      rspWriteAllRegs ();
      return;

    case 'H':
      // Set the thread number of subsequent operations. For now ignore
      // silently and just reply "OK"
      pkt->packStr ("OK");
      rsp->putPkt (pkt);
      return;

    case 'i':
      // Single cycle step. TODO. For now we immediately report we have hit an
      // exception.
      rspReportException ();
      return;

    case 'I':
      // Single cycle step with signal. TODO. For now we immediately report we
      // have hit an exception.
      rspReportException ();
      return;

    case 'k':
      // Kill request.
      switch (killBehaviour)
	{
	case GdbServer::KillBehaviour::EXIT_ON_KILL:
	  // Like the 'monitor exit' command this is a bit grotty.  Would
	  // be better if we could return from gdbserver and have main
	  // delete everything and exit cleanly that way.
	  exit (EXIT_SUCCESS);
	  break;

	case GdbServer::KillBehaviour::RESET_ON_KILL:
	  // Shhh! We don't actually reset right now.  Just keep going.
	  break;
	}
      return;

    case 'm':
      // Read memory (symbolic)
      rspReadMem ();
      return;

    case 'M':
      // Write memory (symbolic)
      rspWriteMem ();
      return;

    case 'p':
      // Read a register
      rspReadReg ();
      return;

    case 'P':
      // Write a register
      rspWriteReg ();
      return;

    case 'q':
      // Any one of a number of query packets
      rspQuery ();
      return;

    case 'Q':
      // Any one of a number of set packets
      rspSet ();
      return;

    case 'r':
      // Reset the system. Deprecated (use 'R' instead)
      cerr << "Warning: RSP 'r' packet is deprecated (use 'R' "
 		<< "packet instead): ignored" << endl;
      return;

    case 'R':
      // Restart the program being debugged. TODO. Nothing for now.
      return;

    case 's':
    case 'S':
      {
	// Single step one machine instruction.
	// @todo For 'S' we currently only handle Syscall requests

	// Check for break before resuming the machine.

	if (rsp->haveBreak ())
	  {
	    (void) cpu->resume (ITarget::ResumeType::STOP, nullptr);
	    rspReportException (TargetSignal::INT);
	    return;
	  }

	// @todo No syscall for now.

	ITarget::ResumeRes resType = cpu->resume (ITarget::ResumeType::STEP,
						  nullptr);

	if (resType == ITarget::ResumeRes::SYSCALL)
	  {
	    // @todo Waiting for syscall. Should not occur, if it does, we
	    // treat it as a trap.

	    cerr << "Warning: Unexpected SYSCALL return in 's' packet: "
		 << "treating as TRAP." << endl;

	    rspReportException (TargetSignal::INT);
	    return;
	  }

	// Check for break now we've stopped. No Syscall for now

	if (rsp->haveBreak ())
	  {
	    (void) cpu->resume (ITarget::ResumeType::STOP, nullptr);
	    rspReportException (TargetSignal::INT);
	    return;
	  }

	rspReportException (TargetSignal::TRAP);
	return;
      }

    case 't':
      // Search. This is not well defined in the manual and for now we don't
      // support it. No response is defined.
      cerr << "Warning: RSP 't' packet not supported: ignored" << endl;
      return;

    case 'T':
      // Is the thread alive. We are bare metal, so don't have a thread
      // context. The answer is always "OK".
      pkt->packStr ("OK");
      rsp->putPkt (pkt);
      return;

    case 'v':
      // Any one of a number of packets to control execution
      rspVpkt ();
      return;

    case 'X':
      // Write memory (binary)
      rspWriteMemBin ();
      return;

    case 'z':
      // Remove a breakpoint/watchpoint.
      rspRemoveMatchpoint ();
      return;

    case 'Z':
      // Insert a breakpoint/watchpoint.
      rspInsertMatchpoint ();
      return;

    default:
      // Unknown commands are ignored
      cerr << "Warning: Unknown RSP request" << pkt->data << endl;
      return;
    }
}	// rspClientRequest ()


//! Send a packet acknowledging an exception has occurred

//! @param[in] sig  The signal to send (defaults to TargetSignal::TRAP).

void
GdbServerImpl::rspReportException (TargetSignal  sig)
{
  // Construct a signal received packet
  pkt->data[0] = 'S';
  pkt->data[1] = Utils::hex2Char (static_cast<int> (sig) >> 4);
  pkt->data[2] = Utils::hex2Char (static_cast<int> (sig) % 16);
  pkt->data[3] = '\0';
  pkt->setLen (strlen (pkt->data));

  rsp->putPkt (pkt);

}	// rspReportException ()


//! Handle a RSP read all registers request

//! This means getting the value of each simulated register and packing it
//! into the packet.

//! Each byte is packed as a pair of hex digits.

void
GdbServerImpl::rspReadAllRegs ()
{
  int  pktSize = 0;

  // The registers. GDB client expects them to be packed according to target
  // endianness.
  for (int  regNum = 0; regNum < RISCV_NUM_REGS; regNum++)
    {
      uint32_t  val;		// Enough for even the PC
      int       byteSize;	// Size of reg in bytes

      byteSize = cpu->readRegister (regNum, val);
      Utils::val2Hex (val, &(pkt->data[pktSize]), byteSize,
		      true /* Little Endian */);
      pktSize += byteSize * 2;	// 2 chars per hex digit
    }

  // Finalize the packet and send it
  pkt->data[pktSize] = 0;
  pkt->setLen (pktSize);
  rsp->putPkt (pkt);

}	// rspReadAllRegs ()


//! Handle a RSP write all registers request

//! Each value is written into the simulated register.

void
GdbServerImpl::rspWriteAllRegs ()
{
  int  pktSize = 0;

  // The registers
  for (int  regNum = 0; regNum < RISCV_NUM_REGS; regNum++)
    {
      int       byteSize = 4;	// @todo automate this. Size of reg in bytes

      uint32_t val = Utils::hex2Val (&(pkt->data[pktSize]), byteSize,
				     true /* little endian */);
      pktSize += byteSize * 2;	// 2 chars per hex digit

      if (byteSize != cpu->writeRegister (regNum, val))
	cerr << "Warning: Size != " << byteSize << " when writing reg "
	     << regNum << "." << endl;
    }

  pkt->packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteAllRegs ()


//! Handle a RSP read memory (symbolic) request

//! Syntax is:
//!   m<addr>,<length>:

//! The response is the bytes, lowest address first, encoded as pairs of hex
//! digits.

//! The length given is the number of bytes to be read.

void
GdbServerImpl::rspReadMem ()
{
  uint32_t  addr;			// Where to read the memory
  int       len;			// Number of bytes to read
  int       off;			// Offset into the memory

  if (2 != sscanf (pkt->data, "m%x,%x:", &addr, &len))
    {
      cerr << "Warning: Failed to recognize RSP read memory command: "
		<< pkt->data << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Make sure we won't overflow the buffer (2 chars per byte)
  if ((len * 2) >= pkt->getBufSize())
    {
      cerr << "Warning: Memory read " << pkt->data
	   << " too large for RSP packet: truncated" << endl;
      len = (pkt->getBufSize() - 1) / 2;
    }

  // Refill the buffer with the reply
  for (off = 0; off < len; off++)
    {
      uint8_t  ch;
      if (1 == cpu->read (addr + off, &ch, 1))
	{
	  pkt->data[off * 2]     = Utils::hex2Char(ch >>   4);
	  pkt->data[off * 2 + 1] = Utils::hex2Char(ch &  0xf);
	}
      else
	cerr << "Warning: failed to read char" << endl;
    }

  pkt->data[off * 2] = '\0';			// End of string
  pkt->setLen (strlen (pkt->data));
  rsp->putPkt (pkt);

}	// rsp_read_mem ()


//! Handle a RSP write memory (symbolic) request

//! Syntax is:

//!   m<addr>,<length>:<data>

//! The data is the bytes, lowest address first, encoded as pairs of hex
//! digits.

//! The length given is the number of bytes to be written.

void
GdbServerImpl::rspWriteMem ()
{
  uint32_t  addr;			// Where to write the memory
  int       len;			// Number of bytes to write

  if (2 != sscanf (pkt->data, "M%x,%x:", &addr, &len))
    {
      cerr << "Warning: Failed to recognize RSP write memory "
		<< pkt->data << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Find the start of the data and check there is the amount we expect.
  char *symDat = (char *)(memchr (pkt->data, ':', pkt->getBufSize())) + 1;
  int   datLen = pkt->getLen() - (symDat - pkt->data);

  // Sanity check
  if (len * 2 != datLen)
    {
      cerr << "Warning: Write of " << len * 2 << "digits requested, but "
		<< datLen << " digits supplied: packet ignored" << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Write the bytes to memory (no check the address is OK here)
  for (int  off = 0; off < len; off++)
    {
      uint8_t  nyb1 = Utils::char2Hex (symDat[off * 2]);
      uint8_t  nyb2 = Utils::char2Hex (symDat[off * 2 + 1]);
      uint8_t  val = static_cast<unsigned int> ((nyb1 << 4) | nyb2);

      if (1 != cpu->write (addr + off, &val, 1))
	cerr << "Warning: Failed to write character" << endl;
    }

  pkt->packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteMem ()


//! Read a single register

//! The registers follow the GDB sequence: 32 general registers, SREG, SP and
//! PC.

//! Each byte is packed as a pair of hex digits.

void
GdbServerImpl::rspReadReg ()
{
  unsigned int  regNum;

  // Break out the fields from the data
  if (1 != sscanf (pkt->data, "p%x", &regNum))
    {
      cerr << "Warning: Failed to recognize RSP read register command: "
		<< pkt->data << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Get the relevant register. GDB client expects them to be packed according
  // to target endianness.
  uint32_t  val;
  int       byteSize;

  byteSize = cpu->readRegister (regNum, val);
  Utils::val2Hex (val, pkt->data, byteSize, true /* little endian */);

  pkt->setLen (strlen (pkt->data));
  rsp->putPkt (pkt);

}	// rspReadReg ()


//! Write a single register

//! The registers follow the GDB sequence for OR1K: GPR0 through GPR31, PC
//! (i.e. SPR NPC) and SR (i.e. SPR SR). The register is specified as a
//! sequence of bytes in target endian order.

//! Each byte is packed as a pair of hex digits.

void
GdbServerImpl::rspWriteReg ()
{
  unsigned int  regNum;
  char          valstr[2 * sizeof (uint64_t) + 1];	// Allow for EOS

  // Break out the fields from the data
  if (2 != sscanf (pkt->data, "P%x=%s", &regNum, valstr))
    {
      cerr << "Warning: Failed to recognize RSP write register command "
	   << pkt->data << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  int      byteSize = 4;	// @todo automate this. Size of reg in bytes
  uint32_t val = Utils::hex2Val (valstr, byteSize, true /* little endian */);

  if (byteSize != cpu->writeRegister (regNum, val))
    cerr << "Warning: Size != " << byteSize << " when writing reg " << regNum
	 << "." << endl;

  pkt->packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteReg ()


//! Handle a RSP query request

//! We deal with those we have an explicit response for and send a null
//! response to anything else, to indicate it is not supported. This makes us
//! flexible to future GDB releases with as yet undefined packets.

void
GdbServerImpl::rspQuery ()
{
  if (0 == strcmp ("qC", pkt->data))
    {
      // Return the current thread ID (unsigned hex). A null response
      // indicates to use the previously selected thread. We use the constant
      // DUMMY_TID to represent our single thread of control.
      sprintf (pkt->data, "QC%x", DUMMY_TID);
      pkt->setLen (strlen (pkt->data));
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("qCRC", pkt->data, strlen ("qCRC")))
    {
      // Return CRC of memory area
      cerr << "Warning: RSP CRC query not supported" << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp ("qfThreadInfo", pkt->data))
    {
      // Return info about active threads. We return just the constant
      // DUMMY_TID to represent our single thread of control.
      sprintf (pkt->data, "m%x", DUMMY_TID);
      pkt->setLen (strlen (pkt->data));
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp ("qsThreadInfo", pkt->data))
    {
      // Return info about more active threads. We have no more, so return the
      // end of list marker, 'l'
      pkt->packStr ("l");
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("qL", pkt->data, strlen ("qL")))
    {
      // Deprecated and replaced by 'qfThreadInfo'
      cerr << "Warning: RSP qL deprecated: no info returned" << endl;
      pkt->packStr ("qM001");
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("qRcmd,", pkt->data, strlen ("qRcmd,")))
    {
      // This is used to interface to commands to do "stuff"
      rspCommand ();
    }
  else if (0 == strncmp ("qSupported", pkt->data, strlen ("qSupported")))
    {
      // Report a list of the features we support. For now we just ignore any
      // supplied specific feature queries, but in the future these may be
      // supported as well. Note that the packet size allows for 'G' + all the
      // registers sent to us, or a reply to 'g' with all the registers and an
      // EOS so the buffer is a well formed string.
      sprintf (pkt->data, "PacketSize=%x", pkt->getBufSize());
      pkt->setLen (strlen (pkt->data));
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("qSymbol:", pkt->data, strlen ("qSymbol:")))
    {
      // Offer to look up symbols. Nothing we want (for now). TODO. This just
      // ignores any replies to symbols we looked up, but we didn't want to
      // do that anyway!
      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("qThreadExtraInfo,", pkt->data,
			 strlen ("qThreadExtraInfo,")))
    {
      // Report that we are runnable, but the text must be hex ASCI
      // digits. For now do this by steam, reusing the original packet
      sprintf (pkt->data, "%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	       'R', 'u', 'n', 'n', 'a', 'b', 'l', 'e', 0);
      pkt->setLen (strlen (pkt->data));
      rsp->putPkt (pkt);
    }
  else
    {
      // We don't support this feature
      pkt->packStr ("");
      rsp->putPkt (pkt);
    }
}	// rspQuery ()


//! Handle a RSP qRcmd request

//! The actual command follows the "qRcmd," in ASCII encoded to hex

void
GdbServerImpl::rspCommand ()
{
  char *cmd = new char[pkt->getBufSize ()];
  int   timeout;

  Utils::hex2Ascii (cmd, &(pkt->data[strlen ("qRcmd,")]));

  if (traceFlags->traceRsp())
    {
      cout << "RSP trace: qRcmd," << cmd << endl;
    }

  if (0 == strncmp ("help", cmd, strlen (cmd)))
    {
      static const char *mess [] = {
	"The following generic monitor commands are supported:\n",
	"  help\n",
	"    Produce this message\n",
	"  reset [cold | warm]\n",
	"    Reset the simulator (default warm)\n",
	"  exit\n",
	"    Exit the GDB server\n",
	"  timeout <interval>\n",
	"    Maximum time in seconds taken by continue packet\n",
	"  cyclecount\n",
	"    Report cycles executed since last report and since reset\n",
	"  instrcount\n",
	"    Report instructions executed since last report and since reset\n",
	"  set debug <level>\n",
	"    Set debug messaging in target to <level>\n",
	"  show debug\n",
	"    Show current level of debug messaging in target\n",
	"  set remote-debug <0|1>\n",
	"    Disable/enable tracing of Remote Serial Protocol (RSP)\n",
	"  show remote-debug\n",
	"    Show whether RSP tracing is enabled\n",
	"  echo <message>\n",
	"    Echo <message> on stdout of the gdbserver\n",
	nullptr };

      for (int i = 0; nullptr != mess[i]; i++)
	{
	  pkt->packRcmdStr (mess[i], true);
	  rsp->putPkt (pkt);
	}

      // Now get any help from the target

      stringstream  ss;
      if (cpu->command (string ("help"), ss))
	{
	  string line;

	  pkt->packRcmdStr ("The following target specific monitor commands are supported:\n",
			     true);
	  rsp->putPkt (pkt);
	  while (getline (ss, line, '\n'))
	    {
	      line.append ("\n");
	      pkt->packRcmdStr (line.c_str (), true);
	      rsp->putPkt (pkt);
	    }
	}
      else
	{
	  // No target specific help

	  pkt->packRcmdStr ("There are no target specific monitor commands",
			     true);
	  rsp->putPkt (pkt);
	}

      // Not silent, so acknowledge OK

      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if ((0 == strcmp (cmd, "reset")) || (0 == strcmp (cmd, "reset warm")))
    {
      // Warm reset the CPU.  Failure to reset causes us to blow up.

      if (ITarget::ResumeRes::SUCCESS != cpu->reset (ITarget::ResetType::WARM))
	{
	  cerr << "*** ABORT *** Failed to reset: Terminating." << endl;
	  exit (EXIT_FAILURE);
	}

      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "reset cold"))
    {
      // Cold reset the CPU.  Failure to reset causes us to blow up.

      if (ITarget::ResumeRes::SUCCESS != cpu->reset (ITarget::ResetType::COLD))
	{
	  cerr << "*** ABORT *** Failed to cold reset: Terminating." << endl;
	  exit (EXIT_FAILURE);
	}

      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "exit"))
    {
      // This is a bit of a kludge. It would be much better to be deleted
      // cleanly from the top.

      delete cpu;
      exit (EXIT_SUCCESS);
    }
  else if (1 == sscanf (cmd, "timeout %d", &timeout))
    {
      timeout = timeout * CLOCKS_PER_SEC;

      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "timestamp"))
    {
      // @todo Do this using std::put_time, which is not in pre 5.0 GCC. Not
      // thread safe.

      std::ostringstream  oss;
      time_t now_c = system_clock::to_time_t (system_clock::now ());
      struct tm * timeinfo = localtime (&(now_c));
      char buff[20];

      strftime (buff, 20, "%F %T", timeinfo);
      oss << buff << endl;
      pkt->packHexstr (oss.str ().c_str ());
      rsp->putPkt (pkt);

      // Not silent, so acknowledge OK

      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "cyclecount"))
    {
      std::ostringstream  oss;
      oss << cpu->getCycleCount () << endl;
      pkt->packHexstr (oss.str ().c_str ());
      rsp->putPkt (pkt);

      // Not silent, so acknowledge OK

      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if (0 == strcmp (cmd, "instrcount"))
    {
      std::ostringstream  oss;
      oss << cpu->getInstrCount () << endl;
      pkt->packHexstr (oss.str ().c_str ());
      rsp->putPkt (pkt);

      // Not silent, so acknowledge OK

      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
    else if (0 == strncmp (cmd, "echo", 4))
      {
	const char *tmp = cmd + 4;
	while (*tmp != '\0' && isspace (*tmp))
	  ++tmp;
	cerr << std::flush;
	cout << tmp << std::endl << std::flush;
	pkt->packStr ("OK");
	rsp->putPkt (pkt);
      }
    // Insert any new generic commands here.
    // Don't forget to document them.

    else if (0 == strncmp (cmd, "set ", strlen ("set ")))
      {
	int i;

	for (i =  strlen ("set ") ; isspace (cmd[i]) ; i++)
	  ;

	rspSetCommand (cmd + i);
      }
    else if (0 == strncmp (cmd, "show ", strlen ("show ")))
      {
	int i;

	for (i =  strlen ("show ") ; isspace (cmd[i]) ; i++)
	  ;

	rspShowCommand (cmd + i);
      }
    else
      {
	// Fallback is to pass the command to the target.

	ostringstream  oss;

	if (cpu->command (string (cmd), oss))
	  {
	    pkt->packRcmdStr (oss.str ().c_str (), true);
	    rsp->putPkt (pkt);

	    // Not silent, so acknowledge OK

	    pkt->packStr ("OK");
	    rsp->putPkt (pkt);
	  }
	else
	  {
	    // Command failed

	    pkt->packStr ("E01");
	    rsp->putPkt (pkt);
	  }
      }

  delete [] cmd;

}	// rspCommand ()


//! Handle a RSP qRcmd request for set

//! The main rspCommand function has decoded the argument string and
//! stripped off "set" and any spaces separating it from the rest.

//! Any unrecognized command is passed to the target to process.

//! @param[in] cmd  The RSP set command string (excluding "set ")

void
GdbServerImpl::rspSetCommand (const char* cmd)
{
  vector<string> tokens;
  Utils::split (cmd, " ", tokens);
  int numTok = tokens.size ();

  // Look for any options we can handle.

  if ((numTok == 3) && (string ("debug") == tokens[0]))
    {
      // monitor set debug <flag> [1|0|on|off|true|false]

      const char * flagName = tokens[1].c_str ();

      // Valid flag?

      if (!traceFlags->isFlag (flagName))
 	{
	  // Not a valid flag

 	  pkt->packStr ("E01");
 	  rsp->putPkt (pkt);
 	  return;
 	}

      // Valid value?

      bool flagVal;

      if ((0 == strcasecmp (tokens[2].c_str (), "0"))
	  || (0 == strcasecmp (tokens[2].c_str (), "off"))
	  || (0 == strcasecmp (tokens[2].c_str (), "false")))
	flagVal = false;
      else if ((0 == strcasecmp (tokens[2].c_str (), "1"))
	  || (0 == strcasecmp (tokens[2].c_str (), "on"))
	  || (0 == strcasecmp (tokens[2].c_str (), "true")))
	flagVal = true;
      else
	{
	  // Not a valid level

 	  pkt->packStr ("E02");
 	  rsp->putPkt (pkt);
 	  return;
 	}

      traceFlags->flag (flagName, flagVal);
      pkt->packStr ("OK");
      rsp->putPkt (pkt);
      return;
    }
  else
    {
      // Not handled here, try the target

      ostringstream  oss;
      string fullCmd = string ("set ") + string (cmd);

      if (cpu->command (string (fullCmd), oss))
	{
	  pkt->packRcmdStr (oss.str ().c_str (), true);
	  rsp->putPkt (pkt);

	  // Not silent, so acknowledge OK

	  pkt->packStr ("OK");
	  rsp->putPkt (pkt);
	}
      else
	{
	  // Command failed

	  pkt->packStr ("E04");
	  rsp->putPkt (pkt);
	}
    }
}	// RspSetCommand ()


//! Handle a RSP qRcmd request for show

//! The main rspCommand function has decoded the argument string and
//! stripped off "show" and any spaces separating it from the rest.

//! Any unrecognized command is passed to the target to process.

//! @param[in] cmd  The RSP command string (excluding "show ")

void
GdbServerImpl::rspShowCommand (const char* cmd)
{
  vector<string> tokens;
  Utils::split (cmd, " ", tokens);
  int numTok = tokens.size ();

  if ((numTok == 1) && (string ("debug") == tokens[0]))
    {
      // monitor show debug

      ostringstream  oss;

      for (auto it = traceFlags->begin (); it != traceFlags->end (); it++)
	oss << *it << ": " << (traceFlags->flag (*it) ? "ON" : "OFF") << endl;

      pkt->packRcmdStr (oss.str ().c_str (), true);
      rsp->putPkt (pkt);
      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
  else if ((numTok == 2) && (string ("debug") == tokens[0]))
    {
      // monitor show debug <flag>

      ostringstream  oss;
      const char * flagName = tokens[1].c_str ();

      // Valid flag?

      if (!traceFlags->isFlag (flagName))
	{
	  // Not a valid flag

	  pkt->packStr ("E01");
	  rsp->putPkt (pkt);
	  return;
	}

      oss << flagName << ": " << (traceFlags->flag (flagName) ? "ON" : "OFF")
	  << endl;

      pkt->packRcmdStr (oss.str ().c_str (), true);
      rsp->putPkt (pkt);
      pkt->packStr ("OK");
      rsp->putPkt (pkt);
    }
  else
    {
      // Not handled here, try the target

      ostringstream  oss;
      string fullCmd = string ("show ") + string (cmd);

      if (cpu->command (string (fullCmd), oss))
	{
	  pkt->packRcmdStr (oss.str ().c_str (), true);
	  rsp->putPkt (pkt);

	  // Not silent, so acknowledge OK

	  pkt->packStr ("OK");
	  rsp->putPkt (pkt);
	}
      else
	{
	  // Command failed

	  pkt->packStr ("E04");
	  rsp->putPkt (pkt);
	}
    }
}	// rspShowCommand ()


//! Handle a RSP set request.

//! There are none that we support, so we always return an empty packet.

void
GdbServerImpl::rspSet ()
{
  pkt->packStr ("");
  rsp->putPkt (pkt);

}	// rspSet ()


//! Handle a RSP 'v' packet

//! @todo for now we don't handle V packets.

void
GdbServerImpl::rspVpkt ()
{
  pkt->packStr ("");
  rsp->putPkt (pkt);

}	// rspVpkt ()


//! Handle a RSP write memory (binary) request

//! Syntax is:

//!   X<addr>,<length>:

//! Followed by the specified number of bytes as raw binary. Response should be
//! "OK" if all copied OK, E<nn> if error <nn> has occurred.

//! The length given is the number of bytes to be written. The data buffer has
//! already been unescaped, so will hold this number of bytes.

void
GdbServerImpl::rspWriteMemBin ()
{
  uint32_t  addr;			// Where to write the memory
  int       len;			// Number of bytes to write

  if (2 != sscanf (pkt->data, "X%x,%x:", &addr, &len))
    {
      cerr << "Warning: Failed to recognize RSP write memory command: %s"
	   << pkt->data << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Find the start of the data and "unescape" it.
  uint8_t *bindat = (uint8_t *)(memchr (pkt->data, ':',
					pkt->getBufSize ())) + 1;
  int   off       = (char *)bindat - pkt->data;
  int   newLen    = Utils::rspUnescape ((char *)bindat, pkt->getLen () - off);

  // Sanity check
  if (newLen != len)
    {
      int  minLen = len < newLen ? len : newLen;

      cerr << "Warning: Write of " << len << " bytes requested, but "
	   << newLen << " bytes supplied. " << minLen << " will be written"
	   << endl;
      len = minLen;
    }

  // Write the bytes to memory.
  if (len != cpu->write (addr, bindat, len))
    cerr << "Warning: Failed to write " << len << " bytes to 0x" << hex
	 << addr << dec << endl;

  pkt->packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteMemBin ()


//! Handle a RSP remove breakpoint or matchpoint request

//! This checks that the matchpoint was actually set earlier. For software
//! (memory) breakpoints, the breakpoint is cleared from memory.

//! @todo This doesn't work with icache/immu yet

void
GdbServerImpl::rspRemoveMatchpoint ()
{
  MpType    type;			// What sort of matchpoint
  uint32_t  addr;			// Address specified
  uint32_t  instr;			// Instruction value found
  int       len;			// Matchpoint length
  uint8_t  *instrVec;			// Instruction as byte vector

  pkt->packStr ("");
  rsp->putPkt (pkt);
  return;

  // Break out the instruction
  string ui32Fmt = SCNx32;
  string fmt = "z%1d,%" + ui32Fmt + ",%1d";
  if (3 != sscanf (pkt->data, fmt.c_str(), (int *)&type, &addr, &len))
    {
      cerr << "Warning: RSP matchpoint deletion request not "
	   << "recognized: ignored" << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Sanity check len
  if (len > sizeof (instr))
    {
      cerr << "Warning: RSP remove breakpoint instruction length " << len
	   << " exceeds maximum of " << sizeof (instr) << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Sort out the type of matchpoint
  switch (type)
    {
    case BP_MEMORY:
      // Software (memory) breakpoint
      if (mpHash->remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "RSP trace: software (memory) breakpoint removed from 0x"
		   << hex << addr << dec << endl;
	    }
	}
      else
	{
	  cerr << "Warning: failed to remove software (memory) breakpoint "
	          "from 0x" << hex << addr << dec << endl;
	  pkt->packStr ("E01");
	  rsp->putPkt (pkt);
	}

      if (traceFlags->traceBreak ())
	cerr << "Putting back the instruction (0x" << hex << setfill ('0')
	     << setw (4) << instr << ") at 0x" << setw(8) << addr
	     << setfill (' ')  << setw (0) << dec << endl;

      // Remove the breakpoint from memory. The endianness of the instruction
      // matches that of the memory.
      instrVec = reinterpret_cast<uint8_t *> (&instr);

      if (len != cpu->write (addr, instrVec, len))
	cerr << "Warning: Failed to write memory removing breakpoint" << endl;

      pkt->packStr ("OK");
      rsp->putPkt (pkt);
      return;

    case BP_HARDWARE:
      // Hardware breakpoint
      if (mpHash->remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "Rsp trace: hardware breakpoint removed from 0x NOT IMPLEMENTED"
		   << hex << addr << dec << endl;
	    }
      /*
	  cpu->removeBreak(addr);
	  */
	  pkt->packStr ("OK");
	  rsp->putPkt (pkt);
	}
      else
	{
	  cerr << "Warning: failed to remove hardware breakpoint from 0x"
	       << hex << addr << dec << endl;
	  pkt->packStr ("E01");
	  rsp->putPkt (pkt);
	}

      return;

    case WP_WRITE:
      // Write watchpoint
      if (mpHash->remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "RSP trace: write watchpoint removed from 0x"
		   << hex << addr << dec << endl;
	    }

	  pkt->packStr ("");		// TODO: Not yet implemented
	  rsp->putPkt (pkt);
	}
      else
	{
	  cerr << "Warning: failed to remove write watchpoint from 0x"
	       << hex << addr << dec << endl;
	  pkt->packStr ("E01");
	  rsp->putPkt (pkt);
	}

      return;

    case WP_READ:
      // Read watchpoint
      if (mpHash->remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "RSP trace: read watchpoint removed from 0x"
		   << hex << addr << dec << endl;
	    }

	  pkt->packStr ("");		// TODO: Not yet implemented
	  rsp->putPkt (pkt);
	}
      else
	{
	  cerr << "Warning: failed to remove read watchpoint from 0x"
	       << hex << addr << dec << endl;
	  pkt->packStr ("E01");
	  rsp->putPkt (pkt);
	}

      return;

    case WP_ACCESS:
      // Access (read/write) watchpoint
      if (mpHash->remove (type, addr, &instr))
	{
	  if (traceFlags->traceRsp())
	    {
	      cout << "RSP trace: access (read/write) watchpoint removed "
		      "from 0x" << hex << addr << dec << endl;
	    }

	  pkt->packStr ("");		// TODO: Not yet implemented
	  rsp->putPkt (pkt);
	}
      else
	{
	  cerr << "Warning: failed to remove access (read/write) watchpoint "
	          "from 0x" << hex << addr << dec << endl;
	  pkt->packStr ("E01");
	  rsp->putPkt (pkt);
	}

      return;

    default:
      cerr << "Warning: RSP matchpoint type " << type
	   << " not recognized: ignored" << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }
}	// rspRemoveMatchpoint ()


//! Handle a RSP insert breakpoint or matchpoint request

//! @todo For now only memory breakpoints are handled

void
GdbServerImpl::rspInsertMatchpoint ()
{
  MpType    type;			// What sort of matchpoint
  uint32_t  addr;			// Address specified
  uint32_t  instr;			// Instruction value found
  int       len;			// Matchpoint length
  uint8_t  *instrVec;			// Instruction as byte vector

  pkt->packStr ("");
  rsp->putPkt (pkt);
  return;

  // Break out the instruction
  string ui32Fmt = SCNx32;
  string fmt = "Z%1d,%" + ui32Fmt + ",%1d";
  if (3 != sscanf (pkt->data, fmt.c_str(), (int *)&type, &addr, &len))
    {
      cerr << "Warning: RSP matchpoint insertion request not "
	   << "recognized: ignored" << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Sanity check len
  if (len > sizeof (instr))
    {
      cerr << "Warning: RSP set breakpoint instruction length " << len
	   << " exceeds maximum of " << sizeof (instr) << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }

  // Sort out the type of matchpoint
  switch (type)
    {
    case BP_MEMORY:
      // Software (memory) breakpoint. Extract the instruction.
      instrVec = reinterpret_cast<uint8_t *> (&instr);

      if (len != cpu->read (addr, instrVec, len))
	cerr << "Warning: Failed to read memory when inserting breakpoint"
	     << endl;

      // Record the breakpoint and write a breakpoint instruction in its
      // place.
      mpHash->add (type, addr, instr);

      if (traceFlags->traceBreak ())
	cerr << "Inserting a breakpoint over the  instruction (0x" << hex
	     << setfill ('0') << setw (4) << instr << ") at 0x" << setw(8)
	     << addr << setfill (' ')  << setw (0) << dec << endl;

      // Little-endian, so least significant byte is at "little" address.

      instr = BREAK_INSTR;
      instrVec = reinterpret_cast<uint8_t *> (&instr);

      if (4 != cpu->write (addr, instrVec, 4))
	cerr << "Warning: Failed to write BREAK instruction" << endl;

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: software (memory) breakpoint inserted at 0x"
	       << hex << addr << dec << endl;
	}

      pkt->packStr ("OK");
      rsp->putPkt (pkt);
      return;

    case BP_HARDWARE:
      // Hardware breakpoint
      mpHash->add (type, addr, 0);	// No instr for HW matchpoints

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: hardware breakpoint set at 0x NOT IMPLEMENTED"
	       << hex << addr << dec << endl;
	}
      /*
      cpu->insertBreak(addr);
      */
      pkt->packStr ("OK");
      rsp->putPkt (pkt);

      return;

    case WP_WRITE:
      // Write watchpoint
      mpHash->add (type, addr, 0);	// No instr for HW matchpoints

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: write watchpoint set at 0x"
	       << hex << addr << dec << endl;
	}

      pkt->packStr ("");		// TODO: Not yet implemented
      rsp->putPkt (pkt);

      return;

    case WP_READ:
      // Read watchpoint
      mpHash->add (type, addr, 0);	// No instr for HW matchpoints

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: read watchpoint set at 0x"
	       << hex << addr << dec << endl;
	}

      pkt->packStr ("");		// TODO: Not yet implemented
      rsp->putPkt (pkt);

      return;

    case WP_ACCESS:
      // Access (read/write) watchpoint
      mpHash->add (type, addr, 0);	// No instr for HW matchpoints

      if (traceFlags->traceRsp())
	{
	  cout << "RSP trace: access (read/write) watchpoint set at 0x"
	       << hex << addr << dec << endl;
	}

      pkt->packStr ("");		// TODO: Not yet implemented
      rsp->putPkt (pkt);

      return;

    default:
      cerr << "Warning: RSP matchpoint type " << type
	   << "not recognized: ignored"<< endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }
}	// rspInsertMatchpoint ()


//! Output operator for TargetSignal enumeration

//! @param[in] s  The stream to output to.
//! @param[in] p  The TargetSignal value to output.
//! @return  The stream with the item appended.

std::ostream &
operator<< (std::ostream & s,
	    GdbServerImpl::TargetSignal  p)
{
  const char * name;

  switch (p)
    {
    case GdbServerImpl::TargetSignal::NONE:    name = "SIGNONE";    break;
    case GdbServerImpl::TargetSignal::INT:     name = "SIGINT";     break;
    case GdbServerImpl::TargetSignal::TRAP:    name = "SIGTRAP";    break;
    case GdbServerImpl::TargetSignal::XCPU:    name = "SIGXCPU";    break;
    case GdbServerImpl::TargetSignal::UNKNOWN: name = "SIGUNKNOWN"; break;
    default:                                   name = "unknown";    break;
    }

  return  s << name;

}	// operator<< ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
