#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "ssd1306.h"

static inline uint32_t millis(void)
{
  return to_ms_since_boot(get_absolute_time());
}

void ssd1306_TestBorder()
{
  ssd1306_SetColor(Black);
  ssd1306_Fill();

  uint32_t start = millis();
  uint32_t end = start;
  uint8_t x = 0;
  uint8_t y = 0;
  do
  {
    ssd1306_SetColor(Black);
    ssd1306_DrawPixel(x, y);

    if((y == 0) && (x < 127))
      x++;
    else if((x == 127) && (y < (SSD1306_HEIGHT-1)))
      y++;
    else if((y == (SSD1306_HEIGHT-1)) && (x > 0))
      x--;
    else
      y--;

    ssd1306_SetColor(White);
    ssd1306_DrawPixel(x, y);
    ssd1306_UpdateScreen();

    sleep_ms(5);
    end = millis();
  } while((end - start) < 8000);

  sleep_ms(1000);
}

void ssd1306_TestFonts()
{
  uint8_t y = 0;
  ssd1306_SetColor(Black);
  ssd1306_Fill();

  ssd1306_SetColor(White);
  ssd1306_SetCursor(2, y);
  ssd1306_WriteString("Font 16x26", Font_16x26);
  y += 26;

  ssd1306_SetCursor(2, y);
  ssd1306_WriteString("Font 11x18", Font_11x18);
  y += 18;

  ssd1306_SetCursor(2, y);
  ssd1306_WriteString("Font 7x10", Font_7x10);
  y += 10;

  ssd1306_UpdateScreen();
}

void ssd1306_TestFPS()
{
  ssd1306_SetColor(White);
  ssd1306_Fill();

  uint32_t start = millis();
  uint32_t end = start;
  int fps = 0;
  char message[] = "ABCDEFGHIJK";

  ssd1306_SetCursor(2, 0);
  ssd1306_SetColor(Black);
  ssd1306_WriteString("Testing...", Font_11x18);

  do
  {
    ssd1306_SetCursor(2, 18);
    ssd1306_WriteString(message, Font_11x18);
    ssd1306_UpdateScreen();

    char ch = message[0];
    memmove(message, message + 1, sizeof(message) - 2);
    message[sizeof(message) - 2] = ch;

    fps++;
    end = millis();
  } while((end - start) < 5000);

  sleep_ms(1000);

  char buff[64];
  fps = (float)fps / ((end - start) / 1000.0);
  snprintf(buff, sizeof(buff), "~%d FPS", fps);

  ssd1306_SetColor(White);
  ssd1306_Fill();
  ssd1306_SetCursor(2, 2);
  ssd1306_SetColor(Black);
  ssd1306_WriteString(buff, Font_11x18);
  ssd1306_UpdateScreen();
}

void ssd1306_TestLine()
{
  ssd1306_SetColor(White);
  ssd1306_DrawLine(1, 1, SSD1306_WIDTH - 1, SSD1306_HEIGHT - 1);
  ssd1306_DrawLine(SSD1306_WIDTH - 1, 1, 1, SSD1306_HEIGHT - 1);
  ssd1306_UpdateScreen();
  return;
}

void ssd1306_TestRectangle()
{
  uint32_t delta;

  ssd1306_SetColor(White);
  for(delta = 0; delta < 5; delta ++) {
    ssd1306_DrawRect(1 + (5 * delta), 1 + (5 * delta) ,SSD1306_WIDTH-1 - (5 * delta), SSD1306_HEIGHT-1 - (5 * delta));
  }
  ssd1306_UpdateScreen();
  return;
}

void ssd1306_TestCircle()
{
  uint32_t delta;

  ssd1306_SetColor(White);
  for(delta = 0; delta < 5; delta ++) {
    ssd1306_DrawCircle(20 * delta + 30, 15, 10);
  }
  ssd1306_UpdateScreen();
  return;
}

void ssd1306_TestArc()
{
  ssd1306_SetColor(White);
  ssd1306_DrawArc(30, 30, 30, 20, 270);
  ssd1306_UpdateScreen();
  return;
}

void ssd1306_TestPolyline()
{
  SSD1306_VERTEX loc_vertex[] =
  {
    {35, 40},
    {40, 20},
    {45, 28},
    {50, 10},
    {45, 16},
    {50, 10},
    {53, 16}
  };

  ssd1306_SetColor(White);
  ssd1306_Polyline(loc_vertex, sizeof(loc_vertex) / sizeof(loc_vertex[0]));
  ssd1306_UpdateScreen();
  return;
}

void mainApp(void)
{
  uint8_t orientation = 1;
  ssd1306_Init();

  while(1)
  {
    if(orientation == 1)
    {
      ssd1306_ResetOrientation();
    }
    else if(orientation == 2)
    {
      ssd1306_FlipScreenVertically();
    }
    else if(orientation == 3)
    {
      ssd1306_MirrorScreen();
    }
    else if(orientation == 4)
    {
      ssd1306_MirrorFlipScreen();
      orientation = 0;
    }
    orientation++;

    ssd1306_TestFPS();
    sleep_ms(3000);
    ssd1306_TestBorder();
    ssd1306_TestFonts();
    sleep_ms(3000);
    ssd1306_SetColor(Black);
    ssd1306_Fill();
    ssd1306_TestRectangle();
    ssd1306_TestLine();
    sleep_ms(3000);
    ssd1306_SetColor(Black);
    ssd1306_Fill();
    ssd1306_TestPolyline();
    sleep_ms(3000);
    ssd1306_SetColor(Black);
    ssd1306_Fill();
    ssd1306_TestArc();
    sleep_ms(3000);
    ssd1306_SetColor(Black);
    ssd1306_Fill();
    ssd1306_TestCircle();
    sleep_ms(3000);
  }
}
