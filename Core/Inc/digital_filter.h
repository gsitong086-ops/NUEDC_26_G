#ifndef __DIGITAL_FILTER_H
#define __DIGITAL_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define DIGITAL_FILTER_INPUT_RATE_HZ    4000000.0f
#define DIGITAL_FILTER_OUTPUT_RATE_HZ   2000000.0f
#define DIGITAL_FILTER_INPUT_SIZE       8192U
#define DIGITAL_FILTER_OUTPUT_SIZE      4096U
#define DIGITAL_FILTER_TAP_COUNT        64U

/* 4 MSPS low-pass FIR followed by 2:1 decimation. */
uint8_t DigitalFilter_ProcessDecimate(const uint16_t *input,
                                      uint32_t input_count,
                                      uint16_t *output,
                                      uint32_t output_count);

#ifdef __cplusplus
}
#endif

#endif /* __DIGITAL_FILTER_H */
