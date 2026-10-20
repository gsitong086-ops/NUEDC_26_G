#ifndef __FFT_ANALYSIS_H
#define __FFT_ANALYSIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define FFT_ANALYSIS_RAW_SAMPLE_COUNT  8192U
#define FFT_ANALYSIS_SIZE              4096U
#define FFT_ANALYSIS_INPUT_RATE_HZ     4000000.0f
#define FFT_ANALYSIS_RATE_HZ           2000000.0f
#define FFT_ANALYSIS_BIN_HZ            (FFT_ANALYSIS_RATE_HZ / FFT_ANALYSIS_SIZE)

/* FFT幅值 → 幅值(mV) 换算系数 (注意: 显示的是幅值, 不是Vpp)
   理论值: 2 × 3300 / (4096 × 1024) ≈ 0.001574
   已补偿 Hann 窗栅栏效应, 实测校准系数 0.00483 → 0.00476 (基波60mV/谐波40mV/20mV三点校准) */
#define FFT_MAG_TO_VPP_MV(mag)      ((mag) * 0.00476f)

typedef struct
{
    float fundamental_hz;
    float harmonic1_hz;          /* 2次谐波频率 */
    float harmonic2_hz;          /* 3次谐波频率 */
    float frequency_resolution_hz;
    float peak_magnitude;        /* 基波幅值 */
    float harmonic1_magnitude;   /* 2次谐波幅值 */
    float harmonic2_magnitude;   /* 3次谐波幅值 */
    uint16_t peak_bin;           /* 基波bin */
    uint16_t harmonic1_bin;      /* 2次谐波bin */
    uint16_t harmonic2_bin;      /* 3次谐波bin */
    float    rms_mv;             /* 真有效值 (mV) */
    float    upp_mv;             /* 总波形峰峰值 (mV, FFT分量重构 φ=0, 抗噪) */
    uint8_t  valid;
} FFT_AnalysisResult;

extern volatile FFT_AnalysisResult g_fft_result;

/** Initialize the 4096-point CMSIS-DSP real FFT and Hann window. */
uint8_t FFT_Analysis_Init(void);

/** Analyze 8192 samples captured at 4 MSPS and find the fundamental. */
uint8_t FFT_Analysis_Run(const uint16_t *samples,
                         uint32_t sample_count,
                         float maximum_frequency_hz);

/** Analyze 4096 digitally filtered samples at 2 MSPS. */
uint8_t FFT_Analysis_RunFiltered(const uint16_t *samples,
                                 uint32_t sample_count,
                                 float maximum_frequency_hz);

/** Reconstruct waveform from FFT components (φ=0) via IFFT and compute total Upp.
 *  Must be called after a successful FFT_Analysis_Run. */
void FFT_ComputeUpp(void);

/** Compute true RMS from compensated FFT component amplitudes.
 *  Must be called after FilterComp_ApplyToResult (if compensation is used). */
void FFT_ComputeRMS(void);

/** Compute Upp directly from time-domain samples (preserves true phase).
 *  Call after FFT_Analysis_Run to override the FFT-reconstructed upp_mv. */
void FFT_ComputeUppFromSamples(const uint16_t *samples,
                               uint32_t sample_count,
                               uint32_t stride);

/** Get pointer to FFT magnitude array (for spectrum display). */
const float* FFT_GetMagnitudeArray(void);

/** Run a true FFT/IFFT reconstruction of the latest filtered frame. */
uint8_t FFT_ReconstructIFFT(const uint16_t *samples, uint32_t sample_count);

/** Run FFT/IFFT on the latest digitally filtered frame. */
uint8_t FFT_ReconstructIFFTFiltered(const uint16_t *samples,
                                    uint32_t sample_count);

/** Get the latest true IFFT reconstruction (centered ADC codes). */
const float* FFT_GetIFFTWaveform(uint32_t *sample_count);

/* ════════════════════════════════════════════════════════════
   切比雪夫低通滤波器幅值补偿 (硬件前端 7th-order Chebyshev LPF)
   ════════════════════════════════════════════════════════════ */

/**
 * @brief 初始化切比雪夫补偿模型
 * @param cutoff_hz  通带边界频率 (Hz), 典型 1500000.0
 * @param ripple_db  通带纹波 (dB), 典型 0.1 / 0.5 / 1.0
 * @param order      阶数, 典型 7
 */
void FilterComp_Init(float cutoff_hz, float ripple_db, uint8_t order);

/**
 * @brief 获取频率 f 处的幅值补偿因子 (1/|H(f)|)
 * @return 补偿因子 >= 1.0, 上限 100x (40dB)
 */
float FilterComp_GetFactor(float freq_hz);

/**
 * @brief 直接对 g_fft_result 中的基波+谐波幅值进行补偿
 *        在 FFT_Analysis_Run() 之后、FFT_ComputeUpp() 之前调用
 */
void FilterComp_ApplyToResult(void);

#ifdef __cplusplus
}
#endif

#endif /* __FFT_ANALYSIS_H */
