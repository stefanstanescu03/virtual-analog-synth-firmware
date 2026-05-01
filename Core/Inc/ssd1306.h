#ifndef INC_SSD1306_H_
#define INC_SSD1306_H_

#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"

static const uint8_t ADDR = 0x3C;
static const uint8_t WIDTH = 128;
static const uint8_t HEIGHT = 32;

struct ssd1306_t {

	I2C_HandleTypeDef* hi2cx;

	uint16_t cmd_size;
	uint16_t data_size;

	uint8_t* cmd;
	uint8_t* data;

	uint8_t frame_buffer[128 * 4];

	uint8_t pages_modified;
};

HAL_StatusTypeDef send(I2C_HandleTypeDef* hi2cx, uint8_t* cmd_buf, uint16_t size);
HAL_StatusTypeDef send_command(struct ssd1306_t* display);
HAL_StatusTypeDef send_data(struct ssd1306_t* display);

HAL_StatusTypeDef init_display(struct ssd1306_t* display);
HAL_StatusTypeDef clear_display(struct ssd1306_t* display);

void clear_framebuffer(struct ssd1306_t* display);

HAL_StatusTypeDef flush(struct ssd1306_t* display);

#endif /* INC_SSD1306_H_ */
