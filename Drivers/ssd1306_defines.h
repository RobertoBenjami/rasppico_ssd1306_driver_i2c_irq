/*
 * ssd1306_defines.h
 *
 *  Created on: 14/04/2018
 *  Update on: 10/04/2019
 *      Author: Andriy Honcharenko
 *      version: 2
 *
 *  Modify from stm32 to raspberry pico: 30/04/2022
 *      Author: Roberto Benjami
 */

#ifndef SSD1306_DEFINES_H_
#define SSD1306_DEFINES_H_

#define SSD1306_I2C_PORT      1   // I2C port number (0=i2c0, 1:i2c1)
#define SSD1306_SCL           3   // GPIO number for I2C SCL (i2c0: 1, 5, 9, 13, 17, 21, 25, 29, i2c1: 3, 7, 11, 15, 19, 23, 27)
#define SSD1306_SDA           2   // GPIO number for I2C SDA (i2c0: 0, 4, 8, 12, 16, 20, 24, 28, i2c1: 2, 6, 10, 14, 18, 22, 26)
#define SSD1306_SPD      400000   // i2c speed (100000 or 400000)
#define SSD1306_ADDRESS    0x3C   // I2C address display
#define SSD1306_128X64            // SSD1306_128X32 or SSD1306_128X64
#define SSD1306_USE_IRQ       1   // 0: not used I2C interrupt mode, 1: used I2C interrupt mode
#define SSD1306_CONTUPDATE    1   // 0: continue update mode disable, 1: continue update mode enable (only interrupt mode)

#endif /* SSD1306_DEFINES_H_ */
