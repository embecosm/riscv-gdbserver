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

// RISC-V headers
#include "Cpu.h"

// Class headers
#include "GdbServer.h"
#include "TraceFlags.h"


using std::atoi;
using std::cerr;
using std::cout;
using std::endl;
using std::strcmp;


int
main (int   argc,
      char *argv[] )
{
  // Argument handling. There is an optional trace flag, followed by three
  // positional arguments, the library, the device within the library and the
  // RSP port. All three positional arguments must be specified.
  unsigned int  flags;

  while (true) {
    int c;
    int longOptind = 0;
    static struct option longOptions[] = {
      {"trace", required_argument, 0,  0 },
      {0,       0,                 0,  0 }
    };

    if ((c = getopt_long (argc, argv, "t:", longOptions, &longOptind)) == -1)
      break;

    switch (c) {
    case 0:
    case 't':
      // We only have the one long option for now (--trace), so no checking.
      // @todo We should allow more than just decimal values.
      flags = atoi (optarg);
      break;

    case '?':
    case ':':
      cerr << "Usage: riscv-gdbserver [ -trace | -t <traceflags> ]" << endl
           << "                     <library> <devicename> <rsp-port>" << endl;
      return 255;

    default:
      cerr << "ERROR: getopt_long returned character code " << c << endl;
    }
  }

  // 3 positional args
  if ((argc - optind) != 3) {
      cerr << "Usage: riscv-gdbserver [ -trace | -t <traceflags> ]" << endl
           << "                     <library> <devicename> <rsp-port>" << endl;
      cerr << "argc = " << argc << ", optind = " << optind << endl;
      return 255;
    }

  char *lib  = argv[optind];
  char *dev  = argv[optind + 1];
  int   port = atoi (argv[optind + 2]);
  TraceFlags *traceFlags = new TraceFlags (flags);

  // The RISC-V model
  Cpu    *cpu = new Cpu ();

  // The RSP server
  GdbServer *gdbServer = new GdbServer (port, cpu, traceFlags);

  // Run the GDB server. If we return we hit some sort of problem.
  gdbServer->rspServer ();

  // Free memory
  delete  gdbServer;
  delete  traceFlags;

  return 255;			// If we return it's a failure!

}	/* sc_main() */


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
