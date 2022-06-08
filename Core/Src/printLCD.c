#include "printLCD.h"
#include "ssd1306.h"
#include "fonts.h"

FontDef_t* MAIN_FONT;

uint8_t curRow = 0;
uint8_t curPos = 0;
uint8_t maxRow = 0;
uint8_t maxPos = 0;

void initPrintLCD(FontDef_t* font) {
  SSD1306_Init();
  SSD1306_Clear();
  MAIN_FONT = font;
  maxRow = 64 / MAIN_FONT->FontHeight;
  maxPos = 128 / MAIN_FONT->FontWidth;
}

void printlnStr(char* str) {
  if (curRow + 1 > maxRow) {
    curRow = 0;
    SSD1306_Clear();
  }
  SSD1306_GotoXY(curPos * MAIN_FONT->FontWidth,curRow * MAIN_FONT->FontHeight);
  SSD1306_Puts(str, MAIN_FONT, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();

  ++curRow;
  curPos = 0;
}

void printlnInt(int num) {
  char str[8];
  itoa(num, str, 10);
  printlnStr(str);
}

void setCharToDisplay(char c) {
  if (curPos + 1 > maxPos ) {
    curRow++;
    curPos = 0;
    if (curRow + 1 > maxRow) {
      curRow = 0;
      SSD1306_Clear();
    }
  }

  SSD1306_GotoXY(curPos * MAIN_FONT->FontWidth, curRow * (MAIN_FONT->FontHeight));
  SSD1306_Putc(c, MAIN_FONT, SSD1306_COLOR_WHITE);

  curPos++;
}

void printStr(char* str) {
  while (*str) {
    setCharToDisplay(*str);
//    if (!(*str == '\r' || *str == '\n')) {
//
//    }
    str++;
  }
  SSD1306_UpdateScreen();
}

void clearLCD() {
  curRow = 0;
  curPos = 0;
//  SSD1306_Clear();
}

void printInt(int num) {
  char str[8];
  itoa(num, str, 10);
  printStr(str);
}

