#include "Clock.h"
#include "stm32l4xx_it.h"
#include "EEPROM.h"

// Create a global variable to store the current time
Time currentTime;
uint8_t sub_second_counter = 0;
uint8_t dts_flag = 1;
uint8_t adjustment_stage = 0;
bool calibration_stage = 0;
int32_t driftTime = 0;
uint8_t adjust_flag = 0;
uint32_t previousTick = 0;
uint64_t elapsedTime = 0; // Use 64-bit to track the total elapsed time in milliseconds, setup initial value for testing
int32_t ppm = 0;
uint32_t ppmCounter = 0;
uint32_t ppmThreshold = 0;

// Function to set the initial time
void set_time(uint8_t year, uint8_t month, uint8_t day, uint8_t week, uint8_t hour, uint8_t minute, uint8_t second, uint8_t DTS) {
    currentTime.year = year;
    currentTime.month = month;
    currentTime.day = day;
    currentTime.week = week;
    currentTime.hour = hour;
    currentTime.minute = minute;
    currentTime.second = second;
    dts_flag = DTS; //0 for Apr to Oct and 1 for Nov to March
}

// Function to convert weekday number to string
const char* weekday_string(uint8_t week) {
    const char *weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    if (week >= 1 && week <= 7) {
        return weekdays[week-1];
    } else {
        return "N/A"; // If week number is out of range
    }
}

// Function to get the number of days in the current month
uint8_t days_in_month(uint8_t month, uint8_t year) {
    switch (month) {
        case 1: return 31; // January
        case 2: return (year % 4 == 0) ? 29 : 28; // February
        case 3: return 31; // March
        case 4: return 30; // April
        case 5: return 31; // May
        case 6: return 30; // June
        case 7: return 31; // July
        case 8: return 31; // August
        case 9: return 30; // September
        case 10: return 31; // October
        case 11: return 30; // November
        case 12: return 31; // December
        default: return 30; // Default case (should not happen)
    }
}

int is_last_sunday(uint8_t day, uint8_t month, uint8_t year) {
    // Get the total number of days in the month
    uint8_t days = days_in_month(month, year);

    // If the day is a Sunday and it's the last occurrence of that weekday in the month
    if ((day + 7) > days && currentTime.week == 7) {
        return 1; // True if it is the last Sunday
    }
    return 0; // False otherwise
}

// Timer callback function to update the time each second
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM15)
	{
		Update_Time_OnScreen();
		adjust_flag = !adjust_flag;
		if(adjust_flag)
		{
			switch (adjustment_stage) {
				case 1:
					LCD_Set_Cursor(0,3);
					LCD_Send_String("    ");
					break;
				case 2:
					LCD_Set_Cursor(0,8);
					LCD_Send_String("  ");
					break;
				case 3:
					LCD_Set_Cursor(0,11);
					LCD_Send_String("  ");
					break;
				case 4:
					LCD_Set_Cursor(0,14);
					LCD_Send_String("   ");
					break;
				case 5:
					LCD_Set_Cursor(1,6);
					LCD_Send_String("  ");
					break;
				case 6:
					LCD_Set_Cursor(1,9);
					LCD_Send_String("  ");
					break;
				case 7:
					LCD_Set_Cursor(1,12);
					LCD_Send_String("  ");
					break;
				default:
					break;
				}
		}
	}
    if (htim->Instance == TIM2) { // Assuming TIM2 is used for the 10Hz timer
    	uint32_t currentTick = HAL_GetTick();

		// Calculate the time elapsed since the last callback (handles wrap-around)
		uint32_t tickDifference = currentTick - previousTick;
		previousTick = currentTick;

		// Add the tick difference to the total elapsed time
		elapsedTime += tickDifference;
    	sub_second_counter++;
    	if(ppm != 0){
    		ppmCounter++;
    	}
    	if(ppm != 0 && ppmThreshold != 0 && ppmCounter >= ppmThreshold)
    	{
    		if(ppm>0){
    			sub_second_counter--;
    		}
    		else{
    			sub_second_counter++;
    		}
    		ppmCounter = 0;
    	}
		// Increment the second
		if (sub_second_counter >= 10){
			sub_second_counter = 0;
			currentTime.second++;

			// Handle minute increment
			if (currentTime.second >= 60) {
				currentTime.second = 0;
				currentTime.minute++;

				// Handle hour increment
				if (currentTime.minute >= 60) {
					currentTime.minute = 0;
					currentTime.hour++;

					// Handle day increment
					if (currentTime.hour >= 24) {
						currentTime.hour = 0;
						currentTime.day++;
						currentTime.week = (currentTime.week % 7) + 1; // Increment the weekday, wrapping around

						// Handle month increment with correct day limits
						if (currentTime.day > days_in_month(currentTime.month, currentTime.year)) {
							currentTime.day = 1;
							currentTime.month++;

							// Handle year increment
							if (currentTime.month > 12) {
								currentTime.month = 1;
								currentTime.year++;
							}
						}
					}

					// Handle Daylight Saving Time changes
					if (currentTime.month == 10 && is_last_sunday(currentTime.day, currentTime.month, currentTime.year)) {
						// DST ends: Move clock back by 1 hour at 1 AM
						if (currentTime.hour == 1 && dts_flag == 0) {
							currentTime.hour = 0;
							dts_flag = 1;
						}
					}
					if (currentTime.month == 3 && is_last_sunday(currentTime.day, currentTime.month, currentTime.year)) {
						// DST starts: Move clock forward by 1 hour at 1 AM
						if (currentTime.hour == 0 && dts_flag == 1) {
							currentTime.hour = 1;
							dts_flag = 0;
						}
					}
				}
			}
    	}
        Update_Time_OnScreen(); // Update the LCD with the current time
	}
}

