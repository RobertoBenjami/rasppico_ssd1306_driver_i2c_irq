#include <string.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <math.h>
#include <hardware/i2c.h>
#include <hardware/irq.h>
#include <hardware/dma.h>

#include "ssd1306_defines.h"
#include "ssd1306.h"

#if SSD1306_I2C_PORT == 0
#undef  SSD1306_I2C_PORT
#define SSD1306_I2C_PORT i2c0
#define SSD1306_DREQ_I2C DREQ_I2C0_TX
#define SSD1306_I2C_IRQ  I2C0_IRQ
#elif SSD1306_I2C_PORT == 1
#undef  SSD1306_I2C_PORT
#define SSD1306_I2C_PORT i2c1
#define SSD1306_DREQ_I2C DREQ_I2C1_TX
#define SSD1306_I2C_IRQ  I2C1_IRQ
#endif

#if SSD1306_USE_IRQ == 0 && SSD1306_CONTUPDATE == 1
#error SSD1306_CONTUPDATE only in IRQ MODE !
#endif

#if SSD1306_USE_IRQ == 1
void ssd1306_i2c_irq_handler(void);
#endif

// Screen object
static SSD1306_t SSD1306;
// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];
// SSD1306 display geometry
SSD1306_Geometry display_geometry = SSD1306_GEOMETRY;

//
//  Get a width and height screen size
//
static const uint16_t width(void)  { return SSD1306_WIDTH; };
static const uint16_t height(void)  { return SSD1306_HEIGHT; };

uint16_t ssd1306_GetWidth(void)
{
  return SSD1306_WIDTH;
}

uint16_t ssd1306_GetHeight(void)
{
  return SSD1306_HEIGHT;
}

SSD1306_COLOR ssd1306_GetColor(void)
{
  return SSD1306.Color;
}

void ssd1306_SetColor(SSD1306_COLOR color)
{
  SSD1306.Color = color;
}

//
//  Fill the whole screen with the given color
//
void ssd1306_Fill(void)
{
  /* Set memory */
  uint32_t i;

  for(i = 0; i < sizeof(SSD1306_Buffer); i++)
  {
    SSD1306_Buffer[i] = (SSD1306.Color == Black) ? 0x00 : 0xFF;
  }
}

//
//  Draw one pixel in the screenbuffer
//  X => X Coordinate
//  Y => Y Coordinate
//  color => Pixel color
//
void ssd1306_DrawPixel(uint8_t x, uint8_t y)
{
  SSD1306_COLOR color = SSD1306.Color;

  if (x >= ssd1306_GetWidth() || y >= ssd1306_GetHeight())
  {
    // Don't write outside the buffer
    return;
  }

  // Check if pixel should be inverted
  if (SSD1306.Inverted)
  {
    color = (SSD1306_COLOR) !color;
  }

  // Draw in the right color
  if (color == White)
  {
    SSD1306_Buffer[x + (y / 8) * width()] |= 1 << (y % 8);
  }
  else
  {
    SSD1306_Buffer[x + (y / 8) * width()] &= ~(1 << (y % 8));
  }
}

void ssd1306_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep)
  {
    SWAP_INT16_T(x0, y0);
    SWAP_INT16_T(x1, y1);
  }

  if (x0 > x1)
  {
    SWAP_INT16_T(x0, x1);
    SWAP_INT16_T(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1)
  {
    ystep = 1;
  }
  else
  {
    ystep = -1;
  }

  for (; x0<=x1; x0++)
  {
    if (steep)
    {
      ssd1306_DrawPixel(y0, x0);
    }
    else
    {
      ssd1306_DrawPixel(x0, y0);
    }
    err -= dy;
    if (err < 0)
    {
      y0 += ystep;
      err += dx;
    }
  }
}

void ssd1306_DrawHorizontalLine(int16_t x, int16_t y, int16_t length)
{
  if (y < 0 || y >= height()) { return; }

  if (x < 0)
  {
    length += x;
    x = 0;
  }

  if ( (x + length) > width())
  {
    length = (width() - x);
  }

  if (length <= 0) { return; }

  uint8_t * bufferPtr = SSD1306_Buffer;
  bufferPtr += (y >> 3) * width();
  bufferPtr += x;

  uint8_t drawBit = 1 << (y & 7);

  switch (SSD1306.Color)
  {
    case White:
      while (length--)
      {
        *bufferPtr++ |= drawBit;
      };
      break;
    case Black:
      drawBit = ~drawBit;
      while (length--)
      {
        *bufferPtr++ &= drawBit;
      };
      break;
    case Inverse:
      while (length--)
      {
        *bufferPtr++ ^= drawBit;
      }; break;
  }
}

