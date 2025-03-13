/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "LCD_I2C.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define GPS_BUFFER_SIZE 256
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//uint8_t gps_buffer[GPS_BUFFER_SIZE];
uint32_t system_clock = 0;
uint32_t gps_clock = 0;
uint32_t tcxo_clock = 0;
uint32_t tcxo_counter = 0;
//uint32_t previousTick = 0;
uint32_t PrevTime = 0;
uint32_t gps_time = 0;
uint32_t response_time = 0;


float TCXO_PPM = 0.0;
float SYS_PPM = 0.0;

int32_t sys_diff = 0;
int32_t accumulated_time = 0;
int32_t disciplined_time = 0;

uint8_t LED_flag = 0;
uint8_t LED_flag2 = 0;
uint8_t tcxo_flag = 1;
uint8_t PPS_flag = 0;
uint8_t sys_counter = 0;
uint8_t pps_counter = 0;

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//    if (huart->Instance == USART2) {
//        if (strstr((char *)gps_buffer, "$GPRMC") && gps_buffer[18] == 'A') {
//            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // 定位成功，点�????????????????????????????????? LED
//        } else {
//            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // 未定位，熄灭 LED
//        }
//        HAL_UART_Receive_IT(&huart2, gps_buffer, sizeof(gps_buffer));
//    }
//}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) { // internal clock 10Hz
//    	uint32_t currentTick = HAL_GetTick();
//    	system_clock += (currentTick - previousTick)/1000; // system_clock in ms
//    	previousTick = currentTick;
    	sys_counter += 1;
    	system_clock += 1;
    	if (sys_counter >= 999){
    		sys_counter = 0;
    	}
    	response_time = HAL_GetTick() - gps_time;
    	if(response_time < 1001){
    		PPS_flag = 1;
    	}
    	else{
    		PPS_flag = 0;
    		PrevTime = 0;
    		TCXO_PPM = 0.0;
    	}
	}
	if (htim->Instance == TIM2) {
		tcxo_counter += 1; // 100k Hz TCXO
		tcxo_clock += 1;
		if(tcxo_counter >= 99999){
			tcxo_counter = 0;
			if (LED_flag2 == 0){
				HAL_GPIO_WritePin(LED_TCXO_GPIO_Port, LED_TCXO_Pin, GPIO_PIN_SET);
				LED_flag2 = !LED_flag2;
			}
			else{
				HAL_GPIO_WritePin(LED_TCXO_GPIO_Port, LED_TCXO_Pin, GPIO_PIN_RESET);
				LED_flag2 = !LED_flag2;
			}
		}
//		if (tcxo_flag == 1 && tcxo_clock > 1){ // compensate the initial error observed in tcxo
//			tcxo_clock -= 1;
//			tcxo_flag = 0;
//		}
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == PPS_Pin) {
		if (PPS_flag == 1){
			if(PrevTime == 0){
				PrevTime = tcxo_counter;
			}
			else{
				if(pps_counter >= 99){ // every 100 seconds, 10M count will be collected to calculate PPM with 0.1 precision
					disciplined_time += accumulated_time;
					TCXO_PPM = (float)accumulated_time / 10.0;
					pps_counter = 0;
					int32_t temp = tcxo_counter - disciplined_time;
					accumulated_time = temp - (int32_t)PrevTime - 1;
					PrevTime = temp;
					tcxo_counter -= disciplined_time; // process of disciplining
				}
				else{
					pps_counter += 1;
					int32_t temp = tcxo_counter;
					accumulated_time += temp - (int32_t)PrevTime - 1 ;
					PrevTime = temp;
				}
			}
			if (LED_flag == 0){
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
				LED_flag = !LED_flag;
			}
			else{
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
				LED_flag = !LED_flag;
			}
		}
		gps_time = HAL_GetTick();
    }
}

void print_PPM(float TCXO_PPM, float SYS_PPM) {
    int int_TCXO_PPM = (int)TCXO_PPM;   // Extract integer part
    unsigned int decimal_TCXO_PPM = (unsigned int)(fabs(TCXO_PPM - int_TCXO_PPM) * 10);  // Extract first decimal digit
    int int_SYS_PPM = (int)SYS_PPM;   // Extract integer part
    unsigned int decimal_SYS_PPM = (unsigned int)(fabs(SYS_PPM - int_SYS_PPM) * 10);  // Extract first decimal digit

	char tcxoPPM[15];
	char sysPPM[15];

    snprintf(tcxoPPM, sizeof(tcxoPPM), "P=%+d.%u", int_TCXO_PPM, decimal_TCXO_PPM);
    snprintf(sysPPM, sizeof(sysPPM), "P=%+d.%u", int_SYS_PPM, decimal_SYS_PPM);
	LCD_Set_Cursor(1,0);
	LCD_Send_String("                ");
	LCD_Set_Cursor(1,0);
	LCD_Send_String(sysPPM);
	LCD_Set_Cursor(1,10);
	LCD_Send_String(tcxoPPM);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C3_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start_IT(&htim1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	if(sys_counter == 1){
		uint32_t tcxo = tcxo_clock / 100;
		sys_diff = (int32_t)(system_clock - tcxo);
//		SYS_PPM = (float)((int64_t)sys_diff * 1000000LL) / (float)tcxo;
		SYS_PPM = ((float)sys_diff / (float)tcxo) * 1000000.0f;
//		char systemClock[10];
		char tcxoClock[10];
		char GPSClock[10];
		char sysDiff[10];
		char accTime[10];
//		char tcxoPPM[15];
//		char sysPPM[15];
		// Format the signed PPM value with snprintf
//		snprintf(systemClock, sizeof(systemClock), "S=%lu", system_clock);
		snprintf(tcxoClock, sizeof(tcxoClock), "T=%lu", (tcxo/1000));
		snprintf(GPSClock, sizeof(GPSClock), "G=%u", pps_counter);
		snprintf(sysDiff, sizeof(sysDiff), "D=%+d", (int)sys_diff);
		snprintf(accTime, sizeof(accTime), "A=%+d", (int)accumulated_time);
//		snprintf(tcxoPPM, sizeof(tcxoPPM), "P=%+.1f", (float)TCXO_PPM); // %+d includes the sign
//		snprintf(sysPPM, sizeof(sysPPM), "P=%+.1f", (float)SYS_PPM);
//		snprintf(tcxoPPM, sizeof(tcxoPPM), "P=%+d", (int)TCXO_PPM); // %+d includes the sign
//		snprintf(sysPPM, sizeof(sysPPM), "P=%+d", (int)SYS_PPM);
		// Send the formatted string to the LCD
//		LCD_Set_Cursor(0,0);
//		LCD_Send_String(systemClock);
		LCD_Set_Cursor(0,0);
		LCD_Send_String("                ");
		LCD_Set_Cursor(0,0);
		LCD_Send_String(tcxoClock);
		LCD_Set_Cursor(0,6);
//		LCD_Send_String(sysDiff);
		LCD_Send_String(accTime);
		LCD_Set_Cursor(0,12);
		LCD_Send_String(GPSClock);

		print_PPM(TCXO_PPM, SYS_PPM);

//		LCD_Set_Cursor(1,0);
//		LCD_Send_String("                ");
//		LCD_Set_Cursor(1,0);
//		LCD_Send_String(sysPPM);
//		LCD_Set_Cursor(1,7);
//		LCD_Send_String(tcxoPPM);
		}
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
