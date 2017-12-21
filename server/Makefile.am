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

bin_PROGRAMS =
if BUILD_64_BIT
  bin_PROGRAMS += riscv64-gdbserver
else
  bin_PROGRAMS += riscv32-gdbserver
endif

if BUILD_GDBSIM_MODEL
  MAYBE_GDBSIM_LDADD=@MDIR_GDBSIM@/sim/riscv/libsim.a           \
		     @MDIR_GDBSIM@/bfd/libbfd.a                 \
		     @MDIR_GDBSIM@/opcodes/libopcodes.a         \
		     @MDIR_GDBSIM@/libiberty/libiberty.a        \
		     @MDIR_GDBSIM@/zlib/libz.a                  \
		     -ldl
  MAYBE_GDBSIM_CPPFLAGS=-I$(top_srcdir)/targets/gdbsim
endif

if BUILD_PICORV32_MODEL
  MAYBE_PICORV32_LDADD=@MDIR_PICORV32@/V@MODNAME_PICORV32@__ALL.a
  MAYBE_PICORV32_CPPFLAGS=-I$(top_srcdir)/targets/picorv32
endif

if BUILD_RI5CY_MODEL
  MAYBE_RI5CY_LDADD=@MDIR_RI5CY@/V@MODNAME_RI5CY@__ALL.a
  MAYBE_RI5CY_CPPFLAGS=-I$(top_srcdir)/targets/ri5cy
endif

if BUILD_RI5CY_MODEL
  MAYBE_VERILATOR_RI5CY_LDADD=@MDIR_RI5CY@/verilated.o        \
			      @MDIR_RI5CY@/verilated_vcd_c.o  \
			      @MDIR_RI5CY@/verilated_dpi.o
else
if BUILD_PICORV32_MODEL
  MAYBE_VERILATOR_PICORV32_LDADD=@MDIR_PICORV32@/verilated.o        \
				 @MDIR_PICORV32@/verilated_vcd_c.o  \
				 @MDIR_PICORV32@/verilated_dpi.o
endif
endif

MAYBE_VERILATOR_LDADD=$(MAYBE_VERILATOR_RI5CY_LDADD) \
		      $(MAYBE_VERILATOR_PICORV32_LDADD)

riscv64_gdbserver_SOURCES = $(ALL_SOURCES)
riscv32_gdbserver_SOURCES = $(ALL_SOURCES)

riscv64_gdbserver_LDADD = $(ALL_LDADD)
riscv32_gdbserver_LDADD = $(ALL_LDADD)

riscv64_gdbserver_CPPFLAGS = $(ALL_CPPFLAGS)
riscv32_gdbserver_CPPFLAGS = $(ALL_CPPFLAGS)

ALL_SOURCES = AbstractConnection.cpp \
	      AbstractConnection.h   \
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
              SyscallReplyPacket.h   \
              Utils.cpp              \
              Utils.h

# For now. Link in the common binaries (we only need these from one model) and
# the actual models.

ALL_LDADD = ../targets/libtargets.la                   \
            ../trace/libtrace.la                       \
	    $(MAYBE_VERILATOR_LDADD)		       \
	    $(MAYBE_GDBSIM_LDADD)		       \
	    $(MAYBE_RI5CY_LDADD)		       \
	    $(MAYBE_PICORV32_LDADD)

ALL_CPPFLAGS = -I$(top_srcdir)/targets          \
               -I$(top_srcdir)/targets/common   \
               -I$(top_srcdir)/trace            \
	       -I$(BINUTILS_INCDIR)             \
	       $(MAYBE_PICORV32_CPPFLAGS)       \
	       $(MAYBE_RI5CY_CPPFLAGS)          \
	       $(MAYBE_GDBSIM_CPPFLAGS)
