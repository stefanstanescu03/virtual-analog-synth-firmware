/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sound.h"
#include "user_interface.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim10;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM10_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
volatile const char shapes[3] = { 't', 's', 'c' };
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile struct buffer buf;

uint8_t trigger_env;

float a = 0.04f;
float k = 1.0f;
float sweep = 0.8f;
float sweep_rate = 0.9994f;
float decay_rate = 0.9997f;
float attack_rate = 0.015f;

uint16_t bpm = 120;
uint16_t shape_index;
uint8_t sub = 0;

uint16_t adc_buf[5] = { 0 };
volatile uint8_t process_flag = 0;

volatile uint16_t sequence[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, };

volatile uint16_t note_index[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1 };

volatile int16_t octaves[16] =
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

volatile int16_t slides[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

volatile int16_t accents[16] =
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };

volatile mode device_mode = PROG;
volatile uint32_t sample_counter = 0;
volatile uint32_t last_interrupt_time = 0;
volatile uint16_t curr = 0;
volatile uint32_t overrun_count = 0;

volatile HAL_StatusTypeDef status;

volatile int16_t seq_cursor = 0;
volatile int16_t seq_page = 0;
volatile int16_t active_pages = 1;

volatile int16_t options_cursor = 0;

volatile menu menu_state = OPTIONS;

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
	__set_FPSCR(__get_FPSCR() | (1 << 24) | (1 << 25));

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_SPI1_Init();
	MX_TIM2_Init();
	MX_ADC1_Init();
	MX_TIM3_Init();
	MX_TIM10_Init();
	MX_I2C1_Init();
	/* USER CODE BEGIN 2 */

	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
	HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 2, 0);

	struct ssd1306_t display;
	display.hi2cx = &hi2c1;
	HAL_Delay(100);
	init_display(&display);

	HAL_TIM_Base_Start(&htim3);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adc_buf, 5);

	trigger_env = 1;
	init_buffer(&buf, 110.0f);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
	HAL_TIM_Base_Start_IT(&htim2);

	buf.slide = 0.02f;

	uint32_t samples_per_step = (SAMPLE_RATE * 60) / (bpm * 4);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		if (device_mode == PLAY) {
			if (process_flag) {
				process_flag = 0;

				a = 0.02f + 0.2 * ((float) adc_buf[0] / 4095.0f);
				k = 0.0f + 10.0f * ((float) adc_buf[1] / 4095.0f);
				sweep = 0.0f + 1.0 * ((float) adc_buf[2] / 4095.0f);
				sweep_rate = 0.999f + 0.0009 * ((float) adc_buf[3] / 4095.0f);

				samples_per_step = (SAMPLE_RATE * 60) / (bpm * 4);

				advance_sequencer(&buf, &sample_counter, samples_per_step, note_index,
						octaves, slides, accents, sequence, curr, sweep, active_pages);

				compute_next(&buf, trigger_env, sweep, sweep_rate, sweep_rate,
						attack_rate, shapes[shape_index], sub);

				filter(&buf, a + buf.sweep, k);

				if (hspi1.State == HAL_SPI_STATE_READY) {
					send_to_dac(&hspi1, GPIOA, GPIO_PIN_6, buf.curr_sample);
				}
			}
		} else {
			if (menu_state == SEQUENCER) {
				for (int i = 15; i >= 0; i--) {
					if (sequence[i] != 0) {
						active_pages = (i / 4) + 1;
						break;
					}
				}

				display_sequencer(&display, note_index, sequence, octaves,
						slides, accents, seq_cursor, seq_page, active_pages);
			} else if (menu_state == OPTIONS) {

				bpm = (uint16_t) (150 - 90 * ((float) adc_buf[4] / 4095.0f));

				display_options(&display, bpm, shapes[shape_index], sub,
						options_cursor);
			}
			HAL_Delay(100);

		}

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 100;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */

	/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = ENABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
	hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 5;
	hadc1.Init.DMAContinuousRequests = ENABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		Error_Handler();
	}

	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_9;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}

	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_4;
	sConfig.Rank = 2;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}

	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_3;
	sConfig.Rank = 3;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}

	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_2;
	sConfig.Rank = 4;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}

	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = 5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */

}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 100000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

	/* USER CODE BEGIN SPI1_Init 0 */

	/* USER CODE END SPI1_Init 0 */

	/* USER CODE BEGIN SPI1_Init 1 */

	/* USER CODE END SPI1_Init 1 */
	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void) {

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 4535;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */

}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void) {

	/* USER CODE BEGIN TIM3_Init 0 */

	/* USER CODE END TIM3_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM3_Init 1 */

	/* USER CODE END TIM3_Init 1 */
	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 9999;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 999;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM3_Init 2 */

	/* USER CODE END TIM3_Init 2 */

}

