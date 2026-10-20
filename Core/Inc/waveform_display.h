#ifndef __WAVEFORM_DISPLAY_H
#define __WAVEFORM_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define WAVEFORM_DISPLAY_MAX_SAMPLES 4096U

/** Enable and initialize the serial-screen waveform control. */
void WaveformDisplay_Init(int horizontal_zoom, int vertical_scale);

/** Apply zoom/scale settings to the screen control immediately (call when zoom_dirty). */
void WaveformDisplay_ApplyZoom(int horizontal_zoom, int vertical_scale);

/** Clear the waveform control (ADC channel only, for redraw). */
void WaveformDisplay_Clear(void);

/** Clear ALL controls — waveform, spectrum, synthesized waveforms,
 *  and reset text controls to "----".
 *  Automatically selects t1_xxx/s8/s7/s17/s16 or t3_xxx/s9/s10/s18/s19
 *  based on the current problem3_mode flag. */
void WaveformDisplay_ClearAll(void);

/** Display a measured frequency as kHz in the selected HMI text control. */
void WaveformDisplay_SetFrequencyHz(const char *text_control, float frequency_hz);

/** Display FFT magnitude as mVpp in the selected HMI text control. */
void WaveformDisplay_SetAmplitudeMV(const char *text_control, float magnitude);

/** Display pre-computed RMS voltage in mV (3 decimal places). */
void WaveformDisplay_SetRMSMV(const char *text_control, float rms_mv);

/** Display reconstructed peak-to-peak voltage in mV (3 decimal places). */
void WaveformDisplay_SetUppMV(const char *text_control, float upp_mv);

/** Draw qualitative FFT spectrum bars on waveform control "7". */
void WaveformDisplay_Spectrum(const volatile uint8_t *run_flag);

/** Draw synthesized waveform from FFT components (φ=0) on sid=16.
 *  Uses exact interpolated frequencies, 512 pts over 1 fundamental period. */
void WaveformDisplay_DrawSynthesized(const volatile uint8_t *run_flag);

/** Draw synthesized waveform on sid=15, 3 fundamental periods. */
void WaveformDisplay_DrawSynthesized3P(const volatile uint8_t *run_flag);

/* ── 题目三 (Problem 3) 显示函数 — 使用独立的 HMI 控件 ────── */

/** Enable and init Problem 3 waveform controls (s9/s10/s18/s19 + t3_xxx). */
void WaveformDisplay_InitP3(int horizontal_zoom, int vertical_scale);

/** Apply zoom to Problem 3 control (s9). */
void WaveformDisplay_ApplyZoomP3(int horizontal_zoom, int vertical_scale);

/** Clear Problem 3 ADC waveform (s9). */
void WaveformDisplay_ClearP3(void);

/** Draw qualitative FFT spectrum bars on P3 control s10. */
void WaveformDisplay_SpectrumP3(const volatile uint8_t *run_flag);

/** Draw raw ADC waveform (s9). */
void WaveformDisplay_DrawP3(const uint16_t *samples,
                             uint32_t sample_count,
                             int horizontal_zoom,
                             int vertical_scale,
                             const volatile uint8_t *run_flag);

/** Draw the true IFFT reconstruction on t3_wave. */
void WaveformDisplay_DrawIFFTP3(int horizontal_zoom,
                                   int vertical_scale,
                                   const volatile uint8_t *run_flag);

/** Draw FFT-synthesized waveform 1-period (s18). */
void WaveformDisplay_DrawSynthesizedP3(const volatile uint8_t *run_flag);

/** Draw FFT-synthesized waveform 3-period (s19). */
void WaveformDisplay_DrawSynthesized3PP3(const volatile uint8_t *run_flag);

/**
 * Draw one frame of raw ADC samples.
 * run_flag may be changed by an interrupt to stop drawing immediately.
 */
void WaveformDisplay_Draw(const uint16_t *samples,
                          uint32_t sample_count,
                          int horizontal_zoom,
                          int vertical_scale,
                          const volatile uint8_t *run_flag);

#ifdef __cplusplus
}
#endif

#endif /* __WAVEFORM_DISPLAY_H */
