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
#define AD9226_D8_Pin GPIO_PIN_8
#define AD9226_D8_GPIO_Port GPIOD
#define AD9226_D9_Pin GPIO_PIN_9
#define AD9226_D9_GPIO_Port GPIOD
#define AD9226_D10_Pin GPIO_PIN_10
#define AD9226_D10_GPIO_Port GPIOD
#define AD9226_D11_Pin GPIO_PIN_11
#define AD9226_D11_GPIO_Port GPIOD
#define AD9226_D0_Pin GPIO_PIN_0
#define AD9226_D0_GPIO_Port GPIOD
#define AD9226_D1_Pin GPIO_PIN_1
#define AD9226_D1_GPIO_Port GPIOD
#define AD9226_D2_Pin GPIO_PIN_2
#define AD9226_D2_GPIO_Port GPIOD
#define AD9226_D3_Pin GPIO_PIN_3
#define AD9226_D3_GPIO_Port GPIOD
#define AD9226_D4_Pin GPIO_PIN_4
#define AD9226_D4_GPIO_Port GPIOD
#define AD9226_D5_Pin GPIO_PIN_5
#define AD9226_D5_GPIO_Port GPIOD
#define AD9226_D6_Pin GPIO_PIN_6
#define AD9226_D6_GPIO_Port GPIOD
#define AD9226_D7_Pin GPIO_PIN_7
#define AD9226_D7_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */
#define ADC_SAMPLE_LEN    8192U   // 4MSPS下采集2.048ms，FFT频率间隔488.28125Hz
#define ADC_DATA_PORT     GPIOD   // AD9226数据端口
#define ADC_DATA_MASK     0x0FFF  // 12位掩码

// FireWater协议：数据用逗号分隔，换行(\n)结束

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
