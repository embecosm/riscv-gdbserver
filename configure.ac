# Top level autoconf configuration file
#
# Copyright (C) 2013 Embecosm Limited
#
# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
#
# This file is part of the RISC-V GDB server
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


# Process this file with autoconf to produce a configure script.

AC_INIT([riscv-gdbserver], [0.0.2], [support@embecosm.com])
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AC_COPYRIGHT([Copyright (C) 2017 Embecosm Limited])

# Make sure we are using a reasonably recent autoconf version
AC_PREREQ(2.69)

# We're using C++
AC_LANG(C++)

# C++ compiler and install utility
AC_PROG_CXX
AC_PROG_INSTALL

WERROR_FLAG=""
AC_ARG_WITH([werror],
	    AC_HELP_STRING([--without-werror],
                           [disable use of -Werror compiler flag]))
AS_IF([test "x$with_werror" != "xno"],[WERROR_FLAG="-Werror"])

WALL_FLAG=""
AC_ARG_WITH([wall],
	    AC_HELP_STRING([--without-wall],
                           [disable use of -Wall compiler flag]))
AS_IF([test "x$with_wall" != "xno"],[WALL_FLAG="-Wall"])

WEXTRA_FLAG=""
AC_ARG_WITH([wextra],
	    AC_HELP_STRING([--without-wextra],
                           [disable use of -Wextra compile flag]))
AS_IF([test "x$with_wextra" != "xno"], [WEXTRA_FLAG="-Wextra"])

# Add to CXXFLAGS.
CXXFLAGS="$CXXFLAGS -std=gnu++11 $WERROR_FLAG $WALL_FLAG $WEXTRA_FLAG"

# We need to know where the generic verilator headers are
AC_ARG_WITH(
	[verilator-headers],
	AC_HELP_STRING([--with-verilator-headers],
	               [location of standard verilator headers]),
	[VINCDIR="$withval"],[VINCDIR="usr/share/verilator/include"])

CPPFLAGS="$CPPFLAGS -I$VINCDIR -I$VINCDIR/vltstd"

# We need to know where the target library and headers are for each target,
# and the name of the top module (from which we derive the module name)

# PicoRV32
AC_ARG_WITH(
	[picorv32-modeldir],
	AC_HELP_STRING([--with-picorv32-modeldir],
	               [directory containing the PicoRV32 model & headers]),
	[MDIR_PICORV32="$(cd $withval; pwd)"],
	[MDIR_PICORV32=""])
AC_ARG_WITH(
	[picorv32-topmodule],
	AC_HELP_STRING([--with-picorv32-topmodule],
	               [Name of the top module in the PicoRV32 Verilog]),
	[MODNAME_PICORV32="$withval"],
	[MODNAME_PICORV32="testbench"])
AC_SUBST(MDIR_PICORV32)
AC_SUBST(MODNAME_PICORV32)
AM_CONDITIONAL([BUILD_PICORV32_MODEL], [test -n "${MDIR_PICORV32}"])
AM_COND_IF([BUILD_PICORV32_MODEL], [AC_DEFINE([BUILD_PICORV32_MODEL], [1], [Build the PicoRV32 model])])

# RI5CY
AC_ARG_WITH(
	[ri5cy-modeldir],
	AC_HELP_STRING([--with-ri5cy-modeldir],
	               [directory containing the RI5CY model & headers]),
	[MDIR_RI5CY="$(cd $withval; pwd)"],
        [MDIR_RI5CY=""])
AC_ARG_WITH(
	[ri5cy-topmodule],
	AC_HELP_STRING([--with-ri5cy-topmodule],
	               [Name of the top module in the RI5CY Verilog (default "top")]),
	[MODNAME_RI5CY="$withval"],
	[MODNAME_RI5CY="top"])
AC_SUBST(MDIR_RI5CY)
AC_SUBST(MODNAME_RI5CY)
AM_CONDITIONAL([BUILD_RI5CY_MODEL], [test -n "${MDIR_RI5CY}"])
AM_COND_IF([BUILD_RI5CY_MODEL], [AC_DEFINE([BUILD_RI5CY_MODEL], [1], [Build the Ri5cy model])])

