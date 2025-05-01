SECTIONS
{
  . = 0x1000;           /* Set start address */
  .text : { *(.text) }
  .data : { *(.data) }
  .rodata : { *(.rodata) }
  .bss : { *(.bss COMMON) }
}
