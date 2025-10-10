ENTRY(entry)
phys = 0x00100000;
virt = 0xc0000000;
diff = virt-phys;

SECTIONS
{
    . = phys;

    .entry              : { __entry_start = .;      *(.entry)   }

    . += diff;

    .text               : AT(ADDR(.text) - diff)    { __text_start = .;       *(.text)    }
    .data               : AT(ADDR(.data) - diff)    { __data_start = .;       *(.data)    }
    .rodata             : AT(ADDR(.rodata) - diff)  { __rodata_start = .;     *(.rodata)  }
    .bss                : AT(ADDR(.bss) - diff)     { __bss_start = .;        *(.bss)     }

    __bss_end = .;

    .stack              : AT(ADDR(.stack) - diff)   { __stack_start = .;      *(.stack)   }
    
    __end = .;
}
