// Main program.

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
#include <cstdlib>
#include <cstring>
#include <getopt.h>

// RISC-V headers in general and for each target

#include "ITarget.h"
#include "Ri5cy.h"

// Class headers

#include "GdbServer.h"
#include "TraceFlags.h"

#include "RspConnection.h"
#include "StreamConnection.h"

using std::atoi;
using std::cerr;
using std::cout;
using std::endl;
using std::ostream;
using std::strcmp;


//! The RISC-V model

static ITarget * cpu;


//! Convenience function to output the usage to a specified stream.

//! @param[in] s  Output stream to use.

static void
usage (ostream & s)
{
  s << "Usage: riscv32-gdbserver --core | -c <corename>" << endl
    << "                         [ --trace | -t <traceflag> ]" << endl
    << "                         [ --silent | -q ]" << endl
    << "                         [ --stdin | -s ]" << endl
    << "                         [ --help | -h ]" << endl
    << "                         <rsp-port>" << endl
    << endl
    << "The trace option may appear multiple times. Trace flags are:" << endl
    << "  rsp     Trace RSP packets" << endl
    << "  conn    Trace RSP connection handling" << endl
    << "  break   Trace breakpoint handling" << endl
    << "  vcd     Generate a Verilog Change Dump" << endl
    << "  silent  Minimize informative messages (synonym for -q)" << endl
    << "  disas   Disassemble each instruction executed" << endl
    << "  dflush  Flush disassembly to file after each step" << endl;

}	// usage ()


//! Main function

//! @see usage () for information on the parameters.  Instantiates the core
//! and GDB server.

//! @param[in] argc  Number of arguments.
//! @param[in] argv  Vector or arguments.
//! @return  The return code for the program.

int
main (int   argc,
      char *argv[] )
{
  // Argument handling.

  unsigned int  flags = 0;
  bool          silent = false;
  char         *coreName = nullptr;
  bool          from_stdin = false;
  int           port = -1;
  TraceFlags *  traceFlags = new TraceFlags ();

  while (true) {
    int c;
    int longOptind = 0;
    static struct option longOptions[] = {
      {"core",   required_argument, nullptr,  'c' },
      {"help",   no_argument,       nullptr,  'h' },
      {"silent", no_argument,       nullptr,  'q' },
      {"trace",  required_argument, nullptr,  't' },
      {"stdin",  no_argument,       nullptr,  's' },
      {0,       0,                 0,  0 }
    };

    if ((c = getopt_long (argc, argv, "c:hqt:s", longOptions, &longOptind)) == -1)
      break;

    switch (c) {
    case 'c':
      coreName = strdup (optarg);
      break;

    case 'h':
      usage (cout);
      return  EXIT_SUCCESS;

    case 'q':

      traceFlags->flag ("silent", true);
      break;

    case 't':

      if (!traceFlags->isFlag (optarg))
	{
	  cerr << "ERROR: Bad trace flag " << optarg << endl;
	  usage (cerr);
	  return EXIT_FAILURE;
	}

      traceFlags->flag (optarg, true);
      break;

    case 's':
      from_stdin = true;
      break;

    case '?':
    case ':':
      usage (cerr);
      return  EXIT_FAILURE;

    default:
      cerr << "ERROR: getopt_long returned character code " << c << endl;
      return  EXIT_FAILURE;
    }
  }

  // 1 positional arg

  if (((argc - optind) != 1 && !from_stdin)
      || coreName == nullptr)
    {
      usage (cerr);
      return  EXIT_FAILURE;
    }

  // The RISC-V model

  if (0 == strcasecmp ("RI5CY", coreName))
    cpu = new Ri5cy (traceFlags);
  else
    {
      cerr << "ERROR: Unrecognized core: " << coreName << ": exiting" << endl;
      return  EXIT_FAILURE;
    }

  AbstractConnection *conn;
  GdbServer::KillBehaviour killBehaviour;
  if (from_stdin)
    {
      conn = new StreamConnection (traceFlags);
      killBehaviour = GdbServer::KillBehaviour::EXIT_ON_KILL;
    }
  else
    {
      port = atoi (argv[optind]);
      conn = new RspConnection (port, traceFlags);
      killBehaviour = GdbServer::KillBehaviour::RESET_ON_KILL;
    }

  // The RSP server, connecting it to its CPU.

  GdbServer *gdbServer = new GdbServer (conn, cpu, traceFlags, killBehaviour);
  cpu->gdbServer (gdbServer);

  // Run the GDB server.

  int ret = gdbServer->rspServer ();

  // Free memory

  delete  conn;
  delete  gdbServer;
  delete  cpu;
  delete  traceFlags;
  free (coreName);

  return ret;

}	/* sc_main() */


//! Function to handle $time calls in the Verilog

double
sc_time_stamp ()
{
  // If we are called before cpu has been constructed, return 0.0
  if (cpu != 0)
    return cpu->timeStamp ();
  else
    return 0.0;
}


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