/**
 * @brief TIM10 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM10_Init(void) {

	/* USER CODE BEGIN TIM10_Init 0 */

	/* USER CODE END TIM10_Init 0 */

	/* USER CODE BEGIN TIM10_Init 1 */

	/* USER CODE END TIM10_Init 1 */
	htim10.Instance = TIM10;
	htim10.Init.Prescaler = 10000 - 1;
	htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim10.Init.Period = 65535;
	htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim10) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM10_Init 2 */

	/* USER CODE END TIM10_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

	/* DMA controller clock enable */
	__HAL_RCC_DMA2_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA2_Stream0_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
	/* DMA2_Stream2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

	/*Configure GPIO pin : PA6 */
	GPIO_InitStruct.Pin = GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : JS_MID_Pin JS_UP_Pin JS_DOWN_Pin JS_LEFT_Pin
	 JS_RIGHT_Pin OCTAVE_BTN_Pin ACCENT_SLIDE_BTN_Pin */
	GPIO_InitStruct.Pin = JS_MID_Pin | JS_UP_Pin | JS_DOWN_Pin | JS_LEFT_Pin
			| JS_RIGHT_Pin | OCTAVE_BTN_Pin | ACCENT_SLIDE_BTN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : STATE_CHANGE_BTN_Pin */
	GPIO_InitStruct.Pin = STATE_CHANGE_BTN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(STATE_CHANGE_BTN_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim == &htim2) {
		if (process_flag == 1) {
			overrun_count++;
		}
		process_flag = 1;
	}
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi == &hspi1) {
		GPIOA->BSRR = GPIO_PIN_6;
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == STATE_CHANGE_BTN_Pin) {

		uint32_t current_time = HAL_GetTick();

		if (current_time - last_interrupt_time > 200) {
			if (device_mode == PROG) {
				device_mode = PLAY;
			} else {
				trigger_env = 1;
				init_buffer(&buf, 110.0f);
				sample_counter = 0;
				curr = 0;
				device_mode = PROG;
			}
			last_interrupt_time = current_time;
		}
	} else if (GPIO_Pin == JS_LEFT_Pin) {
		uint32_t now = HAL_GetTick();
		if (now - last_interrupt_time > 200) {
			if (menu_state == SEQUENCER) {
				if (seq_cursor == 0 && seq_page == 0)
					menu_state = OPTIONS;
				else {
					if (seq_cursor == 0) {
						seq_cursor = 3;
						if (seq_page == 0)
							seq_page = 3;
						else
							seq_page--;
					} else {
						seq_cursor--;
					}
				}
			}
			if (menu_state == OPTIONS) {
				options_cursor = (options_cursor == 0) ? 1 : 0;
			}
			last_interrupt_time = now;
		}
	} else if (GPIO_Pin == JS_RIGHT_Pin) {
		uint32_t now = HAL_GetTick();
		if (now - last_interrupt_time > 200) {
			if (menu_state == SEQUENCER) {
				if (seq_cursor == 3) {
					seq_cursor = 0;
					seq_page = (seq_page + 1) % 4;
				} else {
					seq_cursor++;
				}
			}
			if (menu_state == OPTIONS) {
				if (options_cursor == 1)
					menu_state = SEQUENCER;
				else
					options_cursor = 1;
			}
			last_interrupt_time = now;
		}
	} else if (GPIO_Pin == JS_UP_Pin) {
		uint32_t now = HAL_GetTick();
		if (now - last_interrupt_time > 200) {
			if (menu_state == SEQUENCER)
				note_index[seq_cursor + 4 * seq_page] = (note_index[seq_cursor
						+ 4 * seq_page] + 1) % 12;
			if (menu_state == OPTIONS) {
				if (options_cursor == 0)
					shape_index = (shape_index + 1) % 3;
				else
					sub = (sub == 0) ? 1 : 0;
			}
			last_interrupt_time = now;
		}
	} else if (GPIO_Pin == JS_DOWN_Pin) {
		uint32_t now = HAL_GetTick();
		if (now - last_interrupt_time > 200) {
			if (menu_state == SEQUENCER) {
				uint16_t i = seq_cursor + 4 * seq_page;
				if (note_index[i] == 0) {
					note_index[i] = 11;
				} else {
					note_index[i]--;
				}
			}
			if (menu_state == OPTIONS) {
				if (options_cursor == 0) {
					if (shape_index == 0) {
						shape_index = 2;
					} else {
						shape_index--;
					}
				} else
					sub = (sub == 0) ? 1 : 0;
			}
			last_interrupt_time = now;
		}
	} else if (GPIO_Pin == JS_MID_Pin) {
		uint32_t now = HAL_GetTick();
		if (now - last_interrupt_time > 200) {
			sequence[seq_cursor + 4 * seq_page] = !sequence[seq_cursor
					+ 4 * seq_page];
			last_interrupt_time = now;
		}
	} else if (GPIO_Pin == OCTAVE_BTN_Pin) {
		uint32_t now = HAL_GetTick();
		if (now - last_interrupt_time > 200) {

			uint16_t i = seq_cursor + 4 * seq_page;

			switch (octaves[i]) {
			case 0:
				octaves[i] = 1;
				break;
			case 1:
				octaves[i] = -1;
				break;
			case -1:
				octaves[i] = 0;
				break;
			default:
				break;

			}
			last_interrupt_time = now;
		}
	} else if (GPIO_Pin == ACCENT_SLIDE_BTN_Pin) {
		uint32_t now = HAL_GetTick();
		if (now - last_interrupt_time > 200) {

			uint16_t i = seq_cursor + 4 * seq_page;

			if (slides[i] == 0 && accents[i] == 0) {
				slides[i] = 1;
			} else if (slides[i] == 1 && accents[i] == 0) {
				accents[i] = 1;
				slides[i] = 0;
			} else if (slides[i] == 0 && accents[i] == 1) {
				accents[i] = 1;
				slides[i] = 1;
			} else if (slides[i] == 1 && accents[i] == 1) {
				accents[i] = 0;
				slides[i] = 0;
			}
			last_interrupt_time = now;
		}
	}
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
