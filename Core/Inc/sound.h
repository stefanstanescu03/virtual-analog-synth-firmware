/*
 * sound.h
 *
 *  Created on: Mar 14, 2026
 *      Author: Stefan
 */

#ifndef INC_SOUND_H_
#define INC_SOUND_H_

#include <math.h>
#include "stm32f4xx_hal.h"

#define PI 3.14159265359f
#define OFFS 2.0f
#define AMP 0.5f
#define SAMPLE_RATE 22045.0f
#define VOLTS_TO_DAC 1240.909f

typedef enum { ATTACK, DECAY, SUSTAIN, OFF } env_state;
typedef enum {PROG, PLAY} mode;

static const float notes[12] = { 65.41f, 69.30f, 73.42f, 77.78f, 82.41f, 87.31f,
		92.50f, 98.00f, 103.83f, 110.00f, 116.54f, 123.47f };

struct buffer {
	float curr_sample;
	float phase;
	float freq;
	float current_freq;

	float env_level;

	env_state state;

	float sweep;

	float slide;

	float stage[4];

	float dist_lp;

	float sub_state;


};

void init_buffer(volatile struct buffer* buf, float freq);
void send_to_dac(SPI_HandleTypeDef* hspi, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, float sample);
void compute_next(volatile struct buffer* buf, uint8_t env_start, float sweep,
		float sweep_rate, float decay_rate, float attack_rate, char shape, uint8_t sub);

void filter(volatile struct buffer* buf, float a, float k);

advance_sequencer(volatile struct buffer *buf, uint32_t* sample_counter, uint32_t samples_per_step,
		volatile uint16_t* note_index, volatile uint16_t* octaves, volatile uint16_t* slides,
		volatile uint16_t* accents, volatile uint16_t* sequence, volatile uint16_t curr,
		float sweep, volatile int16_t active_pages);

#endif /* INC_SOUND_H_ */
