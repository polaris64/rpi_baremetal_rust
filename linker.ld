ENTRY (_start)

SECTIONS
{

  /*
   * The Raspberry PI bootloader will start executing code from the
   * kernel at 0x8000
   *
   * NB: qemu-system-aarch64 will load raw binary kernel images at 0x10000!  If
   * running on the final kernel.img, the base address MUST BE 0x10000!
   * Running the buile/output.elf will be OK as QEMU can read the correct base
   * address when loading it.
   */
  . = 0x8000;

  __start = .;
  __text_start = .;
  .text :
  {
    KEEP(*(.text.boot))
    *(.text)
  }
  . = ALIGN(4096);
  __text_end = .;

  __rodata_start = .;
  .rodata :
  {
    *(.rodata)
  }
  . = ALIGN(4096);
  __rodata_end = .;

  __data_start = .;
  .data :
  {
    *(.data)
  }
  . = ALIGN(4096);
  __data_end = .;

  __bss_start = .;
  .bss : {
    bss = .;
    *(.bss)
  }
  . = ALIGN(4096);
  __bss_end = .;
  __end = .;

  __heap_start = . + 0x1000;

  /*
   * Discard all other sections
   */
  /*
  /DISCARD/ : {
    *(*)
  }
  */
}
