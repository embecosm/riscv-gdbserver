# Makefile.am -- automake configuration file for the PicoRV32 target

# Copyright (C) 2017 Embecosm Limited

# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

# This file is part of the RISC-V GDB server

# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.

noinst_LTLIBRARIES = libpicorv32.la

libpicorv32_la_SOURCES = Picorv32.cpp Picorv32Impl.cpp

# FIXME: Remove -Wno-unused-paramted once the Picorv32 class is implemented
# properly.
libpicorv32_la_CXXFLAGS = -Werror -Wall -Wextra -Wno-unused-parameter \
                          -I$(srcdir)/..        \
                          -I$(srcdir)/../common \
			  -I@MDIR_PICORV32@
