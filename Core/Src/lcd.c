#include "lcd.h"
#include "main.h"
#include <stdint.h>

/*
   PCF8574 to LCD mapping (common backpack):
   P0 = RS
   P1 = RW
   P2 = EN
   P3 = Backlight
   P4 = D4
   P5 = D5
   P6 = D6
   P7 = D7
*/

#define LCD_RS         0x01
#define LCD_RW         0x02
#define LCD_EN         0x04
#define LCD_BACKLIGHT  0x08

static void I2C1_Write(uint8_t addr, uint8_t data);
static void LCD_Write4Bits(uint8_t nibble, uint8_t control);
static void LCD_SendCommand(uint8_t cmd);
static void LCD_SendData(uint8_t data);
static void LCD_Delay(volatile uint32_t count);

static void LCD_Delay(volatile uint32_t count)
{
    while (count--) __NOP();
}

static void I2C1_Write(uint8_t addr, uint8_t data)
{
    while (I2C1->ISR & I2C_ISR_BUSY);

    I2C1->CR2 = 0;
    I2C1->CR2 |= ((uint32_t)(addr << 1) << I2C_CR2_SADD_Pos);
    I2C1->CR2 |= (1 << I2C_CR2_NBYTES_Pos);
    I2C1->CR2 |= I2C_CR2_START;

    while (!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = data;

    while (!(I2C1->ISR & I2C_ISR_TC));
    I2C1->CR2 |= I2C_CR2_STOP;

    while (I2C1->CR2 & I2C_CR2_STOP);
}

static void LCD_Write4Bits(uint8_t nibble, uint8_t control)
{
    uint8_t data = 0;

    data |= (nibble & 0xF0);
    data |= control;
    data |= LCD_BACKLIGHT;

    I2C1_Write(LCD_I2C_ADDR, data);
    LCD_Delay(2000);

    I2C1_Write(LCD_I2C_ADDR, data | LCD_EN);
    LCD_Delay(2000);

    I2C1_Write(LCD_I2C_ADDR, data & ~LCD_EN);
    LCD_Delay(2000);
}

static void LCD_SendCommand(uint8_t cmd)
{
    LCD_Write4Bits(cmd & 0xF0, 0);
    LCD_Write4Bits((cmd << 4) & 0xF0, 0);
    LCD_Delay(50000);
}

static void LCD_SendData(uint8_t data)
{
    LCD_Write4Bits(data & 0xF0, LCD_RS);
    LCD_Write4Bits((data << 4) & 0xF0, LCD_RS);
    LCD_Delay(50000);
}

void LCD_Init(void)
{
    LCD_Delay(200000);

    // Initialize in 4-bit mode
    LCD_Write4Bits(0x30, 0);
    LCD_Delay(50000);

    LCD_Write4Bits(0x30, 0);
    LCD_Delay(50000);

    LCD_Write4Bits(0x30, 0);
    LCD_Delay(50000);

    LCD_Write4Bits(0x20, 0);
    LCD_Delay(50000);

    // Function set: 4-bit, 2 lines, 5x8 font
    LCD_SendCommand(0x28);

    // Display ON, cursor OFF
    LCD_SendCommand(0x0C);

    // Entry mode set
    LCD_SendCommand(0x06);

    // Clear display
    LCD_SendCommand(0x01);
    LCD_Delay(100000);
}

void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
    LCD_Delay(100000);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address;

    if (row == 0)
        address = 0x80 + col;
    else
        address = 0xC0 + col;

    LCD_SendCommand(address);
}

void LCD_Print(char *str)
{
    while (*str)
    {
        LCD_SendData((uint8_t)(*str));
        str++;
    }
}