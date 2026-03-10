#include <stdint.h>

void fpu_set_control_word(const uint16_t cw) {
    __asm__ volatile("fldcw %0" ::"m"(cw));
}

void FPU_enable() {
    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" :"=r"(cr4));
    // set 9th bit to 1 in cr4
    cr4 |= 0x200;
    __asm__ volatile("mov %0, %%cr4" ::"r"(cr4));
    fpu_set_control_word(0x37F); // fpu init
    fpu_set_control_word(0x37E); // invalid operand exceptions
    fpu_set_control_word(0x37A); // divide by zero
}