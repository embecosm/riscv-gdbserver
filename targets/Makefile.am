# Makefile.am -- automake configuration file for top level targets directory

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

SUBDIRS = common ri5cy

noinst_LTLIBRARIES = libtargets.la

libtargets_la_SOURCES = ITarget.cpp \
                        ITarget.h

libtargets_la_LIBADD = common/libcommon.la     \
                       ri5cy/libri5cy.la

libtargets_la_CPPFLAGS = -I$(srcdir)/common     \
                         -I$(top_srcdir)/server \
                         -I$(top_srcdir)/trace

libtargets_la_CXXFLAGS = -Werror -Wall -Wextra
