#include "utility.h"

void delay(int32_t count)
{
  /*
   * GCC assembler
   * Format: -
   *   asm [volatile] ( AssemblerTemplate : OutputOperands [ : InputOperands [ : Clobbers ] ])
   *   - AssemblerTemplate: this is a literal string that is the template for
   *                        the assembler code. It is a combination of fixed
   *                        text and tokens that refer to the input, output,
   *                        and goto parameters.
   *   - OutputOperands: a comma-separated list of the C variables modified by
   *                     the instructions in the AssemblerTemplate.
   *   - InputOperands: a comma-separated list of C expressions read by the
   *                    instructions in the AssemblerTemplate.
   *   - Clobbers: a comma-separated list of registers or other values changed
   *               by the AssemblerTemplate, beyond those listed as outputs.
   * 
   * AssemblerTemplate notes: -
   *   - "%=": unique number for each instance of the asm statement
   *
   * OutputOperands notes: -
   *   - "=r"(count) denotes that "count" C variable is overwritten (=) and
   *     that it must be stored in a register (r).
   *
   * InputOperands notes: -
   *   - [count]"0"(count) denotes that the C expression (count) refers to the
   *     asm symbolic name "count" ([count]) with the input in the same place
   *     as the output constraint (zero-based index).
   *
   * Clobbers notes: -
   *   - "cc" denotes that the assembly modifies the flags register.
   *
   * "volatile" is used in this case to remove optimisations
   */
  asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
      : "=r"(count) : [count]"0"(count) : "cc");
}