void ssd1306_DrawVerticalLine(int16_t x, int16_t y, int16_t length)
{
  if (x < 0 || x >= width()) return;

  if (y < 0)
  {
    length += y;
    y = 0;
  }

  if ( (y + length) > height())
  {
    length = (height() - y);
  }

  if (length <= 0) return;


  uint8_t yOffset = y & 7;
  uint8_t drawBit;
  uint8_t *bufferPtr = SSD1306_Buffer;

  bufferPtr += (y >> 3) * width();
  bufferPtr += x;

  if (yOffset)
  {
    yOffset = 8 - yOffset;
    drawBit = ~(0xFF >> (yOffset));

    if (length < yOffset)
    {
      drawBit &= (0xFF >> (yOffset - length));
    }

    switch (SSD1306.Color)
    {
      case White:   *bufferPtr |=  drawBit; break;
      case Black:   *bufferPtr &= ~drawBit; break;
      case Inverse: *bufferPtr ^=  drawBit; break;
    }

    if (length < yOffset) return;

    length -= yOffset;
    bufferPtr += width();
  }

  if (length >= 8)
  {
    switch (SSD1306.Color)
    {
      case White:
      case Black:
        drawBit = (SSD1306.Color == White) ? 0xFF : 0x00;
        do {
          *bufferPtr = drawBit;
          bufferPtr += width();
          length -= 8;
        } while (length >= 8);
        break;
      case Inverse:
        do {
          *bufferPtr = ~(*bufferPtr);
          bufferPtr += width();
          length -= 8;
        } while (length >= 8);
        break;
    }
  }

  if (length > 0)
  {
    drawBit = (1 << (length & 7)) - 1;
    switch (SSD1306.Color)
    {
      case White:   *bufferPtr |=  drawBit; break;
      case Black:   *bufferPtr &= ~drawBit; break;
      case Inverse: *bufferPtr ^=  drawBit; break;
    }
  }
}

void ssd1306_DrawRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
  ssd1306_DrawHorizontalLine(x, y, width);
  ssd1306_DrawVerticalLine(x, y, height);
  ssd1306_DrawVerticalLine(x + width - 1, y, height);
  ssd1306_DrawHorizontalLine(x, y + height - 1, width);
}

void ssd1306_FillRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height)
{
  for (int16_t x = xMove; x < xMove + width; x++)
  {
    ssd1306_DrawVerticalLine(x, yMove, height);
  }
}

void ssd1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
  /* Draw lines */
  ssd1306_DrawLine(x1, y1, x2, y2);
  ssd1306_DrawLine(x2, y2, x3, y3);
  ssd1306_DrawLine(x3, y3, x1, y1);
}

void ssd1306_DrawFillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
  int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
  yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
  curpixel = 0;

  deltax = abs(x2 - x1);
  deltay = abs(y2 - y1);
  x = x1;
  y = y1;

  if (x2 >= x1)
  {
    xinc1 = 1;
    xinc2 = 1;
  }
  else
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (y2 >= y1)
  {
    yinc1 = 1;
    yinc2 = 1;
  }
  else
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay)
  {
    xinc1 = 0;
    yinc2 = 0;
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;
  }
  else
  {
    xinc2 = 0;
    yinc1 = 0;
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;
  }

  for (curpixel = 0; curpixel <= numpixels; curpixel++)
  {
    ssd1306_DrawLine(x, y, x3, y3);

    num += numadd;
    if (num >= den)
    {
      num -= den;
      x += xinc1;
      y += yinc1;
    }
    x += xinc2;
    y += yinc2;
  }
}

/* Draw polyline */
void ssd1306_Polyline(const SSD1306_VERTEX *par_vertex, uint16_t par_size)
{
  uint16_t i;
  if(par_vertex != 0)
  {
    for(i = 1; i < par_size; i++)
    {
      ssd1306_DrawLine(par_vertex[i - 1].x, par_vertex[i - 1].y, par_vertex[i].x, par_vertex[i].y);
    }
  }
  else
  {
    /*nothing to do*/
  }
  return;
}

/*Convert Degrees to Radians*/
static float ssd1306_DegToRad(float par_deg)
{
  return par_deg * 3.14 / 180.0;
}

