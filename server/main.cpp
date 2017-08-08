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
#include "Picorv32.h"
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
  s << "Usage: riscv-gdbserver --core | -c <corename>" << endl
    << "                       [ --trace | -t <traceflags> ]" << endl
    << "                       [ --silent | -q ]" << endl
    << "                       [ --stdin | -s ]" << endl
    << "                       [ --help | -h ]" << endl
    << "                       <rsp-port>" << endl;

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

      return  EXIT_SUCCESS;

    case 't':

      // @todo We should allow more than just decimal values.

      flags |= atoi (optarg);
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
    }
  }

  // 1 positional arg

  if (((argc - optind) != 1 && !from_stdin)
      || coreName == nullptr)
    {
      usage (cerr);
      return  EXIT_FAILURE;
    }

  // Trace flags for the server

  TraceFlags * traceFlags = new TraceFlags (flags);

  if (silent)
    traceFlags->setSilent ();

  // The RISC-V model

  if (0 == strcasecmp ("PicoRV32", coreName))
    cpu = new Picorv32 (traceFlags->traceVcd());
  else if (0 == strcasecmp ("RI5CY", coreName))
    cpu = new Ri5cy (traceFlags->traceVcd());
  else
    {
      cerr << "ERROR: Unrecognized core: " << coreName << ": exiting" << endl;
      return  EXIT_FAILURE;
    }

  AbstractConnection *conn;
  if (from_stdin)
    conn = new StreamConnection (traceFlags);
  else
    {
      port = atoi (argv[optind]);
      conn = new RspConnection (port, traceFlags);
    }

  // The RSP server

  GdbServer *gdbServer = new GdbServer (conn, cpu, traceFlags);

  // Run the GDB server. If we return, then we have hit some sort of problem.

  gdbServer->rspServer ();

  // Free memory

  delete  gdbServer;
  delete  traceFlags;
  delete  cpu;
  free (coreName);

  return EXIT_FAILURE;			// If we return it's a failure!

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
