#include "hmi.h"

/**
 * @brief 发送原始字符串 + 淘晶驰协议结束符 0xFF 0xFF 0xFF
 *
 * HAL_UART_Transmit 整包发送 (轮询 TXE, HAL 状态机正常管理)
 * TX 和 RX 使用独立 HAL 状态变量 (gState / RxState), 互不干扰
 */
void HMI_SendRaw(const char *str)
{
    size_t len = strlen(str);
    uint8_t term[] = {0xFF, 0xFF, 0xFF};

    if (len > 0U)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)len, 100);
    }
    HAL_UART_Transmit(&huart1, term, 3, 100);
}

/**
 * @brief 设置文本控件内容，格式: obj.txt="text" + FF FF FF
 */
void HMI_SetText(const char *obj, const char *text)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "%s.txt=\"%s\"", obj, text);
    HMI_SendRaw(buf);
}

/**
 * @brief 往波形控件追加数据点，格式: add obj.id,ch,val + FF FF FF
 *
 * 使用控件名的 id 属性，避免控件置顶/置底后数字 ID 改变。
 */
void HMI_AddWave(const char *wave, uint8_t ch, uint8_t val)
{
    char buf[48];
    snprintf(buf, sizeof(buf), "add %s.id,%d,%d", wave, ch, val);
    HMI_SendRaw(buf);
}

/**
 * @brief 批量追加波形数据 (逐点 add 命令, 无延时连续发送)
 *
 * UART 速率 (115200bps) 为天然节流 ≈ 1.5ms/点
 */
void HMI_AddWaveBatch(const char *wave, uint8_t ch,
                      const uint8_t *data, uint16_t count,
                      const volatile uint8_t *run_flag)
{
    uint16_t i;
    char buf[48];

    if ((data == NULL) || (count == 0U)) { return; }

    for (i = 0U; i < count; i++)
    {
        if ((run_flag != NULL) && (*run_flag == 0U)) { return; }
        snprintf(buf, sizeof(buf), "add %s.id,%d,%d", wave, ch, data[i]);
        HMI_SendRaw(buf);
		 if ((i & 0x0FU) == 0x0FU) { HAL_Delay(22U); }
       
    }
}

void HMI_AddWaveBatchById(uint8_t wave_id, uint8_t ch,
                          const uint8_t *data, uint16_t count,
                          const volatile uint8_t *run_flag)
{
    uint16_t i;
    char buf[40];

    if ((data == NULL) || (count == 0U)) { return; }

    for (i = 0U; i < count; i++)
    {
        if ((run_flag != NULL) && (*run_flag == 0U)) { return; }
        snprintf(buf, sizeof(buf), "add %u,%u,%u",
                 (unsigned int)wave_id,
                 (unsigned int)ch,
                 (unsigned int)data[i]);
        HMI_SendRaw(buf);
        if ((i & 0x0FU) == 0x0FU) { HAL_Delay(60U); }  /* 频谱 ~1s */
    }
}

/**
 * @brief 清除波形控件指定通道，格式: cle obj.id,ch + FF FF FF
 */
void HMI_ClearWave(const char *wave, uint8_t ch)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "cle %s.id,%d", wave, ch);
    HMI_SendRaw(buf);
}

void HMI_ClearWaveById(uint8_t wave_id, uint8_t ch)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "cle %u,%u",
             (unsigned int)wave_id,
             (unsigned int)ch);
    HMI_SendRaw(buf);
}

/**
 * @brief 切换页面，格式: page id + FF FF FF
 */
void HMI_Page(const char *page)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "page %s", page);
    HMI_SendRaw(buf);
}
