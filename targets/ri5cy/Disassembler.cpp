#include "Disassembler.h"
#include "config.h"
#include <string.h>
#include "dis-asm.h"
#include "disassemble.h"
#include "bfd.h"
#include <stdint.h>


Disassembler::Disassembler ()
{
  init_disassemble_info (&disasm_info, stdout, (fprintf_ftype) fprintf);
  disassemble_init_for_target (& disasm_info);

  disasm_info.buffer = vals;
  disasm_info.buffer_vma = (bfd_vma) vals;
  disasm_info.buffer_length = 4;    
}

void Disassembler::disassemble_riscv (uint32_t insn)
{
  memcpy (vals, &insn, 4);
  print_insn_riscv ((bfd_vma) vals, (disassemble_info*) &disasm_info);
}
