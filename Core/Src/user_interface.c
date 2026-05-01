/*
 * user_interface.cpp
 *
 *  Created on: Apr 7, 2026
 *      Author: Stefan
 */

#include "user_interface.h"

void display_sequencer(struct ssd1306_t *display,
		volatile uint16_t *notes_index, volatile uint16_t *sequence,
		volatile int16_t *octaves, volatile int16_t *slides,
		volatile int16_t *accents, uint16_t seq_cursor,
		volatile uint16_t seq_page, volatile uint16_t active_pages) {

	clear_framebuffer(display);
	for (uint16_t i = 0; i < 4; i++) {

		int x_pos = 15 + (i * 30);

		if (sequence[i + 4 * seq_page]) {
			if (notes_index[i + 4 * seq_page] == 0) {
				draw_char(x_pos, 10, 'C', 2, display);
			} else if (notes_index[i + 4 * seq_page] == 1) {
				write_string(x_pos - 5, 10, "C#", 2, display);
			} else if (notes_index[i + 4 * seq_page] == 2) {
				draw_char(x_pos, 10, 'D', 2, display);
			} else if (notes_index[i + 4 * seq_page] == 3) {
				write_string(x_pos - 5, 10, "D#", 2, display);
			} else if (notes_index[i + 4 * seq_page] == 4) {
				draw_char(x_pos, 10, 'E', 2, display);
			} else if (notes_index[i + 4 * seq_page] == 5) {
				draw_char(x_pos, 10, 'F', 2, display);
			} else if (notes_index[i + 4 * seq_page] == 6) {
				write_string(x_pos - 5, 10, "F#", 2, display);
			} else if (notes_index[i + 4 * seq_page] == 7) {
				draw_char(x_pos, 10, 'G', 2, display);
			} else if (notes_index[i + 4 * seq_page] == 8) {
				write_string(x_pos - 5, 10, "G#", 2, display);
			} else if (notes_index[i + 4 * seq_page] == 9) {
				draw_char(x_pos, 10, 'A', 2, display);
			} else if (notes_index[i + 4 * seq_page] == 10) {
				write_string(x_pos - 5, 10, "A#", 2, display);
			} else if (notes_index[i + 4 * seq_page] == 11) {
				draw_char(x_pos, 10, 'B', 2, display);
			}

		}

		draw_line_p_p(10 + i * 30, 25, 30 + i * 30, 25, display);
	}

	draw_line_p_p(10 + seq_cursor * 30, 30, 30 + seq_cursor * 30, 30, display);
	switch (octaves[seq_cursor + 4 * seq_page]) {
	case 0:
		break;
	case 1:
		write_string(0, 0, "+1 oct", 1, display);
		break;
	case -1:
		write_string(0, 0, "-1 oct", 1, display);
		break;
	default:
		break;
	}
	if (slides[seq_cursor + 4 * seq_page])
		write_string(40, 0, "S", 1, display);
	if (accents[seq_cursor + 4 * seq_page])
		write_string(75, 0, "A", 1, display);

	char page_str[12];
	snprintf(page_str, sizeof(page_str), "%u/%u", seq_page + 1, active_pages);
	write_string(110, 0, page_str, 1, display);

	flush(display);
}

void display_options(struct ssd1306_t *display, uint16_t bpm, char shape, uint8_t sub, volatile int16_t options_cursor) {

	clear_framebuffer(display);

	char bpm_str[5];
	snprintf(bpm_str, sizeof(bpm_str), "%u", bpm);
	write_string(80, 10, bpm_str, 2, display);

	uint16_t padding = (options_cursor == 0) ? 0 : 40;

	draw_line_p_p(2 + padding,  2,  28 + padding, 2,  display);
	draw_line_p_p(28 + padding, 2,  28 + padding, 28, display);
	draw_line_p_p(28 + padding, 28, 2 + padding,  28, display);
	draw_line_p_p(2 + padding,  28, 2 + padding,  2,  display);

	if (shape == 't') {
		draw_line_p_p(5,  25, 25, 5,  display);
		draw_line_p_p(25, 5,  25, 25, display);
	} else if (shape == 's') {
		draw_line_p_p(5,  25, 5,  5,  display);
		draw_line_p_p(5,  5,  15, 5,  display);
		draw_line_p_p(15, 5,  15, 25, display);
		draw_line_p_p(15, 25, 25, 25, display);
	} else if (shape == 'c') {
		draw_line_p_p(5,  13, 25, 5,  display);
		draw_line_p_p(25, 5,  25, 13, display);
		draw_line_p_p(5,  25, 5,  17, display);
		draw_line_p_p(5,  17, 15, 17, display);
		draw_line_p_p(15, 17, 15, 25, display);
		draw_line_p_p(15, 25, 25, 25, display);
	}

	if (sub == 1) {
		draw_line_p_p(4 + 40,  26, 11 + 40, 26, display);
		draw_line_p_p(11 + 40, 26, 11 + 40, 4,  display);
		draw_line_p_p(11 + 40, 4,  19 + 40, 4,  display);
		draw_line_p_p(19 + 40, 4,  19 + 40, 26, display);
		draw_line_p_p(19 + 40, 26, 26 + 40, 26, display);
	} else {
		draw_line_p_p(4 + 40, 16, 26 + 40, 16,  display);
	}

	flush(display);
}
