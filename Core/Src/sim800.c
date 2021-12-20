//
// Created by Victor on 19.12.2021.
//

#include <usart.h>
#include <stdio.h>
#include "sim800.h"

#define RxBuf_SIZE 256
#define MainBuf_SIZE 2048
const char *SENSOR_API_HOST = "http://jwind-sensor-api.herokuapp.com/";

uint8_t RxBuf[RxBuf_SIZE];
uint8_t MainBuf[MainBuf_SIZE];

uint16_t oldPos = 0;
uint16_t newPos = 0;
volatile uint8_t await = 0;
volatile uint8_t err = 0;

extern DMA_HandleTypeDef hdma_usart2_rx;
UART_HandleTypeDef *sim800_uart;

uint8_t parseError(char *str, uint8_t* buf);
void printError();
uint8_t establishGPRS();
uint8_t closeGPRS();
uint8_t initHTTP();

void SIM800_Init(UART_HandleTypeDef *uart) {
  sim800_uart = uart;
}

uint8_t parseError(char* str, uint8_t* buf) {
  uint8_t errorIndex = 0;
  while (*str) {
    errorIndex++;
    char c = *buf;
    char c2 = *str;
    if (c < 32) {
      buf++;
      continue;
    }
    if (c2 != c) {
      return 0;
    }
    buf++;
    str++;
  }
  return errorIndex;
}

void printError() {
//  clearLCD();
//  printlnStr("ERROR:");
//  printlnStr((char*) getError());
  HAL_Delay(10000);
}

uint8_t isOk() {
  return !err;
}

uint8_t isError() {
  return err;
}

uint8_t* getError() {
  return &RxBuf[err];
}

uint8_t* getResponse() {
  int i = 0;
  memset(MainBuf, 0, MainBuf_SIZE);
  for (int j = 0; j < RxBuf_SIZE; ++j) {
    if (!RxBuf[j] || (RxBuf[j] == 'O' && RxBuf[j + 1] == 'K')) {
      break;
    }

    if (RxBuf[j] < 32) {
      continue;
    }

    MainBuf[i++] = RxBuf[j];
  }

  return MainBuf;
}

void partialATCommand(const char* str) {
  uint8_t i = strlen(str);
  HAL_UART_Transmit(sim800_uart, (uint8_t*) str , i , 100);
}
uint8_t executeATCommand() {
  HAL_UART_Transmit(sim800_uart, (uint8_t*) "\r\n" , 2 , 10);

  await = 1;
  err = 0;

  HAL_UARTEx_ReceiveToIdle_DMA(sim800_uart, RxBuf, RxBuf_SIZE);
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

  while (await) {
    HAL_Delay(10);
  }

  return isOk();
}

uint8_t sendGETRequest(const char* url) {
  uint8_t res = establishGPRS();

  res = initHTTP();

  memset(MainBuf, 0, MainBuf_SIZE);
  partialATCommand("AT+HTTPPARA=\"URL\",\"");
  partialATCommand(SENSOR_API_HOST);
  partialATCommand(url);
  partialATCommand("\"");
  res = executeATCommand();

  res = sendATCommand("AT+HTTPACTION=0");

  getResponse();

  res = closeGPRS();

  return res;
}

uint8_t establishGPRS() {
  return sendATCommand("AT+SAPBR=1,1");
}

uint8_t closeGPRS() {
  return sendATCommand("AT+SAPBR=0,1");
}
uint8_t initHTTP() {
  return sendATCommand("AT+HTTPINIT");
}

uint8_t sendATCommand(const char* str) {
  memset(MainBuf, 0, MainBuf_SIZE);

  partialATCommand(str);

  return executeATCommand();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  if (huart->Instance != USART2) {
    return;
  }

  oldPos = newPos;

  err = parseError("+CME ERROR: ", RxBuf);
  if (err) {
    await = 0;
    return;
  }

  if (oldPos + Size > MainBuf_SIZE) {
    oldPos = 0;
    await = 0;
//    printlnStr("OutOfMemory");
//    printStr("oldPos ");
//    printlnInt(oldPos);
//    printStr("Size ");
//    printlnInt(Size);
  } else {
    memcpy((uint8_t *)MainBuf + oldPos, RxBuf, Size);
    newPos = Size + oldPos;
  }

  HAL_UARTEx_ReceiveToIdle_DMA(huart, RxBuf, RxBuf_SIZE);
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

  for (int i = 0; i < Size; ++i) {
    if (RxBuf[i] == 'O' && RxBuf[i + 1] == 'K') {
      await = 0;
      err = 0;
    }
  }
}