/*Normalize degree to [0;360]*/
static uint16_t ssd1306_NormalizeTo0_360(uint16_t par_deg)
{
  uint16_t loc_angle;
  if(par_deg <= 360)
  {
    loc_angle = par_deg;
  }
  else
  {
    loc_angle = par_deg % 360;
    loc_angle = ((par_deg != 0) ? par_deg : 360);
  }
  return loc_angle;
}
/*DrawArc. Draw angle is beginning from 4 quart of trigonometric circle (3pi/2)
 * start_angle in degree
 * sweep in degree
 */
void ssd1306_DrawArc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep)
{
  #define CIRCLE_APPROXIMATION_SEGMENTS 36
  float approx_degree;
  uint32_t approx_segments;
  uint8_t xp1, xp2;
  uint8_t yp1, yp2;
  uint32_t count = 0;
  uint32_t loc_sweep = 0;
  float rad;

  loc_sweep = ssd1306_NormalizeTo0_360(sweep);

  count = (ssd1306_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_degree = loc_sweep / (float)approx_segments;
  while(count < approx_segments)
  {
    rad = ssd1306_DegToRad(count * approx_degree);
    xp1 = x + (int8_t)(sin(rad) * radius);
    yp1 = y + (int8_t)(cos(rad) * radius);
    count++;
    if(count != approx_segments)
    {
      rad = ssd1306_DegToRad(count * approx_degree);
    }
    else
    {
      rad = ssd1306_DegToRad(loc_sweep);
    }
    xp2 = x + (int8_t)(sin(rad) * radius);
    yp2 = y + (int8_t)(cos(rad) * radius);
    ssd1306_DrawLine(xp1, yp1, xp2, yp2);
  }

  return;
}

void ssd1306_DrawCircle(int16_t x0, int16_t y0, int16_t radius)
{
  int16_t x = 0, y = radius;
  int16_t dp = 1 - radius;
  do
  {
    if (dp < 0)
      dp = dp + 2 * (++x) + 3;
    else
      dp = dp + 2 * (++x) - 2 * (--y) + 5;

    ssd1306_DrawPixel(x0 + x, y0 + y);     //For the 8 octants
    ssd1306_DrawPixel(x0 - x, y0 + y);
    ssd1306_DrawPixel(x0 + x, y0 - y);
    ssd1306_DrawPixel(x0 - x, y0 - y);
    ssd1306_DrawPixel(x0 + y, y0 + x);
    ssd1306_DrawPixel(x0 - y, y0 + x);
    ssd1306_DrawPixel(x0 + y, y0 - x);
    ssd1306_DrawPixel(x0 - y, y0 - x);

  } while (x < y);

  ssd1306_DrawPixel(x0 + radius, y0);
  ssd1306_DrawPixel(x0, y0 + radius);
  ssd1306_DrawPixel(x0 - radius, y0);
  ssd1306_DrawPixel(x0, y0 - radius);
}

void ssd1306_FillCircle(int16_t x0, int16_t y0, int16_t radius)
{
  int16_t x = 0, y = radius;
  int16_t dp = 1 - radius;
  do
  {
    if (dp < 0)
    {
      dp = dp + 2 * (++x) + 3;
    }
    else
    {
      dp = dp + 2 * (++x) - 2 * (--y) + 5;
    }

    ssd1306_DrawHorizontalLine(x0 - x, y0 - y, 2*x);
    ssd1306_DrawHorizontalLine(x0 - x, y0 + y, 2*x);
    ssd1306_DrawHorizontalLine(x0 - y, y0 - x, 2*y);
    ssd1306_DrawHorizontalLine(x0 - y, y0 + x, 2*y);


  } while (x < y);
  ssd1306_DrawHorizontalLine(x0 - radius, y0, 2 * radius);
}

void ssd1306_DrawCircleQuads(int16_t x0, int16_t y0, int16_t radius, uint8_t quads)
{
  int16_t x = 0, y = radius;
  int16_t dp = 1 - radius;
  while (x < y)
  {
    if (dp < 0)
      dp = dp + 2 * (++x) + 3;
    else
      dp = dp + 2 * (++x) - 2 * (--y) + 5;
    if (quads & 0x1)
    {
      ssd1306_DrawPixel(x0 + x, y0 - y);
      ssd1306_DrawPixel(x0 + y, y0 - x);
    }
    if (quads & 0x2)
    {
      ssd1306_DrawPixel(x0 - y, y0 - x);
      ssd1306_DrawPixel(x0 - x, y0 - y);
    }
    if (quads & 0x4)
    {
      ssd1306_DrawPixel(x0 - y, y0 + x);
      ssd1306_DrawPixel(x0 - x, y0 + y);
    }
    if (quads & 0x8)
    {
      ssd1306_DrawPixel(x0 + x, y0 + y);
      ssd1306_DrawPixel(x0 + y, y0 + x);
    }
  }
  if (quads & 0x1 && quads & 0x8)
  {
    ssd1306_DrawPixel(x0 + radius, y0);
  }
  if (quads & 0x4 && quads & 0x8)
  {
    ssd1306_DrawPixel(x0, y0 + radius);
  }
  if (quads & 0x2 && quads & 0x4)
  {
    ssd1306_DrawPixel(x0 - radius, y0);
  }
  if (quads & 0x1 && quads & 0x2)
  {
    ssd1306_DrawPixel(x0, y0 - radius);
  }
}

void ssd1306_DrawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress)
{
  uint16_t radius = height / 2;
  uint16_t xRadius = x + radius;
  uint16_t yRadius = y + radius;
  uint16_t doubleRadius = 2 * radius;
  uint16_t innerRadius = radius - 2;

  ssd1306_SetColor(White);
  ssd1306_DrawCircleQuads(xRadius, yRadius, radius, 0b00000110);
  ssd1306_DrawHorizontalLine(xRadius, y, width - doubleRadius + 1);
  ssd1306_DrawHorizontalLine(xRadius, y + height, width - doubleRadius + 1);
  ssd1306_DrawCircleQuads(x + width - radius, yRadius, radius, 0b00001001);

  uint16_t maxProgressWidth = (width - doubleRadius + 1) * progress / 100;

  ssd1306_FillCircle(xRadius, yRadius, innerRadius);
  ssd1306_FillRect(xRadius + 1, y + 2, maxProgressWidth, height - 3);
  ssd1306_FillCircle(xRadius + maxProgressWidth, yRadius, innerRadius);
}

