// Register Size Information

// Copyright (C) 2017  Embecosm Limited <www.embecosm.com>

// This file is part of the RISC-V GDB server

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef REGISTERSIZES_H
#define REGISTERSIZES_H

#include "config.h"
#include <cstdint>
#include <cinttypes>

#ifdef BUILD_64_BIT
typedef uint64_t uint_reg_t;
#define PRIxREG PRIx64
#else
typedef uint32_t uint_reg_t;
#define PRIxREG PRIx32
#endif

#endif /* REGISTERSIZES_H */
