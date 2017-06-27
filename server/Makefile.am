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

riscv_gdbserver_SOURCES = GdbServer.cpp     \
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

# For now. Link in the common binaries (we only need these from one model) and
# the actual models.

riscv_gdbserver_LDADD = ../targets/libtargets.la                   \
                        @MDIR_PICORV32@/verilated.o                \
                        @MDIR_PICORV32@/verilated_vcd_c.o          \
                        @MDIR_PICORV32@/verilated_dpi.o            \
                        @MDIR_PICORV32@/V@MODNAME_PICORV32@__ALL.a \
                        @MDIR_RI5CY@/V@MODNAME_RI5CY@__ALL.a

riscv_gdbserver_CPPFLAGS = -I$(top_srcdir)/targets          \
                           -I$(top_srcdir)/targets/common   \
                           -I$(top_srcdir)/targets/picorv32 \
                           -I$(top_srcdir)/targets/ri5cy
