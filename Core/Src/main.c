/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sim800.h"
#include <stdio.h>
#include "ssd1306.h"
#include "fonts.h"
#include "printLCD.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
RTC_TimeTypeDef clkTime;
RTC_DateTypeDef clkDate;
uint8_t secondFlag = 1;
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
void setTime();
void printTime(const RTC_DateTypeDef *date, const RTC_TimeTypeDef *time);

uint8_t isTimeOk = 0;
uint16_t timers[10];

void printUNum(uint8_t num) {
  HAL_UART_Transmit(&huart1, num, sizeof(num), 100);
};
void printlnUNum(int num) {
  char buffer[20] = {};
  sprintf(buffer, "%d", num);
  printlnUStr((uint8_t *) buffer);
};

void printUStr(uint8_t* str) {
  HAL_UART_Transmit(&huart1, str, strlen(str), 100);
};

void printlnUStr(uint8_t* str) {
  printUStr(str);
  printUStr("\r\n");
};
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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_RTC_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  printlnUStr("START.");
  HAL_RTC_Init(&hrtc);
  HAL_RTCEx_SetSecond_IT(&hrtc);
  SIM800_Init(&huart2);
//  initPrintLCD(&Font_11x18);
  if (sendATCommand("AT")) {
    setTime();
  } else {
    printlnUStr("ERROR");
  }
  printlnUStr("DELAY 5000");
  HAL_Delay(5000);
//  SSD1306_UpdateScreen();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    secondTick();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

void setTime() {
  uint8_t responseSize = sendGETRequest("");
  if (responseSize == 0) {
    termConnection();
    return;
  }
  uint8_t* serverTime = getResponse();

  printlnUStr(serverTime);

  uint8_t i = 13;
  clkDate.Date = atoi(&serverTime[i]);
  clkDate.Month = atoi(&serverTime[i + 3]);
  clkDate.Year = atoi(&serverTime[i + 6]) - 2000;
  clkTime.Hours = atoi(&serverTime[i + 11]);
  clkTime.Minutes = atoi(&serverTime[i + 14]);
  clkTime.Seconds = atoi(&serverTime[i + 17]);
  HAL_RTC_SetTime(&hrtc, &clkTime, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &clkDate, RTC_FORMAT_BIN);
  termConnection();
  isTimeOk = 1;
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
uint8_t timeHasCome(uint8_t timerId, uint8_t minutesDelay) {
  if (timers[timerId] == 0) {
    timers[timerId] = minutesDelay;
    return 1;
  }
  return 0;
}

void decreaseTimers(const RTC_TimeTypeDef *time) {
  if (time->Seconds != 0) {
    return;
  }

  for (int i = 0; i < 10; ++i) {
    if (timers[i] > 0) {
      timers[i] = timers[i] - 1;
    }
  }
}

void tick(RTC_DateTypeDef *date, RTC_TimeTypeDef *time) {
  printTime(date, time);

  if (timeHasCome(1, 1)) {
    if (!isTimeOk) {
      setTime();
    }
  }
}

void printTime(const RTC_DateTypeDef *date, const RTC_TimeTypeDef *time) {
  char str[25] = {0};

  sprintf(str, "%.2x/%.2x/20%.2x", date->Date, date->Month, date->Year);
//  clearLCD();
//  printlnStr(str);
  printUStr(str);
  printUStr(" ");

  sprintf(str, "%.2x:%.2x:%.2x", time->Hours, time->Minutes, time->Seconds);
  printlnUStr(str);

  str[0] = 0;
}

void secondTick() {
  if (secondFlag == 0) {
    return;
  }

  HAL_RTC_GetTime(&hrtc, &clkTime, RTC_FORMAT_BCD);
  HAL_RTC_GetDate(&hrtc, &clkDate, RTC_FORMAT_BCD);
  decreaseTimers(&clkTime);
  tick(&clkDate, &clkTime);

  secondFlag = 0;
}
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
