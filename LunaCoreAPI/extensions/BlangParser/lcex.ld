SECTIONS
{
    .header : {
        KEEP(*(.header))
    }
    .entry : {
        KEEP(*(.entry))
    }
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
    }
}