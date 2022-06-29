//
// Created by Victor on 18.06.2021.
//

#ifndef TEST_PRINTLCD_H
#define TEST_PRINTLCD_H

#include "fonts.h"

void initPrintLCD(FontDef_t* font);

void printlnStr(char* str);

void printlnInt(int num);

void setCharToDisplay(char c);

void printStr(char* str);

void clearLCD();

void printInt(int num);

#endif //TEST_PRINTLCD_H
