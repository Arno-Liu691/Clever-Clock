/*
 * LCD_I2C.c
 *
 *  Created on: Nov 9, 2024
 *      Author: Arno
 */
#include "LCD_I2C.h"


#define LCD_I2C_ADDRESS 0x4E  // I2C address for the LCD in arduino is 7-bit, 0x27, and in stm32 is 8-bit


// Set cursor to specific row and column
void LCD_Set_Cursor(uint8_t row, uint8_t col) {
	// for 16*2 lcd screen
    switch (row)
        {
            case 0:
                col |= 0x80;
                break;
            case 1:
                col |= 0xC0;
                break;
        }
    LCD_Send_Command(col);

// for 20*4 lcd screen
//    switch (row)
//        {
//    	   case 0:
//               col |= 0x80;  // First row, starting address 0x80
//               break;
//           case 1:
//               col |= 0xC0;  // Second row, starting address 0xC0
//               break;
//           case 2:
//               col |= 0x94;  // Third row, starting address 0x94
//               break;
//           case 3:
//               col |= 0xD4;  // Fourth row, starting address 0xD4
//               break;
//           default:
//               return;  // Invalid row, do nothing
//        }
//    LCD_Send_Command(col);
}

// Send a command to the LCD
void LCD_Send_Command(uint8_t cmd) {
    uint8_t upper_nibble = (cmd & 0xF0);
    uint8_t lower_nibble = ((cmd << 4) & 0xF0);

    uint8_t data[4] = {
        upper_nibble | 0x0C,   // Enable high
        upper_nibble | 0x08,   // Enable low
        lower_nibble | 0x0C,   // Enable high
        lower_nibble | 0x08    // Enable low
    };
    HAL_I2C_Master_Transmit(&hi2c3, LCD_I2C_ADDRESS, data, sizeof(data), HAL_MAX_DELAY);
}

// Send data (character) to the LCD
void LCD_Send_Data(uint8_t data) {
    uint8_t upper_nibble = (data & 0xF0);
    uint8_t lower_nibble = ((data << 4) & 0xF0);

    uint8_t data_array[4] = {
        upper_nibble | 0x0D,   // Enable high, RS = 1
        upper_nibble | 0x09,   // Enable low, RS = 1
        lower_nibble | 0x0D,   // Enable high, RS = 1
        lower_nibble | 0x09    // Enable low, RS = 1
    };
    HAL_I2C_Master_Transmit(&hi2c3, LCD_I2C_ADDRESS, data_array, sizeof(data_array), HAL_MAX_DELAY);
}

// Send a string to the LCD
void LCD_Send_String(char *str) {
    while (*str) {
        LCD_Send_Data((uint8_t)(*str));
        str++;
    }
}

void LCD_Init(void) {
    // Initialization sequence for the LCD
    HAL_Delay(50);                // Wait for LCD to power up
    LCD_Send_Command(0x30);       // Wake up
    HAL_Delay(5);
    LCD_Send_Command(0x30);       // Wake up
    HAL_Delay(1);
    LCD_Send_Command(0x30);       // Wake up
    LCD_Send_Command(0x20);       // 4-bit mode

    // Configure display
    LCD_Send_Command(0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
    HAL_Delay(1);
    LCD_Send_Command(0x08); //Display on/off control --> D=0,C=0, B=0  ---> display off
    HAL_Delay(1);
    LCD_Send_Command(0x01);  // clear display
    HAL_Delay(2);
    LCD_Send_Command(0x06); //Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
    HAL_Delay(1);
    LCD_Send_Command(0x0C); //Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
}