// Draw monochrome bitmap
// input:
//   X, Y - top left corner coordinates of bitmap
//   W, H - width and height of bitmap in pixels
//   pBMP - pointer to array containing bitmap
// note: each '1' bit in the bitmap will be drawn as a pixel
//       each '0' bit in the will not be drawn (transparent bitmap)
// bitmap: one byte per 8 vertical pixels, LSB top, truncate bottom bits
void ssd1306_DrawBitmap(uint8_t X, uint8_t Y, uint8_t W, uint8_t H, const uint8_t* pBMP)
{
  uint8_t pX;
  uint8_t pY;
  uint8_t tmpCh;
  uint8_t bL;

  pY = Y;
  while (pY < Y + H)
  {
    pX = X;
    while (pX < X + W)
    {
      bL = 0;
      tmpCh = *pBMP++;
      if (tmpCh)
      {
        while (bL < 8)
        {
          if (tmpCh & 0x01) ssd1306_DrawPixel(pX,pY + bL);
          tmpCh >>= 1;
          if (tmpCh)
          {
            bL++;
          }
          else
          {
            pX++;
            break;
          }
        }
      }
      else
      {
        pX++;
      }
    }
    pY += 8;
  }
}

char ssd1306_WriteChar(char ch, FontDef Font)
{
  uint32_t i, b, j;

  // Check remaining space on current line
  if (width() <= (SSD1306.CurrentX + Font.FontWidth) ||
    height() <= (SSD1306.CurrentY + Font.FontHeight))
  {
    // Not enough space on current line
    return 0;
  }

  // Use the font to write
  for (i = 0; i < Font.FontHeight; i++)
  {
    b = Font.data[(ch - 32) * Font.FontHeight + i];
    for (j = 0; j < Font.FontWidth; j++)
    {
      if ((b << j) & 0x8000)
      {
        ssd1306_DrawPixel(SSD1306.CurrentX + j, SSD1306.CurrentY + i);
      }
      else
      {
        SSD1306.Color = !SSD1306.Color;
        ssd1306_DrawPixel(SSD1306.CurrentX + j, SSD1306.CurrentY + i);
        SSD1306.Color = !SSD1306.Color;
      }
    }
  }

  // The current space is now taken
  SSD1306.CurrentX += Font.FontWidth;

  // Return written char for validation
  return ch;
}

//
//  Write full string to screenbuffer
//
char ssd1306_WriteString(char* str, FontDef Font)
{
  // Write until null-byte
  while (*str)
  {
    if (ssd1306_WriteChar(*str, Font) != *str)
    {
      // Char could not be written
      return *str;
    }

    // Next char
    str++;
  }

  // Everything ok
  return *str;
}

//
//  Position the cursor
//
void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
  SSD1306.CurrentX = x;
  SSD1306.CurrentY = y;
}

