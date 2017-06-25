# Makefile.am -- automake configuration file for the RI5CY target

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

noinst_LTLIBRARIES = libri5cy.la

libri5cy_la_SOURCES = Ri5cy.cpp     \
                      Ri5cy.h       \
                      Ri5cyImpl.cpp \
                      Ri5cyImpl.h

libri5cy_la_CXXFLAGS = -Werror -Wall -Wextra
