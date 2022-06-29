//
// Created by Victor on 19.12.2021.
//

#include <usart.h>
#include <stdio.h>
#include <stdlib.h>
#include "sim800.h"
#include "main.h"
#include "printLCD.h"

#define RxBuf_SIZE 256
#define MainBuf_SIZE 2048

#define API_HOST_PATH "http://jwind-sensor-api.herokuapp.com/"
#define STATE_URL "sensors/" UUID "/state"

const uint8_t ATTEMPTS_50 = 200;
const char *SENSOR_API_HOST = API_HOST_PATH;

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
uint8_t initGPRS();
uint8_t closeGPRS();
uint8_t initHTTP();

uint16_t parseResponse(uint16_t responseIndex);

uint16_t waitAtResponse(const char *string);

uint8_t setUrl(const char *url);

uint8_t sendHTTPData(const uint16_t bodySize);

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
//  printlnUStr("ERROR:");
//  printlnUStr((char*) getError());
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
  printUStr(str);
  uint8_t i = strlen(str);
  HAL_UART_Transmit(sim800_uart, (uint8_t*) str , i , 100);
}
uint8_t executeATCommand() {
  HAL_UART_Transmit(sim800_uart, (uint8_t*) "\r\n" , 2 , 10);

  await = 1;
  err = 0;
  int i = 0;

  HAL_UARTEx_ReceiveToIdle_DMA(sim800_uart, RxBuf, RxBuf_SIZE);
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

  while (await) {
    if (i++ >= ATTEMPTS_50) {
      printlnUStr("ERROR BY TIMEOUT");
      return 0;
    }
    HAL_Delay(50);
  }

  if (isOk()) {
    printlnUStr("OK");
  }

  return isOk();
}

uint8_t prepareConnection(const char* url) {
  if (!initGPRS()) {
    return 0;
  }

  if (!initHTTP()) {
    return 0;
  }

  if (!setUrl(url)) {
    return 0;
  }

  return 1;
}

uint8_t sendState() {
  sendPostRequest(STATE_URL, "{}");

  printlnUStr(getResponse());

  termConnection();
}

uint8_t sendPostRequest(const char* url, const char* body) {
  uint8_t isOk = prepareConnection(url);
  if (!isOk) {
    termConnection();
    return 0;
  }
  sendATCommand("AT+HTTPPARA=CONTENT,application/json");
  sendHTTPData(strlen(body));

  sendATCommand(body);

  isOk = sendATCommand("AT+HTTPACTION=1");
  if (!isOk) {
    termConnection();
    return 0;
  }

  uint16_t responseIndex = waitAtResponse("+HTTPACTION: ");
  if (responseIndex == 0 ) {
    printlnUStr("responseIndex 0");
    termConnection();
    return 0;
  }

  return sendATCommand("AT+HTTPREAD");
}

uint8_t sendHTTPData(const uint16_t bodySize) {
  memset(MainBuf, 0, MainBuf_SIZE);
  memset(RxBuf, 0, RxBuf_SIZE);

  char string[10];
  itoa(bodySize, string, 10);

  partialATCommand("AT+HTTPDATA=");
  partialATCommand(string);
  partialATCommand(",1000");

  HAL_UART_Transmit(sim800_uart, (uint8_t*) "\r\n" , 2 , 10);
  uint8_t isOk = waitAtResponse("DOWNLOAD");
  printlnUStr("OK");
  return isOk;
}

uint8_t sendGETRequest(const char* url) {
  uint8_t isOk = prepareConnection(url);
  if (!isOk) {
    termConnection();
    return 0;
  }

  isOk = sendATCommand("AT+HTTPACTION=0");
  if (!isOk) {
    termConnection();
    return 0;
  }

  uint16_t responseIndex = waitAtResponse("+HTTPACTION: ");
  if (responseIndex == 0 ) {
    printlnUStr("responseIndex 0");
    termConnection();
    return 0;
  }

  return sendATCommand("AT+HTTPREAD");
}

uint8_t setUrl(const char *url) {
  memset(MainBuf, 0, MainBuf_SIZE);

  partialATCommand("AT+HTTPPARA=\"URL\",\"");
  partialATCommand(SENSOR_API_HOST);
  partialATCommand(url);
  partialATCommand("\"");
  printUStr(": ");

  return executeATCommand();
}

uint16_t waitAtResponse(const char *command) {
  uint8_t len = strlen(command);
  uint8_t flag = 0;
  uint attempts = 0;
  while (attempts < ATTEMPTS_50) {
    printUStr("AT ");
    HAL_Delay(50);
    uint16_t j = 0;
    for (uint16_t i = 0; i < RxBuf_SIZE; ++i) {
      for (; j < len; ++j) {
        if (RxBuf[i + j] != command[j]) {
          flag = 0;
          break;
        } else {
          flag = 1;
        }
      }
      if (flag == 1) {
        return i + j;
      }
    }
    attempts++;
  }
  return 0;
}

uint8_t initGPRS() {
  return sendATCommand("AT+SAPBR=1,1");
}

uint8_t closeGPRS() {
  return sendATCommand("AT+SAPBR=0,1");
}

uint8_t initHTTP() {
  return sendATCommand("AT+HTTPINIT");
}

uint8_t termHTTP() {
  return sendATCommand("AT+HTTPTERM");
}

uint8_t termConnection() {
  uint8_t res;
  res = termHTTP();
  res = closeGPRS() & res;
  return res;
}

uint8_t sendATCommand(const char* str) {
  memset(MainBuf, 0, MainBuf_SIZE);
  memset(RxBuf, 0, RxBuf_SIZE);

  partialATCommand(str);

  printUStr(": ");
  return executeATCommand();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  if (huart->Instance != USART2) {
    return;
  }
//  printlnUStr("UART CALLBACK START");

  oldPos = newPos;
//  printUStr("RX: ");
//  printlnUStr(RxBuf);
  err = parseError("+CME ERROR: ", RxBuf);
  if (err) {
    await = 0;
    return;
  }

  if (oldPos + Size > MainBuf_SIZE) {
    oldPos = 0;
    await = 0;
//    printlnUStr("OutOfMemory");
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
//  printlnUStr("UART CALLBACK END");
}
