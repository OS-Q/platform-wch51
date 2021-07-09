#include "CH559.h"
#include <compiler.h>
#include <stdint.h>

SBIT(LED, 0xB0, 3);


void delay()
{
    uint32_t i;
    for (i = 0; i < (120000UL); i++)
    {
        __asm__("nop");
    }
}


void main()
{
    PORT_CFG = 0b10000111;
    P3_DIR   = 0b11111111;
    P3_PU    = 0b00000000;
    LED = 0;

    while(1)
    {
        LED = !LED;
        delay();
    }
}

