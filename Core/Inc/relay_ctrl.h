/**
 * @file    relay_ctrl.h
 * @brief   继电器控制 — PE5/PE6 高低电平切换 AD9226 前端通路
 *
 *          PE5/PE6 = LOW  → 继电器常闭: 输入直连 AD9226 (题目1/2)
 *          PE5/PE6 = HIGH → 继电器常开: 输入→1.5MHz硬件LPF→AD9226 (题目3)
 */

#ifndef __RELAY_CTRL_H
#define __RELAY_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** 初始化 PE5/PE6 为推挽输出 (默认低电平 = 直连通路) */
void Relay_Init(void);

/** PE5/PE6 = LOW  → 输入直连 AD9226 (无滤波, 题目1/2模式) */
void Relay_SetBypass(void);

/** PE5/PE6 = HIGH → 输入经 1.5MHz 硬件 LPF → AD9226 (题目3模式) */
void Relay_SetLPF(void);

#ifdef __cplusplus
}
#endif

#endif /* __RELAY_CTRL_H */
