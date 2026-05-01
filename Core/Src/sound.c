/*
 * sound.c
 *
 *  Created on: Mar 14, 2026
 *      Author: Stefan
 */
#include "sound.h"

void init_buffer(volatile struct buffer *buf, float freq) {

	buf->curr_sample = 0.0f;
	buf->phase = 0.0f;
	buf->freq = freq;
	buf->current_freq = freq;

	buf->env_level = 0.0f;
	buf->state = DECAY;

	buf->sweep = 0.0f;

	buf->slide = 0.0f;

	buf->stage[0] = 0.0f;
	buf->stage[1] = 0.0f;
	buf->stage[2] = 0.0f;
	buf->stage[3] = 0.0f;

	buf->dist_lp = 0.0f;

	buf->sub_state = 0;

}

void send_to_dac(SPI_HandleTypeDef *hspi, GPIO_TypeDef *GPIOx,
		uint16_t GPIO_Pin, float sample) {

	static uint16_t dac_data;

	float val = sample * VOLTS_TO_DAC;
	if (val > 4095.0f)
		val = 4095.0f;
	if (val < 0.0f)
		val = 0.0f;

	dac_data = 0x3000 | ((uint16_t) val & 0x0FFF);

	GPIOx->BSRR = (uint32_t) GPIO_Pin << 16;
	HAL_SPI_Transmit_DMA(hspi, (uint8_t*) &dac_data, 1);
}

void compute_next(volatile struct buffer *buf, uint8_t env_start, float sweep,
		float sweep_rate, float decay_rate, float attack_rate, char shape,
		uint8_t sub) {

	buf->current_freq += (buf->freq - buf->current_freq) * buf->slide;

	buf->phase += 2.0f * PI * buf->current_freq * (1.0f / SAMPLE_RATE);
	if (buf->phase >= 2.0f * PI) {
		buf->phase -= 2.0f * PI;
		buf->sub_state = !buf->sub_state;
	}

	if (buf->state == ATTACK) {
		buf->env_level += attack_rate;
		if (buf->env_level >= 1) {
			buf->env_level = 1.0f;
			buf->state = DECAY;
		}
	}
	if (buf->state == DECAY) {
		buf->env_level *= decay_rate;
	}

	buf->sweep *= sweep_rate;

	float normalized_phase = buf->phase / (2.0f * PI);

	float raw_sig = 0.0f;
	if (shape == 't') {
		raw_sig = (normalized_phase * 2.0f) - 1.0f;
	} else if (shape == 's') {
		raw_sig = (normalized_phase < 0.5f) ? 1.0f : -1.0f;
	} else if (shape == 'c') {
		float sawtooth = (normalized_phase * 2.0f) - 1.0f;
		float sq_phase = normalized_phase + 0.25f;
		if (sq_phase >= 1.0f)
			sq_phase -= 1.0f;
		float square = (sq_phase < 0.5f) ? 1.0f : -1.0f;
		raw_sig = 0.5f * (sawtooth + square);
	}

	if (sub) {
		float sub_osc = buf->sub_state ? 1.0f : -1.0f;
		raw_sig = 0.7f * raw_sig + 0.3f * sub_osc;
	}

	buf->curr_sample = OFFS + (AMP * buf->env_level * raw_sig);
}

void filter(volatile struct buffer *buf, float a, float k) {

	a = fmaxf(0.0f, fminf(0.5f, a));

	float s0 = buf->stage[0];
	float s1 = buf->stage[1];
	float s2 = buf->stage[2];
	float hp_out = buf->stage[3];
	float inv_a = 1.0f - a;

	float x = (buf->curr_sample - OFFS) / AMP;

	float fb_raw = k * s2;

	static float last_fb_in = 0;
	hp_out = 0.97f * (hp_out + fb_raw - last_fb_in);
	last_fb_in = fb_raw;
	buf->stage[3] = hp_out;

	float compensation = 1.0f + (k * 0.5f);
	float input_with_fb = (x * compensation) - hp_out;

	input_with_fb = fmaxf(-1.0f, fminf(1.0f, input_with_fb));
	input_with_fb = input_with_fb
			* (1.5f - 0.5f * input_with_fb * input_with_fb);

	s0 = (a * input_with_fb) + (inv_a * s0);
	s1 = (a * s0) + (inv_a * s1);
	s2 = (a * s1) + (inv_a * s2);

	buf->stage[0] = s0;
	buf->stage[1] = s1;
	buf->stage[2] = s2;

	buf->curr_sample = OFFS + (AMP * buf->stage[2]);
}

advance_sequencer(volatile struct buffer *buf, uint32_t* sample_counter, uint32_t samples_per_step,
		volatile uint16_t* note_index, volatile uint16_t* octaves, volatile uint16_t* slides,
		volatile uint16_t* accents, volatile uint16_t* sequence, volatile uint16_t curr,
		float sweep, volatile int16_t active_pages) {
	*sample_counter++;
	if (*sample_counter >= samples_per_step) {
		*sample_counter = 0;

		if (octaves[curr] == 1) {
			buf->freq = 2.0f * notes[note_index[curr]] * 1.016f;
		} else if (octaves[curr] == -1) {
			buf->freq = 0.5f * notes[note_index[curr]] * 1.016f;
		} else {
			buf->freq = notes[note_index[curr]] * 1.016f;
		}

		if (slides[curr]) {
			buf->slide = 0.001f;
		} else {
			buf->slide = 0.05f;
		}

		if (accents[curr]) {
			sweep *= 2.0f;
		}

		if (sequence[curr]) {
			buf->state = ATTACK;
			buf->env_level = 0.0f;
			buf->sweep = sweep;
		}

		curr = (curr + 1) % (4 * active_pages);
	}
}
