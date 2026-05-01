/*
 * user_interface.h
 *
 *  Created on: Apr 7, 2026
 *      Author: Stefan
 */

#ifndef INC_USER_INTERFACE_H_
#define INC_USER_INTERFACE_H_

#include "graphics.h"

typedef enum { OPTIONS, SEQUENCER } menu;

void display_sequencer(struct ssd1306_t *display,
		volatile uint16_t *notes_index, volatile uint16_t *sequence,
		volatile int16_t *octaves, volatile int16_t *slides,
		volatile int16_t *accents, uint16_t seq_cursor,
		volatile uint16_t seq_page, volatile uint16_t active_pages);

void display_options(struct ssd1306_t *display, uint16_t bpm, char shape, uint8_t sub, volatile int16_t options_cursor);

#endif /* INC_USER_INTERFACE_H_ */
