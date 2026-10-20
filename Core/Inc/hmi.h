#ifndef __HMI_H
#define __HMI_H

#include "usart.h"
#include "string.h"
#include "stdio.h"

extern UART_HandleTypeDef huart1;

/**
 * @brief 发送原始字符串指令 + 0xFF 0xFF 0xFF 结束符（淘晶驰协议）
 */
void HMI_SendRaw(const char *str);

/**
 * @brief 设置文本控件内容，格式: obj.txt="text" + FFFFFF
 */
void HMI_SetText(const char *obj, const char *text);

/**
 * @brief 往波形控件追加一个数据点，格式: add obj.id,ch,val + FFFFFF
 * @param wave 波形控件名称，如 "t1_wave"
 * @param ch   通道号（0起始）
 * @param val  数值 0~255
 */
void HMI_AddWave(const char *wave, uint8_t ch, uint8_t val);

/**
 * @brief 批量往波形控件追加数据点 (逐点 add 命令, 无延时连续发送)
 * @param wave     波形控件名称，如 "t1_wave"
 * @param ch       通道号 (0起始)
 * @param data     数据字节数组 (0~255)
 * @param count    数据点数
 * @param run_flag 中断标志 (NULL=不检查, 指向 capture_on 则按键可随时中止)
 *
 * UART 速率天然节流 ≈ 1.5ms/点
 */
void HMI_AddWaveBatch(const char *wave, uint8_t ch,
                      const uint8_t *data, uint16_t count,
                      const volatile uint8_t *run_flag);

/** Send waveform points using the numeric component ID required by add. */
void HMI_AddWaveBatchById(uint8_t wave_id, uint8_t ch,
                          const uint8_t *data, uint16_t count,
                          const volatile uint8_t *run_flag);

/**
 * @brief 清除波形控件指定通道
 * @param wave 波形控件名称，如 "t1_wave"
 * @param ch   通道号（0起始）
 */
void HMI_ClearWave(const char *wave, uint8_t ch);

/** Clear a waveform channel using its numeric component ID. */
void HMI_ClearWaveById(uint8_t wave_id, uint8_t ch);

/**
 * @brief 切换页面
 */
void HMI_Page(const char *page);

#endif /* __HMI_H */
