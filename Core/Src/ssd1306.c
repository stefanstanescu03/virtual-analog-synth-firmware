#include "ssd1306.h"

HAL_StatusTypeDef send(I2C_HandleTypeDef *hi2cx, uint8_t *cmd_buf,
		uint16_t size) {

	 return HAL_I2C_Master_Transmit(hi2cx, (uint16_t) (ADDR << 1), cmd_buf, size,
			HAL_MAX_DELAY);
}

HAL_StatusTypeDef send_command(struct ssd1306_t *display) {

	uint8_t cmd_buf[display->cmd_size + 1];
	cmd_buf[0] = 0x00;

	memcpy(&cmd_buf[1], display->cmd, display->cmd_size);

	return send(display->hi2cx, cmd_buf, display->cmd_size + 1);
}

HAL_StatusTypeDef send_data(struct ssd1306_t *display) {

	uint8_t cmd_buf[display->data_size + 1];
	cmd_buf[0] = 0x40;

	memcpy(&cmd_buf[1], display->data, display->data_size);

	return send(display->hi2cx, cmd_buf, display->data_size + 1);
}

HAL_StatusTypeDef init_display(struct ssd1306_t *display) {

	static uint8_t init_cmds[27] = { 0xAE, // Display OFF
			0x20, 0x00, // Horizontal addressing
			0xB0, // Page 0 start
			0xC8, // COM scan direction
			0x00, // Low column
			0x10, // High column
			0x40, // Start line
			0x81, 0x7F, // Contrast
			0xA1, // Segment remap
			0xA6, // Normal display
			0xA8, 0x1F, // Multiplex 32
			0xD3, 0x00, // Display offset
			0xD5, 0x80, // Clock divide
			0xD9, 0xF1, // Pre-charge
			0xDA, 0x02, // COM pins for 32 rows
			0xDB, 0x40, // VCOM detect
			0x8D, 0x14, // Charge pump on
			0xAF // Display ON
			};

	display->cmd = init_cmds;
	display->cmd_size = sizeof(init_cmds);

	memset(display->frame_buffer, 0x00, 512);

	return send_command(display);
}

HAL_StatusTypeDef clear_display(struct ssd1306_t *display) {

	uint8_t cmd[3] = { 0xB0, 0x00, 0x10 };
	uint8_t data[128];

	memset(data, 0x00, sizeof(data));

	display->cmd = cmd;
	display->data = data;

	display->cmd_size = 3;
	display->data_size = 128;

	HAL_StatusTypeDef res;

	for (int page = 0; page < 4; page++) {
		display->cmd[0] = 0xB0 + page;
		res = send_command(display);

		if (res != HAL_OK)
			return res;

		res = send_data(display);

		if (res != HAL_OK)
			return res;
	}

	return HAL_OK;
}

void clear_framebuffer(struct ssd1306_t *display) {
	memset(display->frame_buffer, 0x00, 512);
}

HAL_StatusTypeDef flush(struct ssd1306_t *display) {

	HAL_StatusTypeDef res;

	static uint8_t cmd[3] = { 0xB0, 0x00, 0x10 };

	display->cmd = cmd;
	display->cmd_size = 3;

	for (int page = 0; page < 4; page++) {

			display->cmd[0] = 0xB0 + page;
			res = send_command(display);

			if (res != HAL_OK)
				return res;

			display->data = &display->frame_buffer[page * WIDTH];
			display->data_size = WIDTH;
			res = send_data(display);

			if (res != HAL_OK)
				return res;
	}
	return HAL_OK;
}