void ssd1306_Clear()
{
  memset(SSD1306_Buffer, 0, SSD1306_BUFFER_SIZE);
}

//=============================================================================
// #define  PINDEBUG
// #define  USEDMA
/* 
Az i2c sajnos nem mukodik dma-val, mert 8 bites dma-t hasznalva a regiszter felso bitjeibe szemet kerul
(olyan, mint ha az also 8 bit megismetlodne a felso 8 bitben) es ez feleslegesen start es stop allapotokat
general mivel a felso bitek ezeket a funkciokat jelentik (0x01 -> 0x0101, 0x02 -> 0x0202, 0x04 -> 0x0404 stb.)
*/

#if SSD1306_USE_IRQ == 1
void ssd1306_i2c_irq_handler(void);
#ifdef USEDMA
int dma_chn;
dma_channel_config dma_conf;
void ssd1306_dma_irq_handler(void);
#endif // #ifdef USEDMA
#endif // #if SSD1306_USE_IRQ == 1

//  Initialize the oled screen
uint8_t ssd1306_Init(void)
{
  #ifdef PINDEBUG
  gpio_init(4);
  gpio_init(5);
  gpio_init(6);
  gpio_init(7);
  gpio_set_dir(4, GPIO_OUT);
  gpio_set_dir(5, GPIO_OUT);
  gpio_set_dir(6, GPIO_OUT);
  gpio_set_dir(7, GPIO_OUT);
  gpio_put(4, 0);
  gpio_put(5, 0);
  gpio_put(6, 0);
  gpio_put(7, 0);
  #endif

  i2c_init(SSD1306_I2C_PORT, SSD1306_SPD);
  gpio_set_function(SSD1306_SDA, GPIO_FUNC_I2C);
  gpio_set_function(SSD1306_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(SSD1306_SDA);
  gpio_pull_up(SSD1306_SCL);

  #if SSD1306_USE_IRQ == 1
  SSD1306_I2C_PORT->hw->intr_mask = 0;
  irq_set_exclusive_handler(SSD1306_I2C_IRQ, ssd1306_i2c_irq_handler);
  irq_set_enabled(SSD1306_I2C_IRQ, true);

  #ifdef USEDMA
  dma_chn = dma_claim_unused_channel(true);
  dma_conf = dma_channel_get_default_config(dma_chn);
  channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_8);
  channel_config_set_dreq(&dma_conf, SSD1306_DREQ_I2C);
  channel_config_set_read_increment(&dma_conf, true);
  channel_config_set_write_increment(&dma_conf, false);

  dma_channel_set_irq0_enabled(dma_chn, true);
  irq_set_exclusive_handler(DMA_IRQ_0, ssd1306_dma_irq_handler);
  irq_set_enabled(DMA_IRQ_0, true);
  #endif // #ifdef USEDMA
  #endif // #if SSD1306_USE_IRQ == 1

  // Wait for the screen to boot
  sleep_ms(100);

  /* Init LCD */
  ssd1306_WriteCommand(DISPLAYOFF);
  ssd1306_WriteCommand(SETDISPLAYCLOCKDIV);
  ssd1306_WriteCommand(0xF0);           // Increase speed of the display max ~96Hz
  ssd1306_WriteCommand(SETMULTIPLEX);
  ssd1306_WriteCommand(height() - 1);
  ssd1306_WriteCommand(SETDISPLAYOFFSET);
  ssd1306_WriteCommand(0x00);
  ssd1306_WriteCommand(SETSTARTLINE);
  ssd1306_WriteCommand(CHARGEPUMP);
  ssd1306_WriteCommand(0x14);
  ssd1306_WriteCommand(MEMORYMODE);
  ssd1306_WriteCommand(0x00);
  ssd1306_WriteCommand(SEGREMAP);
  ssd1306_WriteCommand(COMSCANINC);
  ssd1306_WriteCommand(SETCOMPINS);

  if (display_geometry == GEOMETRY_128_64)
  {
    ssd1306_WriteCommand(0x12);
  }
  else if (display_geometry == GEOMETRY_128_32)
  {
    ssd1306_WriteCommand(0x02);
  }

  ssd1306_WriteCommand(SETCONTRAST);

  if (display_geometry == GEOMETRY_128_64)
  {
    ssd1306_WriteCommand(0xCF);
  }
  else if (display_geometry == GEOMETRY_128_32)
  {
    ssd1306_WriteCommand(0x8F);
  }

  ssd1306_WriteCommand(SETPRECHARGE);
  ssd1306_WriteCommand(0xF1);
  ssd1306_WriteCommand(SETVCOMDETECT);  // 0xDB, (additionally needed to lower the contrast)
  ssd1306_WriteCommand(0x40);           // 0x40 default, to lower the contrast, put 0
  ssd1306_WriteCommand(DISPLAYALLON_RESUME);
  ssd1306_WriteCommand(NORMALDISPLAY);
  ssd1306_WriteCommand(0x2e);           // stop scroll
  ssd1306_WriteCommand(DISPLAYON);

  // Set default values for screen object
  SSD1306.CurrentX = 0;
  SSD1306.CurrentY = 0;
  SSD1306.Color = Black;

  // Clear screen
  ssd1306_Clear();

  // Continuous Update on
  ssd1306_ContUpdateEnable();

  // Flush buffer to screen
  ssd1306_UpdateScreen();

  SSD1306.Initialized = 1;

  /* Return OK */
  return 1;
}

