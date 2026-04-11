OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    . = 0x400000;

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }

    /* Force inclusion of bss section in the file*/
    .fill :
    {
        . = ALIGN(4);
        BYTE(0)
    }
}