void Adjustment()
{
	if(calibration_stage == 0) // only execute when not in the calibration mode
	{
		if(adjustment_stage == 0)
		{
			HAL_TIM_Base_Stop_IT(&htim2);
			HAL_TIM_Base_Start_IT(&htim15);
			LCD_Set_Cursor(3,0);
			LCD_Send_String("Adjustment ON");
			HAL_GPIO_TogglePin(Red_LED_GPIO_Port, Red_LED_Pin);
		}
		if(adjustment_stage > 6)
		{
			LCD_Set_Cursor(3,0);
			LCD_Send_String("                    ");
			HAL_TIM_Base_Stop_IT(&htim15);
			adjustment_stage = 0;
			if(currentTime.day > days_in_month(currentTime.month, currentTime.year))
			{
				currentTime.day = days_in_month(currentTime.month, currentTime.year);
			}
			HAL_GPIO_TogglePin(Red_LED_GPIO_Port, Red_LED_Pin);
			previousTick = HAL_GetTick(); // update previousTick to ignore the time used to adjust the clock, because the clock is not operating under this mode
			HAL_TIM_Base_Start_IT(&htim2);
			return;
		}
		adjustment_stage++;
	}
}

void Calibration()
{
	if(adjustment_stage == 0) // only execute when not in the adjustment mode
	{
		HAL_GPIO_TogglePin(Red_LED_GPIO_Port, Red_LED_Pin);
		calibration_stage = !calibration_stage;
		if(calibration_stage == 1)
		{
			LCD_Set_Cursor(3,0);
			LCD_Send_String("Calibration ON");
		}
		else
		{
			int32_t delta_ppm = (int32_t)((driftTime * 1000000LL) / (int64_t)elapsedTime);
			// (ppm * 100000) / 1000000 is the Drift in microseconds per 0.1 second
			// Calculate the correction interval for the threshold of ppmCounter
			if(delta_ppm != 0){
				ppm += delta_ppm;
				driftTime = 0;
				elapsedTime = 0;
				EEPROM_WriteInt32(VIRT_ADDRESS_PPM, ppm);
				ppmThreshold = 100000 / labs(ppm/10);
				char ppmString[15]; // Buffer to hold the PPM string (including sign)
				// Format the signed PPM value with snprintf
				snprintf(ppmString, sizeof(ppmString), "PPM=%+d", (int)ppm); // %+d includes the sign
				// Send the formatted string to the LCD
				LCD_Set_Cursor(2,0);
				LCD_Send_String(ppmString);
			}
			LCD_Set_Cursor(3,0);
			LCD_Send_String("                    ");
		}
	}
}

