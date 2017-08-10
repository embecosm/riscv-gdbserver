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

riscv_gdbserver_SOURCES = AbstractConnection.cpp \
                          AbstractConnection.h   \
                          Disassembler.h         \
                          Disassembler.cpp       \
                          GdbServer.cpp          \
                          GdbServer.h            \
                          GdbServerImpl.cpp      \
                          GdbServerImpl.h        \
		          main.cpp               \
                          MpHash.cpp             \
                          MpHash.h               \
                          RspConnection.cpp      \
                          RspConnection.h        \
                          RspPacket.cpp          \
                          RspPacket.h            \
                          StreamConnection.cpp   \
                          StreamConnection.h     \
                          Utils.cpp              \
                          Utils.h

# For now. Link in the common binaries (we only need these from one model) and
# the actual models.

riscv_gdbserver_LDADD = ../targets/libtargets.la                   \
                        ../trace/libtrace.la                       \
                        @MDIR_RI5CY@/verilated.o                   \
                        @MDIR_RI5CY@/verilated_vcd_c.o             \
                        @MDIR_RI5CY@/verilated_dpi.o               \
                        @MDIR_PICORV32@/V@MODNAME_PICORV32@__ALL.a \
                        @MDIR_RI5CY@/V@MODNAME_RI5CY@__ALL.a       \
                        -L$(BINUTILS_LIBDIR) -lopcodes -lbfd

riscv_gdbserver_CPPFLAGS = -I$(top_srcdir)/targets          \
                           -I$(top_srcdir)/targets/common   \
                           -I$(top_srcdir)/targets/picorv32 \
                           -I$(top_srcdir)/targets/ri5cy    \
                           -I$(top_srcdir)/trace            \
                           -I$(BINUTILS_INCDIR)