# GDBSIM
AC_ARG_WITH(
	[gdbsim-builddir],
	AC_HELP_STRING([--with-gdbsim-builddir],
	               [directory containing the GDBSIM build (of GDB)]),
	[MDIR_GDBSIM="$(cd $withval; pwd)"],
	[MDIR_GDBSIM=""])
AC_SUBST(MDIR_GDBSIM)
AM_CONDITIONAL([BUILD_GDBSIM_MODEL], [test -n "${MDIR_GDBSIM}"])
AM_COND_IF([BUILD_GDBSIM_MODEL], [AC_DEFINE([BUILD_GDBSIM_MODEL], [1], [Build the GDB simulator model])])

AC_ARG_WITH(
       [gdbsim-incdir],
       AC_HELP_STRING([--with-gdbsim-incdir],
		      [include directory for gdbsim headers]),
       [GDBSIM_INCDIR="$(cd $withval; pwd)"],
       [GDBSIM_INCDIR="$srcdir/../gdb/include"
	 AS_IF([test -d $GDBSIM_INCDIR],
	       [GDBSIM_INCDIR="$(cd $GDBSIM_INCDIR; pwd)"],
	       [AC_MSG_FAILURE([Couldn't find gdbsim header files, use --with-gdbsim-incdir])])])
AC_SUBST(GDBSIM_INCDIR)

# We need to know where to find binutils headers
AC_ARG_WITH(
       [binutils-incdir],
       AC_HELP_STRING([--with-binutils-incdir],
                      [include directory for binutils utilities]),
       [BINUTILS_INCDIR="$(cd $withval; pwd)"],
       [AC_MSG_FAILURE(Binutils include directory not specified)])
AC_SUBST(BINUTILS_INCDIR)

# Register size for this target (32 or 64 bits)
AC_ARG_WITH(
	[xlen],
	AC_HELP_STRING([--with-xlen=XLEN],
	               [Set XLEN as the register bit width (default 32)]),
	[AS_IF([test $withval -ne 32 -a $withval -ne 64],
           [AC_MSG_FAILURE([Invalid --with-xlen value, should be 32 or 64])])],
	[with_xlen=32])
AM_CONDITIONAL([BUILD_64_BIT], [test $with_xlen = 64])
AS_IF([test $with_xlen = 64],
      [AC_DEFINE([BUILD_64_BIT], [1], [Build 64-bit gdbserver])])

# This is GNU compliant source and uses GNU libraries
AC_DEFINE(_GNU_SOURCE, 1, "The source code uses the GNU libraries)

# Check the headers. For now we'll assume that if we have the headers, we have
# the functions (no AC_CHECK_FUNCS).

# Headers for C++
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([cerrno cstdio cstdlib cstring fstream iomanip iostream], , \
                 [AC_MSG_FAILURE(C++ headers missing)])
AC_LANG_POP([C++])
# Headers for C
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h netinet/tcp.h poll.h \
                 inttypes.h sys/socket.h], , \
		 [AC_MSG_FAILURE(C headers missing)])
# Headers for Verilator
AC_CHECK_HEADERS([verilated.h verilated_dpi.h verilated_vcd_c.h], , \
		 [AC_MSG_FAILURE(Verilator headers missing)])

# check for uint{8,32,64}_t in stdint.h.
AC_CHECK_TYPES([uint8_t, uint32_t, uint64_t], , \
               [AC_MSG_FAILURE(fixed unsigned types not available)], \
               [#include <stdint.h>])

# Substitute any changed environment variables
AC_SUBST(CPPFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(VTESTBENCH)

# We want to create a Makefile
AC_CONFIG_FILES([Makefile                  \
                 server/Makefile           \
		 targets/Makefile          \
		 targets/common/Makefile   \
		 targets/picorv32/Makefile \
		 targets/ri5cy/Makefile    \
		 targets/gdbsim/Makefile   \
		 trace/Makefile])

# Put it all out.
AC_OUTPUT
