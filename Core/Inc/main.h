/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define JS_MID_Pin GPIO_PIN_10
#define JS_MID_GPIO_Port GPIOB
#define JS_MID_EXTI_IRQn EXTI15_10_IRQn
#define JS_UP_Pin GPIO_PIN_12
#define JS_UP_GPIO_Port GPIOB
#define JS_UP_EXTI_IRQn EXTI15_10_IRQn
#define JS_DOWN_Pin GPIO_PIN_13
#define JS_DOWN_GPIO_Port GPIOB
#define JS_DOWN_EXTI_IRQn EXTI15_10_IRQn
#define JS_LEFT_Pin GPIO_PIN_14
#define JS_LEFT_GPIO_Port GPIOB
#define JS_LEFT_EXTI_IRQn EXTI15_10_IRQn
#define JS_RIGHT_Pin GPIO_PIN_15
#define JS_RIGHT_GPIO_Port GPIOB
#define JS_RIGHT_EXTI_IRQn EXTI15_10_IRQn
#define STATE_CHANGE_BTN_Pin GPIO_PIN_8
#define STATE_CHANGE_BTN_GPIO_Port GPIOA
#define STATE_CHANGE_BTN_EXTI_IRQn EXTI9_5_IRQn
#define OCTAVE_BTN_Pin GPIO_PIN_5
#define OCTAVE_BTN_GPIO_Port GPIOB
#define OCTAVE_BTN_EXTI_IRQn EXTI9_5_IRQn
#define ACCENT_SLIDE_BTN_Pin GPIO_PIN_9
#define ACCENT_SLIDE_BTN_GPIO_Port GPIOB
#define ACCENT_SLIDE_BTN_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
