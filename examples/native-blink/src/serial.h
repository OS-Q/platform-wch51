void UART_INIT(void);
void _tm1() __interrupt 3 __using 1;
#ifndef PLATFORMIO
void putchar(unsigned char);
#endif
#define uart_put_char putchar
#define _putchar putchar
