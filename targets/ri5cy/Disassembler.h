#include "config.h"
#include <string.h>
#include "dis-asm.h"
#include "disassemble.h"
#include "bfd.h"
#include <stdint.h>

class Disassembler
{

  public:
  Disassembler ();
  ~Disassembler () { }
  
  void disassemble_riscv (uint32_t insn);

  private:
  bfd_byte vals[4];
  struct disassemble_info disasm_info;
};