void Plus()
{
	if(calibration_stage == 0 && adjustment_stage != 0) // not in the calibration mode but in the adjustment mode
	{
		switch (adjustment_stage) {
			case 1:
				// Adjust year (increase)
				currentTime.year = (currentTime.year + 1) % 100;
				break;
			case 2:
				// Adjust month (increase)
				currentTime.month = (currentTime.month % 12) + 1;
				break;
			case 3:
				// Adjust day (increase)
				currentTime.day = (currentTime.day % days_in_month(currentTime.month, currentTime.year)) + 1;
				break;
			case 4:
				// Adjust week (increase)
				currentTime.week = (currentTime.week % 7) + 1;
				break;
			case 5:
				// Adjust hour (increase)
				currentTime.hour = (currentTime.hour + 1) % 24;
				break;
			case 6:
				// Adjust minute (increase)
				currentTime.minute = (currentTime.minute + 1) % 60;
				break;
			case 7:
				// Adjust second (increase)
				currentTime.second = (currentTime.second + 1) % 60;
				break;
			default:
				break;
		    }
	}
	else if(calibration_stage == 1 && adjustment_stage == 0) //not in the adjustment mode but in the calibration mode
	{
		if (sub_second_counter + 5 >= 10) {
		    currentTime.second++;
		}
		sub_second_counter = (sub_second_counter + 5) % 10;
		driftTime -= 500; //minus 500 millisecond each press as the clock is slower so negative ppm
	}
	else
	{
		return;
	}
}

void Minus()
{
	if(calibration_stage == 0 && adjustment_stage != 0) // not in the calibration mode but in the adjustment mode
	{
		switch (adjustment_stage) {
			case 1:
				// Adjust year (decrease)
				currentTime.year = (currentTime.year == 0) ? 99 : currentTime.year - 1;
				break;
			case 2:
				// Adjust month (decrease)
				currentTime.month = (currentTime.month == 1) ? 12 : currentTime.month - 1;
				break;
			case 3:
				// Adjust day (decrease)
				currentTime.day = (currentTime.day == 1) ? days_in_month(currentTime.month, currentTime.year) : currentTime.day - 1;
				break;
			case 4:
				// Adjust week (decrease)
				currentTime.week = (currentTime.week == 1) ? 7 : currentTime.week - 1;
				break;
			case 5:
				// Adjust hour (decrease)
				currentTime.hour = (currentTime.hour == 0) ? 23 : currentTime.hour - 1;
				break;
			case 6:
				// Adjust minute (decrease)
				currentTime.minute = (currentTime.minute == 0) ? 59 : currentTime.minute - 1;
				break;
			case 7:
				// Adjust second (decrease)
				currentTime.second = (currentTime.second == 0) ? 59 : currentTime.second - 1;
				break;
			default:
				break;
		    }
	}
	else if(calibration_stage == 1 && adjustment_stage == 0) //not in the adjustment mode but in the calibration mode
	{
		if (sub_second_counter < 5) {
		    sub_second_counter = (sub_second_counter + 10 - 5) % 10;
		    // Handle underflow for seconds, minutes and hours
		    if (currentTime.second > 0) {
		        currentTime.second--;
		    }
			else
			{
				currentTime.second = 59;
				if (currentTime.minute > 0) {
					currentTime.minute--;
				} else {
					currentTime.minute = 59;
					if (currentTime.hour > 0) {
						currentTime.hour--;
					} else {
						currentTime.hour = 23;
					}
				}
			}
		}
		else{
			sub_second_counter -= 5;
		}
		driftTime += 500; //plus 500 millisecond each press which means the clock is faster than current time so add
	}
	else
	{
		return;
	}
}

void PPM_Init()
{
	if(ppm!=0)
	{
		ppmThreshold = 100000 / labs(ppm/10);
		char ppmString[15]; // Buffer to hold the PPM string (including sign)
		// Format the signed PPM value with snprintf
		snprintf(ppmString, sizeof(ppmString), "PPM=%+d", (int)ppm); // %+d includes the sign
		// Send the formatted string to the LCD
		LCD_Set_Cursor(2,0);
		LCD_Send_String(ppmString);
	}
	else
	{
		LCD_Set_Cursor(2,0);
		LCD_Send_String("PPM=/");
	}
}