#if SSD1306_USE_IRQ == 0
void i2c_mem_write(uint8_t addr, uint8_t mem_address, const uint8_t *src, size_t len) 
{
  SSD1306_I2C_PORT->hw->enable = 0;
  SSD1306_I2C_PORT->hw->tar = addr;
  SSD1306_I2C_PORT->hw->enable = 1;

  int byte_ctr;
  int ilen = (int)len + 1;

  for (byte_ctr = 0; byte_ctr < ilen; ++byte_ctr) 
  {
    bool last = byte_ctr == ilen - 1;
    if(byte_ctr == 0)
      SSD1306_I2C_PORT->hw->data_cmd = (1 << I2C_IC_DATA_CMD_RESTART_LSB) | mem_address;
    else
      SSD1306_I2C_PORT->hw->data_cmd = bool_to_bit(last) << I2C_IC_DATA_CMD_STOP_LSB | *src++;
    while (!(SSD1306_I2C_PORT->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS));
  }
  while (!(SSD1306_I2C_PORT->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS));
  SSD1306_I2C_PORT->restart_on_next = false;
}

//
//  Send a byte to the command register
//
void ssd1306_WriteCommand(uint8_t command)
{
  uint16_t command16 = command << 8;
  i2c_mem_write(SSD1306_I2C_ADDR, 0, &command, 1);
}

void ssd1306_WriteData(uint8_t* data, uint16_t size)
{
  i2c_mem_write(SSD1306_I2C_ADDR, 0x40, data, size);
}

//
//  Write the screenbuffer with changed to the screen
//
void ssd1306_UpdateScreen(void)
{
  uint8_t i;
  for (i = 0; i < SSD1306_HEIGHT / 8; i++)
  {
    ssd1306_WriteCommand(0xB0 + i);
    ssd1306_WriteCommand(SETLOWCOLUMN);
    ssd1306_WriteCommand(SETHIGHCOLUMN);
    ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], width());
  }
}

#elif SSD1306_USE_IRQ == 1

volatile uint8_t ssd1306_updatestatus = 0;
volatile uint8_t ssd1306_updateend;

volatile struct 
{
  i2c_inst_t *i2c;
  uint8_t    *data;
  uint16_t   len;
  uint8_t    count;
  uint8_t    command;
  uint8_t    mem_address;
}i2c_transactiondata;

void ssd1306_i2c_transaction_end(void);

#ifdef USEDMA
void ssd1306_dma_irq_handler(void)
{
  #ifdef PINDEBUG  
  gpio_put(6, 1);
  #endif
  dma_hw->ints0 = 1u << dma_chn;
  SSD1306_I2C_PORT->hw->dma_cr = 0;
  SSD1306_I2C_PORT->hw->intr_mask = I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS;
  #ifdef PINDEBUG  
  gpio_put(6, 0);
  #endif
}
#endif // #ifdef USEDMA

