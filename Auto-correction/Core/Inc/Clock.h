#ifndef INC_CLOCK_H_
#define INC_CLOCK_H_

#include "main.h"
#include "LCD_I2C.h"
#include "tim.h"

// Define a structure to hold time information
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} Time;

extern Time currentTime;
extern uint8_t sub_second_counter;
extern uint8_t dts_flag;
extern int32_t ppm;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void set_time(uint8_t year, uint8_t month, uint8_t day, uint8_t week, uint8_t hour, uint8_t minute, uint8_t second, uint8_t DTS);
const char* weekday_string(uint8_t week);
uint8_t days_in_month(uint8_t month, uint8_t year);
int is_last_sunday(uint8_t day, uint8_t month, uint8_t year);
void Adjustment();
void Calibration();
void Plus();
void Minus();
void PPM_Init();

#endif /* INC_CLOCK_H_ */
