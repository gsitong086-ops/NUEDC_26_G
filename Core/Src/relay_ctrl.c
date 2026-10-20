/**
 * @file    relay_ctrl.c
 * @brief   PE5/PE6 继电器控制实现
 */

#include "relay_ctrl.h"
#include "main.h"

/**
 * @brief 初始化 PE5/PE6 为推挽输出, 默认拉低 → 直连通路
 */
void Relay_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* 默认: 直连通路 (题目1/2) */
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5 | GPIO_PIN_6, GPIO_PIN_RESET);
}


/**
 * @brief 切到直连通路 (PE5/PE6 = LOW)
 */
void Relay_SetBypass(void)
{
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5 | GPIO_PIN_6, GPIO_PIN_RESET);
}


/**
 * @brief 切到 LPF 通路 (PE5/PE6 = HIGH)
 */
void Relay_SetLPF(void)
{
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5 | GPIO_PIN_6, GPIO_PIN_SET);
}
