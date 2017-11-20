# cxx_flag_check.m4 -- check c++ flags			-*- Autoconf -*-
#
# Copyright (C) 2017 Embecosm Limited
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

dnl @synopsis CXX_FLAGS_SUPPORTED [compiler flags]
dnl @summary check whether compiler supports given C++ flags or not, the
dnl          variable 'cxx_flag_supported' is set to either 'yes' or 'no'.
AC_DEFUN([CXX_FLAG_SUPPORTED],
[dnl
  AC_MSG_CHECKING([if $CXX supports $1])
  AC_LANG_PUSH([C++])
  ac_saved_cxxflags="$CXXFLAGS"
  CXXFLAGS="-Werror $1"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [AC_MSG_RESULT([yes])
    cxx_flag_supported=yes],
    [AC_MSG_RESULT([no])
    cxx_flag_supported=no]
  )
  CXXFLAGS="$ac_saved_cxxflags"
  AC_LANG_POP([C++])
])

dnl @synopsis CXX_FLAGS_REQUIRED [compiler flags]
dnl @summary check whether compiler supports given C++ flags or not, if not
dnl          then issue an error and the configuration stops.
AC_DEFUN([CXX_FLAG_REQUIRED],
[dnl
  CXX_FLAG_SUPPORTED([$1])
  AS_IF([test $cxx_flag_supported = no],
      [AC_MSG_ERROR([Required compiler flag $1 is not supported by $CXX])])
])
