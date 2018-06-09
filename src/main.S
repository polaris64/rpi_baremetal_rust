/*
Example kernel for Raspberry Pi 2
*/
.section ".text.boot"
.global _start

_start:

  /* Disable all but CPU core #0 */

  /*
  Move register c0 from coprocessor 15 to ARM register r1. This effectively
  returns the current CPU core number [0, 4] in the lower 3-bits of r1.
  Syntax: MRC coproc, #opcode1, Rt, CRn, CRm, #opcode2
   - coproc: coprocessor name (pn, n: [0, 15])
   - #opcode1: 3-bit coprocessor-specific opcode
   - Rt: ARM destination register
   - CRn and CRm: coprocessor registers
   - #opcode2: optional 3-bit coprocessor-specific opcode
  */
  mrc p15, #0, r1, c0, c0, #5

  /*
  Mask lower 3-bits of result and compare to #0; if the core is not #0 then
  halt it.
  */
  and r1, r1, #3
  cmp r1, #0
  bne halt


  /*
  Kernel is loaded at 0x8000 onwards (.init section), so initial stack can
  safely grow backwards from this point.
  */
  mov sp, #0x8000

  /*
  Load registers with the addresses of the start and and of the BSS section; C
  stores all uninitialised global variables in the BSS section.
  */
  ldr r4, =__bss_start
  ldr r9, =__bss_end


  /*
  C requires that the entire BSS section be initialised to zero, so set all
  addressed to 0.
  */

  /* Define 4 words of 0 */
  mov r5, #0
  mov r6, #0
  mov r7, #0
  mov r8, #0

  /* Branch forward to numeric label 2 */
  b 2f

  1:
    /*
    Store 4 words (stm-) at r4 and increment r4 after (-ia, r4! means to write
    value back to r4) by the length of data stored
    */
    stmia r4!, {r5-r8}

  2:
    cmp r4, r9 /* End of BSS section? */
    blo 1b     /* Branch backward to numeric label 1 */


  /*
  Branch to global kernel_main symbol
  BL performs a branch and sets the LR (link register) to the next instruction,
  allowing a return from the kernel_main subroutine. Upon return, the following
  "halt:" label will be hit, halting this CPU core.
  */
  bl kernel_main
  /*
  ldr r3, =kernel_main
  blx r3
  */


halt:
  wfe    /* Wait for event */
  b halt
