#include <stdio.h>
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "image.h"

void drawLines()
{
  for (int16_t i = 0; i < ssd1306_GetWidth(); i += 4)
  {
    ssd1306_DrawLine(0, 0, i, ssd1306_GetHeight() - 1);
    ssd1306_UpdateScreen();
    sleep_ms(10);
  }
  for (int16_t i = 0; i < ssd1306_GetHeight(); i += 4)
  {
    ssd1306_DrawLine(0, 0, ssd1306_GetWidth() - 1, i);
    ssd1306_UpdateScreen();
    sleep_ms(10);
  }
  sleep_ms(250);

  ssd1306_Clear();
  for (int16_t i = 0; i < ssd1306_GetWidth(); i += 4)
  {
   ssd1306_DrawLine(0, ssd1306_GetHeight() - 1, i, 0);
   ssd1306_UpdateScreen();
   sleep_ms(10);
  }
  for (int16_t i = ssd1306_GetHeight() - 1; i >= 0; i -= 4)
  {
   ssd1306_DrawLine(0, ssd1306_GetHeight() - 1, ssd1306_GetWidth() - 1, i);
   ssd1306_UpdateScreen();
   sleep_ms(10);
  }
  sleep_ms(250);
  ssd1306_Clear();
  for (int16_t i = ssd1306_GetWidth() - 1; i >= 0; i -= 4)
  {
    ssd1306_DrawLine(ssd1306_GetWidth() - 1, ssd1306_GetHeight() - 1, i, 0);
    ssd1306_UpdateScreen();
    sleep_ms(10);
  }
  for (int16_t i = ssd1306_GetHeight() - 1; i >= 0; i -= 4)
  {
    ssd1306_DrawLine(ssd1306_GetWidth() - 1, ssd1306_GetHeight() - 1, 0, i);
    ssd1306_UpdateScreen();
    sleep_ms(10);
  }
  sleep_ms(250);
  ssd1306_Clear();
  for (int16_t i = 0; i < ssd1306_GetHeight(); i += 4)
  {
    ssd1306_DrawLine(ssd1306_GetWidth() - 1, 0, 0, i);
    ssd1306_UpdateScreen();
    sleep_ms(10);
  }
  for (int16_t i = 0; i < ssd1306_GetWidth(); i += 4)
  {
    ssd1306_DrawLine(ssd1306_GetWidth() - 1, 0, i, ssd1306_GetHeight() - 1);
    ssd1306_UpdateScreen();
    sleep_ms(10);
  }
  sleep_ms(250);
}

// Adapted from Adafruit_SSD1306
void drawRect(void)
{
  for (int16_t i = 0; i < ssd1306_GetHeight() / 2; i += 2)
  {
    ssd1306_DrawRect(i, i, ssd1306_GetWidth() - 2 * i, ssd1306_GetHeight() - 2 * i);
    ssd1306_UpdateScreen();
    sleep_ms(10);
  }
}

// Adapted from Adafruit_SSD1306
void fillRect(void) {
  uint8_t color = 1;
  for (int16_t i = 0; i < ssd1306_GetHeight() / 2; i += 3)
  {
    ssd1306_SetColor((color % 2 == 0) ? Black : White); // alternate colors
    ssd1306_FillRect(i, i, ssd1306_GetWidth() - i * 2, ssd1306_GetHeight() - i * 2);
    ssd1306_UpdateScreen();
    sleep_ms(10);
    color++;
  }
  // Reset back to WHITE
  ssd1306_SetColor(White);
}

// Adapted from Adafruit_SSD1306
void drawCircle(void)
{
  for (int16_t i = 0; i < ssd1306_GetHeight(); i += 2)
  {
    ssd1306_DrawCircle(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, i);
    ssd1306_UpdateScreen();
    sleep_ms(10);
  }
  sleep_ms(1000);
  ssd1306_Clear();

  // This will draw the part of the circel in quadrant 1
  // Quadrants are numberd like this:
  //   0010 | 0001
  //  ------|-----
  //   0100 | 1000
  //
  ssd1306_DrawCircleQuads(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, ssd1306_GetHeight() / 4, 0x01 /*0b00000001*/);
  ssd1306_UpdateScreen();
  sleep_ms(200);
  ssd1306_DrawCircleQuads(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, ssd1306_GetHeight() / 4, 0x03 /*0b00000011*/);
  ssd1306_UpdateScreen();
  sleep_ms(200);
  ssd1306_DrawCircleQuads(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, ssd1306_GetHeight() / 4, 0x07 /*0b00000111*/);
  ssd1306_UpdateScreen();
  sleep_ms(200);
  ssd1306_DrawCircleQuads(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, ssd1306_GetHeight() / 4, 0x0F /*0b00001111*/);
  ssd1306_UpdateScreen();
}

void drawProgressBarDemo(int counter)
{
  char str[128];
  // draw the progress bar
  ssd1306_DrawProgressBar(0, 32, 120, 10, counter);

  // draw the percentage as String
  ssd1306_SetCursor(64, 15);
  sprintf(str, "%i%%", counter);
  ssd1306_WriteString(str, Font_7x10);
  ssd1306_UpdateScreen();
}

void drawLines2()
{
  for (int16_t i = 0; i < ssd1306_GetWidth(); i += 4)
    ssd1306_DrawLine(0, 0, i, ssd1306_GetHeight() - 1);
  for (int16_t i = 0; i < ssd1306_GetHeight(); i += 4)
    ssd1306_DrawLine(0, 0, ssd1306_GetWidth() - 1, i);
  ssd1306_UpdateScreen();
} 

void mainApp(void)
{
  ssd1306_Init();
  ssd1306_FlipScreenVertically();
  //ssd1306_Clear();
  ssd1306_SetColor(White);
  sleep_ms(1000);
  while (1)
  {
    drawLines();
    sleep_ms(1000);
    ssd1306_Clear();
  
    drawRect();
    sleep_ms(1000);
    ssd1306_Clear();

    fillRect();
    sleep_ms(1000);
    ssd1306_Clear();

    drawCircle();
    sleep_ms(1000);
    ssd1306_Clear();

    for(int i = 0; i < 100; i++)
    {
      drawProgressBarDemo(i);
      sleep_ms(25);
      while(!ssd1306_UpdateScreenCompleted());
      ssd1306_Clear();
    }

    ssd1306_DrawRect(0, 0, ssd1306_GetWidth(), ssd1306_GetHeight());
    ssd1306_SetCursor(8, 20);
    ssd1306_WriteString("SSD1306", Font_16x26);
    ssd1306_UpdateScreen();
    sleep_ms(2000);
    ssd1306_Clear();
    ssd1306_DrawBitmap(0, 0, 128, 64, stm32fan);
    ssd1306_UpdateScreen();
    sleep_ms(2000);
    ssd1306_InvertDisplay();
    sleep_ms(2000);
    ssd1306_NormalDisplay();
    ssd1306_Clear();
  }
}