void ssd1306_i2c_irq_handler(void)
{
  if(i2c_transactiondata.len == 0)
    return;
  #ifdef PINDEBUG  
  gpio_put(5, 1);
  #endif
  if(i2c_transactiondata.len == 1)
  {
    if(i2c_transactiondata.count == 0)
    {
      SSD1306_I2C_PORT->hw->intr_mask = I2C_IC_RAW_INTR_STAT_STOP_DET_BITS;
      SSD1306_I2C_PORT->hw->data_cmd = (1 << I2C_IC_DATA_CMD_STOP_LSB) | i2c_transactiondata.command;
      i2c_transactiondata.count++;
    }
    else
    {
      SSD1306_I2C_PORT->hw->intr_mask = 0;
      i2c_transactiondata.len = 0;
      ssd1306_i2c_transaction_end();
    }  
  }
  else 
  {
    if(i2c_transactiondata.count < i2c_transactiondata.len - 1)
    {
      #ifndef USEDMA
      SSD1306_I2C_PORT->hw->data_cmd = *i2c_transactiondata.data++;
      i2c_transactiondata.count++;
      #else
      SSD1306_I2C_PORT->hw->intr_mask = 0;
      SSD1306_I2C_PORT->hw->dma_cr = I2C_IC_DMA_CR_TDMAE_BITS;
      SSD1306_I2C_PORT->hw->dma_tdlr = 1;
      dma_channel_configure(dma_chn, &dma_conf, &SSD1306_I2C_PORT->hw->data_cmd, i2c_transactiondata.data, i2c_transactiondata.len - 1, true);
      i2c_transactiondata.count += i2c_transactiondata.len - 1;
      i2c_transactiondata.data += i2c_transactiondata.len - 1;
      #endif  
    }
    else if(i2c_transactiondata.count == i2c_transactiondata.len - 1) 
    {
      SSD1306_I2C_PORT->hw->intr_mask = I2C_IC_RAW_INTR_STAT_STOP_DET_BITS;
      SSD1306_I2C_PORT->hw->data_cmd = (1 << I2C_IC_DATA_CMD_STOP_LSB) | *i2c_transactiondata.data;
      i2c_transactiondata.count++;
    }
    else
    {  
      SSD1306_I2C_PORT->hw->intr_mask = 0; 
      i2c_transactiondata.len = 0;
      ssd1306_i2c_transaction_end();
    }
  }
  #ifdef PINDEBUG
  gpio_put(5, 0);
  #endif
}

void i2c_mem_write_irq(uint8_t addr, uint8_t mem_address, uint8_t *data, size_t len)
{
  #ifdef PINDEBUG
  gpio_put(4, 1);
  #endif
  #if SSD1306_CONTUPDATE == 0
  while(i2c_transactiondata.len);
  #endif
  while (!(SSD1306_I2C_PORT->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS));
  i2c_transactiondata.len = len;
  i2c_transactiondata.mem_address = mem_address;
  if(len == 1)
  {
    i2c_transactiondata.data = NULL;
    i2c_transactiondata.command = data[0];
  }  
  else
    i2c_transactiondata.data = data;
  i2c_transactiondata.count = 0;

  SSD1306_I2C_PORT->hw->enable = 0;
  SSD1306_I2C_PORT->hw->tar = addr;
  SSD1306_I2C_PORT->hw->enable = 1;
  SSD1306_I2C_PORT->hw->data_cmd = (1 << I2C_IC_DATA_CMD_RESTART_LSB) | mem_address;
  SSD1306_I2C_PORT->hw->intr_mask = I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS;
  #ifdef PINDEBUG
  gpio_put(4, 0);
  #endif
}

#if SSD1306_CONTUPDATE == 0

void ssd1306_WriteCommand(uint8_t command)
{
  while(ssd1306_updatestatus);
  i2c_mem_write_irq(SSD1306_I2C_ADDR, 0, &command, 1);
}

//
//  Write the screenbuffer with changed to the screen
//
void ssd1306_UpdateScreen(void)
{
  uint8_t  command;
  if(ssd1306_updatestatus == 0)
  { // start
    ssd1306_updatestatus = SSD1306_HEIGHT;
    ssd1306_updateend = SSD1306_HEIGHT + (SSD1306_HEIGHT / 2);
    command = 0xB0;
    i2c_mem_write_irq(SSD1306_I2C_ADDR, 0x00, &command, 1);
  }
  else if(ssd1306_updatestatus >= SSD1306_HEIGHT)
  { 
    ssd1306_updatestatus -= (SSD1306_HEIGHT / 2);
    ssd1306_updateend = (ssd1306_updatestatus + (SSD1306_HEIGHT / 2 + 1)) & 0xFC;
  }
}

char ssd1306_UpdateScreenCompleted(void)
{
  if(ssd1306_updatestatus)
    return 0;
  else
    return 1;
}

void ssd1306_UpdateCompletedCallback(void) { };

