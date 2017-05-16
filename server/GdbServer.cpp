// GDB RSP server: implementation

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
// ----------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <cstring>
#include <string>
#include <sstream>
#include <cstdlib>

#include "GdbServer.h"
#include "Cpu.h"
#include "Utils.h"

using std::cout;
using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::string;


//-----------------------------------------------------------------------------
// For now these are hard-coded constants, but they need to be made
// configurable.
//-----------------------------------------------------------------------------

//! Total number of regs. 32 general regs + PC
static const int RISCV_NUM_REGS = 33;
//! Total bytes taken by regs. 4 bytes for each
static const int RISCV_NUM_REG_BYTES = RISCV_NUM_REGS * 4;

// Special registers
static const int RISCV_PC_REGNUM   = 32; 

//! Minimum packet size for RSP. Must be large enough for any initial
//! dialogue. Should at least allow all the registers ASCII encloded + end of
//! string marker.
static const int RSP_PKT_SIZE =
  (RISCV_NUM_REG_BYTES * 2 + 1) < 256 ? 256 : RISCV_NUM_REG_BYTES * 2 + 1;


//! Constructor for the GDB RSP server.

//! Allocate a packet data structure and a new RSP connection. By default no
//! timeout for run/continue.

//! @param[in] rspPort      RSP port to use.
//! @param[in] _cpu         The simulated CPU
//! @param[in] _traceFlags  Flags controlling tracing
GdbServer::GdbServer (int         rspPort,
		      Cpu      *_cpu,
		      TraceFlags *_traceFlags) :
  cpu (_cpu),
  traceFlags (_traceFlags),
  clock_timeout (0)
{
  pkt       = new RspPacket (RSP_PKT_SIZE);
  rsp       = new RspConnection (rspPort, traceFlags);
  mpHash    = new MpHash ();

}	// GdbServer ()


//! Destructor
GdbServer::~GdbServer ()
{
  delete  mpHash;
  delete  rsp;
  delete  pkt;

}	// ~GdbServer


//! Main loop to listen for RSP requests

//! This only terminates if there was an error.
void
GdbServer::rspServer ()
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


//! Deal with a request from the GDB client session

//! In general, apart from the simplest requests, this function replies on
//! other functions to implement the functionality.

//! @note It is the responsibility of the recipient to delete the packet when
//!       it is finished with. It is permissible to reuse the packet for a
//!       reply.

//! @param[in] pkt  The received RSP packet
void
GdbServer::rspClientRequest ()
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

    case 'c':
      // Continue. For now without taking any notice of the address. TODO we
      // need to be clearer about the exception we report back.
      timeout_start = std::clock ();

#ifdef GDBSERVER_DEBUG
      fprintf (stderr, "Continuing...\n");
#endif

      for (int i = 0; true; i++)
	{
	  if (cpu->step ())
	    {
	      rspReportException ();
	      return;
	    }
	  if ((clock_timeout != 0)
	      && (0 == (i % RUN_SAMPLE_PERIOD))
	      && ((clock() - timeout_start) > clock_timeout))
	    {
	      rspReportException (TARGET_SIGNAL_XCPU);	// Timeout
	      return;
	    }
	}

    case 'C':
      // Continue with signal (in the packet). For now it is just the same as
      // 'c'. TODO for now we report we have hit an exception.
      rspReportException ();
      return;

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

    case 'F':
      // File I/O is not currently supported
      cerr << "Warning: RSP file I/O not currently supported: 'F' "
	   << "packet ignored" << endl;
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
      // Kill request. Do nothing for now.
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
      // Single step one machine instruction. For now we ignore the address.
      // TODO. We need to be clearer about the exception, if any, that we
      // hit.
#ifdef GDBSERVER_DEBUG
      fprintf (stderr, "Single stepping ...\n");
#endif
      cpu->step ();
      rspReportException ();
      return;

    case 'S':
      // Single step one machine instruction with signal. For now just the
      // same as 's'. TODO. Need to implement properly.
