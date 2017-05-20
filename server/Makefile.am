# Makefile.am -- Program automake configuration file
#
# Copyright (C) 2009, 2013 Embecosm Limited
#
# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
#
# This file is part of the RISCV GDB server
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.


bin_PROGRAMS = riscv-gdbserver

riscv_gdbserver_SOURCES = Cpu.cpp           \
                          Cpu.h             \
                          GdbServer.cpp     \
                          GdbServer.h       \
		          main.cpp          \
                          MpHash.cpp        \
                          MpHash.h          \
                          RspConnection.cpp \
                          RspConnection.h   \
                          RspPacket.cpp     \
                          RspPacket.h       \
                          TraceFlags.cpp    \
                          TraceFlags.h      \
                          Utils.cpp         \
                          Utils.h

# Link in the objects we built when we verilated our model
riscv_gdbserver_LDADD = $(VTESTBENCH)/Vtestbench__ALL.a $(VTESTBENCH)/verilated.o $(VTESTBENCH)/verilated_dpi.o