void ssd1306_i2c_transaction_end(void)
{
  uint32_t phase;
  uint8_t  command;
  #ifdef PINDEBUG
  gpio_put(7, 1);
  #endif
  if(ssd1306_updatestatus)
  {
    if(ssd1306_updatestatus < ssd1306_updateend)
    {
      ssd1306_updatestatus++;
      phase = ssd1306_updatestatus & 3;
      if(phase == 3)
        i2c_mem_write_irq(SSD1306_I2C_ADDR, 0x40, &SSD1306_Buffer[SSD1306_WIDTH * ((ssd1306_updatestatus >> 2) & (SSD1306_HEIGHT / 8 - 1))], width());
      else
      {
        if(phase == 0)
          command = 0xB0 + ((ssd1306_updatestatus >> 2) & (SSD1306_HEIGHT / 8 - 1));
        else if(phase == 1)
          command = SETLOWCOLUMN;
        else if(phase == 2)
          command = SETHIGHCOLUMN;
        i2c_mem_write_irq(SSD1306_I2C_ADDR, 0x00, &command, 1);
      }
    }
    else
    {
      ssd1306_updatestatus = 0;
      ssd1306_UpdateCompletedCallback();
    }
  }
  #ifdef PINDEBUG
  gpio_put(7, 0);
  #endif
}

#elif SSD1306_CONTUPDATE == 1

volatile uint8_t ssd1306_command = 0;
volatile uint8_t ssd1306_ContUpdate = 0;
volatile uint8_t ssd1306_RasterIntRegs = 0;

//
//  Send a byte to the command register
//
void ssd1306_WriteCommand(uint8_t command)
{
  if(ssd1306_updatestatus)
  {
    while(ssd1306_command);
    ssd1306_command = command;
  }
  else
  {
    while(i2c_transactiondata.len);
    i2c_mem_write_irq(SSD1306_I2C_ADDR, 0, &command, 1);
  }
}

void ssd1306_ContUpdateEnable(void)
{
  uint8_t  command;
  if(!ssd1306_ContUpdate)
  {
    while(i2c_transactiondata.len);
    ssd1306_ContUpdate = 1;
    ssd1306_updatestatus = SSD1306_HEIGHT;
    ssd1306_updateend = SSD1306_HEIGHT + (SSD1306_HEIGHT / 2);
    command = 0xB0;
    i2c_mem_write_irq(SSD1306_I2C_ADDR, 0, &command, 1);
  }
}

void ssd1306_ContUpdateDisable(void)
{
  if(ssd1306_ContUpdate)
  {
    ssd1306_ContUpdate = 0;
  }
}

void ssd1306_i2c_transaction_end(void)
{
  uint32_t phase;
  uint8_t  command;
  uint8_t  raster;
  #ifdef PINDEBUG
  gpio_put(7, 1);
  #endif
  if(ssd1306_updatestatus)
  {
    if(ssd1306_updatestatus < ssd1306_updateend)
    {
      ssd1306_updatestatus++;
      phase = ssd1306_updatestatus & 3;
      if(phase == 3)
      {
        raster = (ssd1306_updatestatus >> 2) & (SSD1306_HEIGHT / 8 - 1);
        if(ssd1306_RasterIntRegs & (1 << raster))
          ssd1306_RasterIntCallback(raster);
        i2c_mem_write_irq(SSD1306_I2C_ADDR, 0x40, &SSD1306_Buffer[SSD1306_WIDTH * ((ssd1306_updatestatus >> 2) & (SSD1306_HEIGHT / 8 - 1))], width());
      }
      else
      {
        if(phase == 0)
          command = 0xB0 + ((ssd1306_updatestatus >> 2) & (SSD1306_HEIGHT / 8 - 1));
        else if(phase == 1)
          command = SETLOWCOLUMN;
        else if(phase == 2)
          command = SETHIGHCOLUMN;
        i2c_mem_write_irq(SSD1306_I2C_ADDR, 0, &command, 1);
      }
    }
    else
    { /* refresh end */
      if(ssd1306_command)
      { /* command ? */
        command = ssd1306_command;
        ssd1306_command = 0;
      }
      else
      { /* refresh restart */
        if(ssd1306_ContUpdate)
        {
          command = 0xB0;
          ssd1306_updatestatus = SSD1306_HEIGHT;
        }
        else
        {
          ssd1306_updatestatus = 0;
        }
      }
      if(ssd1306_updatestatus)
        i2c_mem_write_irq(SSD1306_I2C_ADDR, 0, &command, 1);
    }
  }
  #ifdef PINDEBUG
  gpio_put(7, 0);
  #endif
}

void ssd1306_SetRasterInt(uint8_t r)
{
  ssd1306_RasterIntRegs = r;
}

__attribute__((weak)) void ssd1306_RasterIntCallback(uint8_t r) { }

#endif // #elif SSD1306_CONTUPDATE == 1
#endif // #elif SSD1306_USE_IRQ == 1
