#ifndef LCD_H
#define LCD_H

#include "stm32f0xx.h"
#include <stdint.h>

#define LCD_I2C_ADDR  0x27   // common address, sometimes 0x3F

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(char *str);

#endif