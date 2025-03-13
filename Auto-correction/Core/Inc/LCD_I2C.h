#ifndef INC_LCD_I2C_H_
#define INC_LCD_I2C_H_

#include "i2c.h"
#include "Clock.h"
#include "main.h"

void LCD_Init(void);
void LCD_Send_Command(uint8_t cmd);
void LCD_Send_Data(uint8_t data);
void LCD_Send_String(const char *str);
void LCD_Set_Cursor(uint8_t row, uint8_t col);
void Update_Time_OnScreen();

#endif /* INC_LCD_I2C_H_ */