#ifdef GDBSERVER_DEBUG
      fprintf (stderr, "Single stepping with signal ...\n");
#endif
      cpu->step ();
      rspReportException ();
      return;

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

//! @param[in] sig  The signal to send (defaults to TARGET_SIGNAL_TRAP).
void
GdbServer::rspReportException (TargetSignal  sig)
{
  // Construct a signal received packet
  pkt->data[0] = 'S';
  pkt->data[1] = Utils::hex2Char (sig >> 4);
  pkt->data[2] = Utils::hex2Char (sig % 16);
  pkt->data[3] = '\0';
  pkt->setLen (strlen (pkt->data));

  rsp->putPkt (pkt);

}	// rspReportException ()


//! Handle a RSP read all registers request

//! This means getting the value of each simulated register and packing it
//! into the packet.

//! Each byte is packed as a pair of hex digits.
void
GdbServer::rspReadAllRegs ()
{
  int  pktSize = 0;

  // The registers. GDB client expects them to be packed according to target
  // endianness.
  for (int  regNum = 0; regNum < RISCV_NUM_REGS; regNum++)
    {
      uint32_t  val;		// Enough for even the PC
      int       byteSize;	// Size of reg in bytes

      switch (regNum)
	{
	case RISCV_PC_REGNUM:   val = cpu->readProgramAddr(); byteSize = 4; break;
	default:              val = cpu->readReg(regNum);  byteSize = 4; break;
	}

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
GdbServer::rspWriteAllRegs ()
{
  int  pktSize = 0;

  // The registers
  for (int  regNum = 0; regNum < RISCV_NUM_REGS; regNum++)
    {
      int       byteSize;	// Size of reg in bytes

      // Find the register size
      byteSize = 4;
	
      uint32_t val = Utils::hex2Val (&(pkt->data[pktSize]), byteSize,
				     true /* little endian */);
      pktSize += byteSize * 2;	// 2 chars per hex digit

      // Write the register
      switch (regNum)
	{
	case RISCV_PC_REGNUM:
	  cpu->writeProgramAddr(val);
	  break;
	default:              cpu->writeReg(regNum, val); break;
	}
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
GdbServer::rspReadMem ()
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
      ch= cpu->readMem (addr + off);
      pkt->data[off * 2]     = Utils::hex2Char(ch >>   4);
      pkt->data[off * 2 + 1] = Utils::hex2Char(ch &  0xf);
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
GdbServer::rspWriteMem ()
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

      cpu->writeMem (addr + off, (unsigned)((nyb1 << 4) | nyb2));
    }

  pkt->packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteMem ()


//! Read a single register

//! The registers follow the GDB sequence: 32 general registers, SREG, SP and
//! PC.

//! Each byte is packed as a pair of hex digits.
void
GdbServer::rspReadReg ()
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

  switch (regNum)
    {
    case RISCV_PC_REGNUM:   val = cpu->readProgramAddr(); byteSize = 4; break;
    default:              val = cpu->readReg(regNum);  byteSize = 4; break;
    }

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
GdbServer::rspWriteReg ()
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

  int       byteSize;	// Size of reg in bytes

  // Find the register size
  byteSize = 4;
  
  uint32_t val = Utils::hex2Val (valstr, byteSize, true /* little endian */);

  // Write the register
  switch (regNum)
    {
    case RISCV_PC_REGNUM:
      /* This will only work at the very start of the program before executing anything */
#ifdef GDBSERVER_DEBUG
      fprintf (stderr, "Writing PC to model as 0x%x\n", val);
#endif
      cpu->writeProgramAddr (val);
#ifdef GDBSERVER_DEBUG
      fprintf (stderr, "Reading PC back from model as 0x%x\n", cpu->readProgramAddr ());
#endif
      break;
    default:              cpu->writeReg(regNum, val); break;
    }

  pkt->packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteReg ()


//! Handle a RSP query request

//! We deal with those we have an explicit response for and send a null
//! response to anything else, to indicate it is not supported. This makes us
//! flexible to future GDB releases with as yet undefined packets.
void
GdbServer::rspQuery ()
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
GdbServer::rspCommand ()
{
  char *cmd = new char[pkt->getBufSize ()];
  int   timeout;

  Utils::hex2Ascii (cmd, &(pkt->data[strlen ("qRcmd,")]));

  if (traceFlags->traceRsp())
    {
      cout << "RSP trace: qRcmd," << cmd << endl;
    }

  if (0 == strcmp (cmd, "reset"))
    {
      // Also resets timer, which does not happen by default with cpu->reset ()
      cout << "Model reset NOT IMPLEMENTED" << endl;
      /*
      cpu->reset();
      cpu->setCycleCounter (0);
      cpu->setLifetimeCounter (0);
      */
    }
  else if (1 == sscanf (cmd, "timeout %d", &timeout))
    {
      clock_timeout = timeout * CLOCKS_PER_SEC;
    }
  else if (0 == strcmp (cmd, "timestamp"))
    {
      std::ostringstream  oss;
      cout << "Timestamp NOT IMPLEMENTED" << endl;
      /*
      oss << "Cycles since last call: " << cpu->getCycleCounter () << endl;
      pkt->packHexstr (oss.str ().c_str ());
      oss << "Cycles since reset:     " << cpu->getLifetimeCounter () << endl;
      pkt->packHexstr (oss.str ().c_str ());
      rsp->putPkt (pkt);
      */
    }
  else if (0 == strcmp (cmd, "exit"))
    {
      // This is a bit of a kludge. It would be much better to be deleted
      // cleanly from the top.
      exit (0);
    }

  // Acknowledge OK
  pkt->packStr ("OK");
  rsp->putPkt (pkt);

  delete [] cmd;

}	// rspCommand ()


//! Handle a RSP set request.

//! There are none that we support, so we always return an empty packet.
void
GdbServer::rspSet ()
{
  pkt->packStr ("");
  rsp->putPkt (pkt);

}	// rspSet ()


//! Handle a RSP 'v' packet

//! These are commands associated with executing the code on the target
void
GdbServer::rspVpkt ()
{
  if (0 == strncmp ("vAttach;", pkt->data, strlen ("vAttach;")))
    {
      // Attaching is a null action, since we have no other process. We just
      // return a stop packet (using TARGET_SIGNAL_TRAP) to indicate we are
      // stopped.
      sprintf (pkt->data, "S%02d", TARGET_SIGNAL_TRAP);
      rsp->putPkt (pkt);
      return;
    }
  else if (0 == strcmp ("vCont?", pkt->data))
    {
      // For now we don't support this.
      pkt->packStr ("");
      rsp->putPkt (pkt);
      return;
    }
  else if (0 == strncmp ("vCont", pkt->data, strlen ("vCont")))
    {
      // This shouldn't happen, because we've reported non-support via vCont?
      // above
      cerr << "Warning: RSP vCont not supported: ignored" << endl;
      return;
    }
  else if (0 == strncmp ("vFile:", pkt->data, strlen ("vFile:")))
    {
      // For now we don't support this.
      cerr << "Warning: RSP vFile not supported: ignored" << endl;
      pkt->packStr ("");
      rsp->putPkt (pkt);
      return;
    }
  else if (0 == strncmp ("vFlashErase:", pkt->data, strlen ("vFlashErase:")))
    {
      // For now we don't support this.
      cerr << "Warning: RSP vFlashErase not supported: ignored" << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }
  else if (0 == strncmp ("vFlashWrite:", pkt->data, strlen ("vFlashWrite:")))
    {
      // For now we don't support this.
      cerr << "Warning: RSP vFlashWrite not supported: ignored" << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }
  else if (0 == strcmp ("vFlashDone", pkt->data))
    {
      // For now we don't support this.
      cerr << "Warning: RSP vFlashDone not supported: ignored" << endl;;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }
  else if (0 == strncmp ("vRun;", pkt->data, strlen ("vRun;")))
    {
      // We shouldn't be given any args, but check for this
      if (pkt->getLen () > strlen ("vRun;"))
	{
	  cerr << "Warning: Unexpected arguments to RSP vRun "
	    "command: ignored" << endl;
	}

      // Restart the current program. However unlike a "R" packet, "vRun"
      // should behave as though it has just stopped. TODO For now we use the
      // SIGNAL_TRAP signal and do nothing.
      sprintf (pkt->data, "S%02d", TARGET_SIGNAL_TRAP);
      rsp->putPkt (pkt);
    }
  else if (0 == strncmp ("vMustReplyEmpty", pkt->data, strlen ("vMustReplyEmpty")))
    {
      pkt->packStr ("");
      rsp->putPkt (pkt);
      return;
    }
  else
    {
      cerr << "Warning: Unknown RSP 'v' packet type " << pkt->data
	   << ": ignored" << endl;
      pkt->packStr ("E01");
      rsp->putPkt (pkt);
      return;
    }
}	// rspVpkt ()


//! Handle a RSP write memory (binary) request

//! Syntax is:

//!   X<addr>,<length>:

//! Followed by the specified number of bytes as raw binary. Response should be
//! "OK" if all copied OK, E<nn> if error <nn> has occurred.

//! The length given is the number of bytes to be written. The data buffer has
//! already been unescaped, so will hold this number of bytes.
void
GdbServer::rspWriteMemBin ()
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
  for (int i = 0; i < len; i++)
    cpu->writeMem (addr + i, (unsigned) bindat[i]);

  pkt->packStr ("OK");
  rsp->putPkt (pkt);

}	// rspWriteMemBin ()


//! Handle a RSP remove breakpoint or matchpoint request

//! This checks that the matchpoint was actually set earlier. For software
//! (memory) breakpoints, the breakpoint is cleared from memory.

//! @todo This doesn't work with icache/immu yet
void
GdbServer::rspRemoveMatchpoint ()
{
  MpType    type;			// What sort of matchpoint
  uint32_t  addr;			// Address specified
  uint32_t  instr;			// Instruction value found
  int       len;			// Matchpoint length
  uint8_t  *instrVec;			// Instruction as byte vector

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

#ifdef GDBSERVER_DEBUG
      fprintf (stderr, "Putting back the instruction (%x) at 0x%x.\n", instr, addr);
#endif

      // Remove the breakpoint from memory. The endianness of the instruction
      // matches that of the memory.
      instrVec = (uint8_t *)(&instr);

      for (int  i = 0 ; i < len; i++)
	    cpu->writeMem (addr + i, (unsigned) instrVec[i]);

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
GdbServer::rspInsertMatchpoint ()
{
  MpType    type;			// What sort of matchpoint
  uint32_t  addr;			// Address specified
  uint32_t  instr;			// Instruction value found
  int       len;			// Matchpoint length
  uint8_t  *instrVec;			// Instruction as byte vector

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
      instrVec = (uint8_t *)(&instr);

      for (int  i = 0 ; i < len; i++)
	instrVec[i] = cpu->readMem (addr + i);

      // Record the breakpoint and write a breakpoint instruction in its
      // place.
      mpHash->add (type, addr, instr);

#ifdef GDBSERVER_DEBUG
      fprintf (stderr, "Inserting a breakpoint over the instruction (%d) at 0x%x:\n", instr, addr);
#endif

      // Little-endian, so least significant byte is at "little" address.
      cpu->writeMem (addr, BREAK_INSTR & 0xff);
      cpu->writeMem (addr + 1, (BREAK_INSTR >> 8) & 0xff);
      cpu->writeMem (addr + 2, (BREAK_INSTR >> 16) & 0xff);
      cpu->writeMem (addr + 3, (BREAK_INSTR >> 24) & 0xff);

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


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:

