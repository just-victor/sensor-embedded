//
// Created by Victor on 19.12.2021.
//

#ifndef ANEMOMETR_SIM800_H
#define ANEMOMETR_SIM800_H

#include "main.h"
#include <string.h>

void SIM800_Init(UART_HandleTypeDef *sim800_uart);
uint8_t sendATCommand(const char* str);
uint8_t isOk();
uint8_t* getResponse();
uint8_t isError();
uint8_t* getError();
uint8_t sendGETRequest(const char* url);

#endif //ANEMOMETR_SIM800_H
