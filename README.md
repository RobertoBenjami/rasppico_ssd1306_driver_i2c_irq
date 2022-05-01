# Raspberry Pico ssd1306 i2c driver

The original source: https://github.com/taburyak/STM32_OLED_SSD1306_HAL_DMA

(I modified this a bit)

The driver can work in 3 modes

## Settings in "ssd1306_defines.h"
- #define SSD1306_I2C_PORT : 0 or 1 (which i2c are you using)
- #define SSD1306_SCL : GPIO number for I2C SCL
- #define SSD1306_SDA : GPIO number for I2C SDA
- #define SSD1306_SPD : i2c speed (100000 or 400000)
- #define SSD1306_ADDRESS : 0x3C
- #define SSD1306_128X64 or SSD1306_128X32 (Your display is 64 or 32 lines)
- #define SSD1306_USE_IRQ : 0 or 1 (not use or use the interrupt mode)
- #define SSD1306_CONTUPDATE : 0 or 1 (display update mode in interrupt mode)

## Without interrupt
(#define SSD1306_USE_IRQ 0, #define SSD1306_CONTUPDATE 0)

The drawing functions work in the screen buffer memory. When all drawing functions are completed, use the "ssd1306_UpdateScreen" function. Thus, the contents of the "screen buffer" are transferred to the display. It will not exit this function until it has completed the transfer.

## With interrupt
(#define SSD1306_USE_IRQ 1, #define SSD1306_CONTUPDATE 0)

The drawing functions work in the screen buffer memory. When all drawing functions are completed, use the "ssd1306_UpdateScreen" function. Thus, the contents of the "screen buffer" are transferred to the display. The Update function only starts the transfer, the transfer is done with IRQ in the background. If you redraw in the screen buffer before the transfer is complete, it is possible that the contents of the modified screen buffer will only be partially transferred to the display. In this case, use the update function again to completely transfer the contents of the current screen buffer to the display. With the update function, it is not necessary to wait for the end of the previous update IRQ transmission, you can use it at any time.
It is possible to query if the IRQ transfer is complete (ssd1306_UpdateScreenCompleted). You can also use a callback function to detect the end of a IRQ transmission (ssd1306_UpdateCompletedCallback).

## Continuous update with interrupt
(#define SSD1306_USE_IRQ 1, #define SSD1306_CONTUPDATE 1)

The drawing functions work in the screen buffer memory, but the contents of the screen buffer are continuously transmitted to the display with IRQ in the background. Therefore, it is not necessary to use the update function (the ssd1306_UpdateScreen macro is empty). If you do not draw for a long time, it is possible to pause continuous IRQ transmission (ssd1306_ContUpdateDisable). If you draw again, you can re-enable continuous IRQ transmission (ssd1306_ContUpdateEnable).
It is possible to request interrupts with the callback function when the IRQ transmission is in a certain area of the display. Use the ssd1306_SetRasterInt function to set which display memory page you want to interrupt. The interrupt function must be named ssd1306_RasterIntCallback.
The 64-line display contains 8 memory pages and the 32-row display contains 4 memory pages (see the ssd1306 chip data sheet).

If anyone has any ideas on how to operate the display in DMA mode, email me. Unfortunately, when writing an 8-bit DMA, it also writes the contents of bits 0..2 of the data to bits 8..10 of the data_cmd register, which incorrectly causes RESTART, STOP, CMD commands (eg 0x01->0x0101, 0x02->0x0202, 0x04->0x0404). Enabling the #define USEDMA definition in the ssd1306.c file will allow you to use DMA, but it will not work properly depending on the image content.