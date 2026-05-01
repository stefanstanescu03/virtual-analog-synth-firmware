/*
 * graphics.c
 *
 *  Created on: Apr 4, 2026
 *      Author: Stefan
 */

#include "graphics.h"

void draw_pixel(uint8_t x, uint8_t y, uint8_t color, struct ssd1306_t *display) {

	if (x >= WIDTH || y >= HEIGHT)
		return;

	uint16_t index = WIDTH * (y / 8) + x;

	display->pages_modified |= (1 << (y / 8));

	if (color) {
		display->frame_buffer[index] |= (1 << (y % 8));
	} else {
		display->frame_buffer[index] &= ~(1 << (y % 8));
	}

}

void draw_line(uint8_t m1, uint8_t m2, uint8_t b, struct ssd1306_t *display) {

	uint8_t y;
	for (uint8_t x = 0; x < WIDTH; x++) {
		y = x * m1 / m2 + b;
		if (x < WIDTH && x >= 0 && y < HEIGHT && y >= 0) {
			draw_pixel(x, y, 1, display);
		}
	}

}

void draw_circle(uint8_t c_x, uint8_t c_y, uint8_t r, struct ssd1306_t *display) {

	uint8_t x, y;
	uint8_t y_abs;
	for (x = (c_x - r); x <= (c_x + r); x++) {

		y_abs = sqrt(r * r - (x - c_x) * (x - c_x));

		y = c_y + y_abs;
		if (x < WIDTH && x >= 0 && y < HEIGHT && y >= 0) {
			draw_pixel(x, y, 1, display);
		}

		y = c_y - y_abs;
		if (x < WIDTH && x >= 0 && y < HEIGHT && y >= 0) {
			draw_pixel(x, y, 1, display);
		}
	}

}

void draw_line_p_p(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		struct ssd1306_t *display) {
	int16_t dx = abs(x1 - x0);
	int16_t dy = -abs(y1 - y0);
	int16_t sx = (x0 < x1) ? 1 : -1;
	int16_t sy = (y0 < y1) ? 1 : -1;
	int16_t err = dx + dy;

	while (1) {
		if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT)
			draw_pixel(x0, y0, 1, display);

		if (x0 == x1 && y0 == y1)
			break;

		int16_t e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}
}

void draw_shape(int16_t *vertices, int16_t size, struct ssd1306_t *display) {

	int16_t x0, x1, y0, y1;

	for (int16_t i = 0; i < size - 3; i += 2) {
		x0 = vertices[i];
		y0 = vertices[i + 1];
		x1 = vertices[i + 2];
		y1 = vertices[i + 3];
		draw_line_p_p(x0, y0, x1, y1, display);
	}

	x0 = vertices[size - 2];
	y0 = vertices[size - 1];
	x1 = vertices[0];
	y1 = vertices[1];
	draw_line_p_p(x0, y0, x1, y1, display);
}

void draw_char(int16_t x, int16_t y, char c, uint16_t font_size,
		struct ssd1306_t *display) {

	if (c < 32 || c > 126)
		return;

	uint8_t index = c - 32;

	for (uint8_t col = 0; col < 5; col++) {
		uint8_t byte = FONT[index][col];
		for (uint8_t row = 0; row < 8; row++) {
			if (byte & (1 << row)) {
				if (font_size == 1) {
					draw_pixel(x + col, y + row, 1, display);
				} else {
					draw_pixel(x + 2 * col, y + 2 * row, 1, display);
					draw_pixel(x + 2 * col + 1, y + 2 * row, 1, display);
					draw_pixel(x + 2 * col + 1, y + 2 * row - 1, 1, display);
					draw_pixel(x + 2 * col, y + 2 * row - 1, 1, display);
				}
			}
		}
	}

}

void write_string(int16_t x, int16_t y, char *str, uint16_t font_size,
		struct ssd1306_t *display) {

	for (uint16_t i = 0; str[i]; i++) {
		draw_char(x, y, str[i], font_size, display);
		x += 6 * font_size;
		if (x > WIDTH - 6 * font_size)
			break;
	}
}

void draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, struct ssd1306_t* display) {
    for (uint8_t i = 0; i < h; i++) {
        for (uint8_t j = 0; j < w; j++) {
            if (bitmap[(i / 8) * w + j] & (1 << (i % 8))) {
                draw_pixel(x + j, y + i, 1, display);
            }
        }
    }
}

