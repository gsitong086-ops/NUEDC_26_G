#include "fft_analysis.h"

#include "arm_math.h"

#define FFT_MIN_FREQUENCY_HZ       9000.0f
#define FFT_PEAK_RELATIVE_LIMIT    0.05f
#define HARMONIC_MIN_RATIO         0.005f  /* 谐波最小幅值/基波幅值 (0.5%) */
#define TWO_PI_F                   6.28318530717958647692f

static arm_rfft_fast_instance_f32 fft_instance;
static float fft_window[FFT_ANALYSIS_SIZE];
static float fft_input[FFT_ANALYSIS_SIZE];
static float fft_output[FFT_ANALYSIS_SIZE];
static float fft_magnitude[(FFT_ANALYSIS_SIZE / 2U) + 1U];
static uint8_t fft_initialized = 0U;
static uint8_t ifft_valid = 0U;

volatile FFT_AnalysisResult g_fft_result = {0};

static uint32_t FFT_FrequencyToBin(float frequency_hz)
{
    uint32_t bin = (uint32_t)(frequency_hz / FFT_ANALYSIS_BIN_HZ);

    if (((float)bin * FFT_ANALYSIS_BIN_HZ) < frequency_hz)
    {
        bin++;
    }
    return bin;
}

uint8_t FFT_Analysis_Init(void)
{
    uint32_t i;

    if (arm_rfft_fast_init_f32(&fft_instance, FFT_ANALYSIS_SIZE) != ARM_MATH_SUCCESS)
    {
        fft_initialized = 0U;
        return 0U;
    }

    for (i = 0U; i < FFT_ANALYSIS_SIZE; i++)
    {
        float phase = TWO_PI_F * (float)i / (float)(FFT_ANALYSIS_SIZE - 1U);
        fft_window[i] = 0.5f - 0.5f * arm_cos_f32(phase);
    }

    g_fft_result.fundamental_hz = 0.0f;
    g_fft_result.harmonic1_hz = 0.0f;
    g_fft_result.harmonic2_hz = 0.0f;
    g_fft_result.frequency_resolution_hz = FFT_ANALYSIS_BIN_HZ;
    g_fft_result.peak_magnitude = 0.0f;
    g_fft_result.harmonic1_magnitude = 0.0f;
    g_fft_result.harmonic2_magnitude = 0.0f;
    g_fft_result.peak_bin = 0U;
    g_fft_result.harmonic1_bin = 0U;
    g_fft_result.harmonic2_bin = 0U;
    g_fft_result.rms_mv = 0.0f;
    g_fft_result.valid = 0U;
    ifft_valid = 0U;
    fft_initialized = 1U;
    return 1U;
}

static uint8_t FFT_Analysis_RunInternal(const uint16_t *samples,
                                                uint32_t sample_count,
                                                uint32_t sample_stride,
                                                float maximum_frequency_hz)
{
    uint32_t i;
    uint32_t first_bin;
    uint32_t last_bin;
    uint32_t strongest_bin;
    uint32_t fundamental_bin = 0U;
    float mean = 0.0f;
    float strongest_magnitude = 0.0f;
    float delta = 0.0f;

    g_fft_result.valid = 0U;
    ifft_valid = 0U;

    if ((fft_initialized == 0U) || (samples == NULL) ||
        (sample_count < (FFT_ANALYSIS_SIZE * sample_stride)))
    {
        return 0U;
    }

    /* Step 1: 隔点取 4096 点 @ 2MSPS → 去直流 + 汉宁窗 → FFT */
    for (i = 0U; i < FFT_ANALYSIS_SIZE; i++)
    {
        mean += (float)(samples[i * sample_stride] & ADC_DATA_MASK);
    }
    mean /= (float)FFT_ANALYSIS_SIZE;

    for (i = 0U; i < FFT_ANALYSIS_SIZE; i++)
    {
        float centered = (float)(samples[i * sample_stride] & ADC_DATA_MASK) - mean;
        fft_input[i] = centered * fft_window[i];
    }

    arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0U);

    fft_magnitude[0] = 0.0f;
    for (i = 1U; i < (FFT_ANALYSIS_SIZE / 2U); i++)
    {
        float real = fft_output[i * 2U];
        float imag = fft_output[i * 2U + 1U];
        arm_sqrt_f32(real * real + imag * imag, &fft_magnitude[i]);
    }
    fft_magnitude[FFT_ANALYSIS_SIZE / 2U] = 0.0f;

    first_bin = FFT_FrequencyToBin(FFT_MIN_FREQUENCY_HZ);
    last_bin = (uint32_t)(maximum_frequency_hz / FFT_ANALYSIS_BIN_HZ);
    if (last_bin >= (FFT_ANALYSIS_SIZE / 2U))
    {
        last_bin = (FFT_ANALYSIS_SIZE / 2U) - 1U;
    }
    if ((first_bin < 1U) || (last_bin <= first_bin + 1U))
    {
        return 0U;
    }

    strongest_bin = first_bin;
    for (i = first_bin; i <= last_bin; i++)
    {
        if (fft_magnitude[i] > strongest_magnitude)
        {
            strongest_magnitude = fft_magnitude[i];
            strongest_bin = i;
        }
    }

    if (strongest_magnitude <= 0.0f)
    {
        return 0U;
    }

    /* 直接用最强峰作为基波, 避免弱信号下噪声峰误触发 */
    fundamental_bin = strongest_bin;

    if ((fundamental_bin > first_bin) && (fundamental_bin < last_bin))
    {
        float left = fft_magnitude[fundamental_bin - 1U];
        float center = fft_magnitude[fundamental_bin];
        float right = fft_magnitude[fundamental_bin + 1U];
        float denominator = left - 2.0f * center + right;

        if ((denominator > 0.000001f) || (denominator < -0.000001f))
        {
            delta = 0.5f * (left - right) / denominator;
            if (delta > 0.5f)       { delta = 0.5f; }
            else if (delta < -0.5f) { delta = -0.5f; }
        }
    }

    g_fft_result.fundamental_hz =
        ((float)fundamental_bin + delta) * FFT_ANALYSIS_BIN_HZ;
    g_fft_result.frequency_resolution_hz = FFT_ANALYSIS_BIN_HZ;
    /* 补偿 Hann 窗栅栏效应: correction ≈ (1-δ²)/sinc(δ) ≈ 1+0.71×δ² */
    g_fft_result.peak_magnitude =
        fft_magnitude[fundamental_bin] * (1.0f + 0.71f * delta * delta);
    g_fft_result.peak_bin = (uint16_t)fundamental_bin;

    /* ════════════════════════════════════════════════════════════
       谐波搜索 — 按 SNR 强度选, 不按序号顺序
       ════════════════════════════════════════════════════════════ */

    /* 候选谐波结构体 */
    #define HARM_CAND_MAX  5U  /* 最多候选数量 (orders 2..6) */
    {
        float fundamental_magnitude = g_fft_result.peak_magnitude;
        float harmonic_min_mag = fundamental_magnitude * HARMONIC_MIN_RATIO;
        uint32_t order;
        uint32_t cand_count = 0U;

        /* 候选数组 */
        float    c_freq[HARM_CAND_MAX]  = {0.0f};
        float    c_mag[HARM_CAND_MAX]   = {0.0f};
        float    c_snr[HARM_CAND_MAX]   = {0.0f};
        uint16_t c_bin[HARM_CAND_MAX]   = {0U};
        uint8_t  c_order[HARM_CAND_MAX] = {0U};

        /* 初始化结果 */
        g_fft_result.harmonic1_hz = 0.0f;
        g_fft_result.harmonic1_magnitude = 0.0f;
        g_fft_result.harmonic1_bin = 0U;
        g_fft_result.harmonic2_hz = 0.0f;
        g_fft_result.harmonic2_magnitude = 0.0f;
        g_fft_result.harmonic2_bin = 0U;

        /* ── 第一遍: 扫描 orders 2~6, 收集所有候选 ── */
        for (order = 2U; order <= 6U; order++)
        {
            float expected_hz = g_fft_result.fundamental_hz * (float)order;
            uint32_t center_bin, half_span;
            uint32_t win_start, win_end;
            uint32_t peak_bin, j;
            float peak_mag, win_mean;
            float h_delta = 0.0f;

            if (expected_hz > maximum_frequency_hz) { break; }

            center_bin = (uint32_t)(expected_hz / FFT_ANALYSIS_BIN_HZ + 0.5f);
            half_span  = (uint32_t)(expected_hz * 0.15f / FFT_ANALYSIS_BIN_HZ);
            if (half_span < 3U) { half_span = 3U; }

            win_start = (center_bin > half_span) ? (center_bin - half_span) : 1U;
            win_end   = center_bin + half_span;
            if (win_end >= (FFT_ANALYSIS_SIZE / 2U))
            { win_end = (FFT_ANALYSIS_SIZE / 2U) - 1U; }
            if (win_start >= win_end) { continue; }

            /* 找窗内最大峰 + 计算均值 */
            peak_bin = win_start;
            peak_mag = fft_magnitude[win_start];
            {
                float sum = 0.0f;
                uint32_t cnt = 0U;
                for (j = win_start; j <= win_end; j++)
                {
                    if (fft_magnitude[j] > peak_mag)
                    { peak_mag = fft_magnitude[j]; peak_bin = j; }
                    sum += fft_magnitude[j];
                    cnt++;
                }
                win_mean = (cnt > 0U) ? (sum / (float)cnt) : 0.0f;
            }

            /*
             * 过滤噪声: 局部峰/均 > 2.0× 且 峰 > 基波×0.5%
             * 对 2 次谐波额外收紧: 峰/均 > 3.5×, 减少假阳性
             */
            {
                float snr_threshold = (order == 2U) ? 3.5f : 2.0f;
                if ((peak_mag < win_mean * snr_threshold) ||
                    (peak_mag < harmonic_min_mag))
                { continue; }
            }

            /* 抛物线插值精炼频率 (原版公式) */
            if ((peak_bin > win_start) && (peak_bin < win_end))
            {
                float left   = fft_magnitude[peak_bin - 1U];
                float center = fft_magnitude[peak_bin];
                float right  = fft_magnitude[peak_bin + 1U];
                float denominator = left - 2.0f * center + right;

                if ((denominator > 0.000001f) ||
                    (denominator < -0.000001f))
                {
                    h_delta = 0.5f * (left - right) / denominator;
                    if (h_delta > 0.5f)       { h_delta = 0.5f; }
                    else if (h_delta < -0.5f) { h_delta = -0.5f; }
                }
            }

            /* 频率校验: 必须是基波的整数倍 ±15% (原版参数) */
            {
                float detected_hz =
                    ((float)peak_bin + h_delta) * FFT_ANALYSIS_BIN_HZ;
                float order_float = detected_hz / g_fft_result.fundamental_hz;
                uint32_t nearest = (uint32_t)(order_float + 0.5f);
                float err = order_float - (float)nearest;
                if (err < 0.0f) { err = -err; }
                if (err > 0.15f || nearest != order)
                { continue; }
            }

            /* 候选入库 (按 SNR = peak / win_mean 打分) */
            c_freq[cand_count]  = ((float)peak_bin + h_delta) * FFT_ANALYSIS_BIN_HZ;
            c_mag[cand_count]   = peak_mag * (1.0f + 0.71f * h_delta * h_delta);
            c_snr[cand_count]   = (win_mean > 0.0f) ? (peak_mag / win_mean) : 0.0f;
            c_bin[cand_count]   = (uint16_t)peak_bin;
            c_order[cand_count] = (uint8_t)order;
            cand_count++;

            if (cand_count >= HARM_CAND_MAX) { break; }
        }

        /* ── 第二遍: 按谐波次数排序 (低次优先), 同次数再按 SNR 降序 ── */
        if (cand_count > 1U)
        {
            uint32_t a, b;
            for (a = 0U; a < cand_count - 1U; a++)
            {
                for (b = a + 1U; b < cand_count; b++)
                {
                    /* 低次优先; 同次则 SNR 高的优先 */
                    if ((c_order[b] < c_order[a]) ||
                        (c_order[b] == c_order[a] && c_snr[b] > c_snr[a]))
                    {
                        float   ft;
                        uint16_t ut;
                        uint8_t  bt;
                        ft = c_freq[a];  c_freq[a] = c_freq[b];  c_freq[b] = ft;
                        ft = c_mag[a];   c_mag[a]  = c_mag[b];   c_mag[b]  = ft;
                        ft = c_snr[a];   c_snr[a]  = c_snr[b];   c_snr[b]  = ft;
                        ut = c_bin[a];   c_bin[a]  = c_bin[b];   c_bin[b]  = ut;
                        bt = c_order[a]; c_order[a]=c_order[b];  c_order[b]=bt;
                    }
                }
            }
        }

        /* ── 第三遍: 取前两名, 按频率升序存储 (harmonic1 < harmonic2) ── */
        if (cand_count >= 2U)
        {
            uint32_t best_a = 0U;
            uint32_t best_b = 1U;

            if (c_freq[best_a] > c_freq[best_b])
            { best_a = 1U; best_b = 0U; }

            g_fft_result.harmonic1_hz        = c_freq[best_a];
            g_fft_result.harmonic1_magnitude = c_mag[best_a];
            g_fft_result.harmonic1_bin       = c_bin[best_a];

            g_fft_result.harmonic2_hz        = c_freq[best_b];
            g_fft_result.harmonic2_magnitude = c_mag[best_b];
            g_fft_result.harmonic2_bin       = c_bin[best_b];
        }
        else if (cand_count == 1U)
        {
            g_fft_result.harmonic1_hz        = c_freq[0];
            g_fft_result.harmonic1_magnitude = c_mag[0];
            g_fft_result.harmonic1_bin       = c_bin[0];
        }
    }

    /* 过滤幅值 < 5mV 的谐波 (噪声门限) */
    {
        if (FFT_MAG_TO_VPP_MV(g_fft_result.harmonic1_magnitude) < 5.0f)
        {
            g_fft_result.harmonic1_hz        = 0.0f;
            g_fft_result.harmonic1_magnitude = 0.0f;
            g_fft_result.harmonic1_bin       = 0U;
        }
        if (FFT_MAG_TO_VPP_MV(g_fft_result.harmonic2_magnitude) < 5.0f)
        {
            g_fft_result.harmonic2_hz        = 0.0f;
            g_fft_result.harmonic2_magnitude = 0.0f;
            g_fft_result.harmonic2_bin       = 0U;
        }
    }

    g_fft_result.valid = 1U;
    return 1U;
}

uint8_t FFT_Analysis_Run(const uint16_t *samples,
                         uint32_t sample_count,
                         float maximum_frequency_hz)
{
    return FFT_Analysis_RunInternal(samples, sample_count, 2U,
                                    maximum_frequency_hz);
}

uint8_t FFT_Analysis_RunFiltered(const uint16_t *samples,
                                 uint32_t sample_count,
                                 float maximum_frequency_hz)
{
    return FFT_Analysis_RunInternal(samples, sample_count, 1U,
                                    maximum_frequency_hz);
}

const float* FFT_GetMagnitudeArray(void)
{
    return fft_magnitude;
}

const float* FFT_GetIFFTWaveform(uint32_t *sample_count)
{
    if (sample_count != NULL)
    {
        *sample_count = ifft_valid ? FFT_ANALYSIS_SIZE : 0U;
    }
    return ifft_valid ? fft_input : NULL;
}

static uint8_t FFT_ReconstructIFFTInternal(const uint16_t *samples,
                                                   uint32_t sample_count,
                                                   uint32_t sample_stride)
{
    uint32_t i;
    float mean = 0.0f;

    if ((fft_initialized == 0U) || (samples == NULL) ||
        (sample_count < (FFT_ANALYSIS_SIZE * sample_stride)))
    {
        ifft_valid = 0U;
        return 0U;
    }

    for (i = 0U; i < FFT_ANALYSIS_SIZE; i++)
    {
        mean += (float)(samples[i * sample_stride] & ADC_DATA_MASK);
    }
    mean /= (float)FFT_ANALYSIS_SIZE;

    for (i = 0U; i < FFT_ANALYSIS_SIZE; i++)
    {
        fft_input[i] = (float)(samples[i * sample_stride] & ADC_DATA_MASK) - mean;
    }

    arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0U);
    arm_rfft_fast_f32(&fft_instance, fft_output, fft_input, 1U);
    ifft_valid = 1U;
    return 1U;
}

uint8_t FFT_ReconstructIFFT(const uint16_t *samples, uint32_t sample_count)
{
    return FFT_ReconstructIFFTInternal(samples, sample_count, 2U);
}

uint8_t FFT_ReconstructIFFTFiltered(const uint16_t *samples,
                                    uint32_t sample_count)
{
    return FFT_ReconstructIFFTInternal(samples, sample_count, 1U);
}

/**
 * @brief 从 FFT 分量计算真有效值 (使用补偿后的幅值)
 * @note  在 FilterComp_ApplyToResult 之后调用,
 *        直接用补偿后的 g_fft_result 分量值计算
 *        Vrms = sqrt(A0^2 + A1^2 + A2^2) / sqrt(2)
 */
void FFT_ComputeRMS(void)
{
    float sum_sq;

    if (g_fft_result.valid == 0U) { return; }

    {
        float vpp0 = FFT_MAG_TO_VPP_MV(g_fft_result.peak_magnitude);
        sum_sq = vpp0 * vpp0;

        if (g_fft_result.harmonic1_hz > 0.0f)
        {
            float vpp1 = FFT_MAG_TO_VPP_MV(g_fft_result.harmonic1_magnitude);
            sum_sq += vpp1 * vpp1;
        }
        if (g_fft_result.harmonic2_hz > 0.0f)
        {
            float vpp2 = FFT_MAG_TO_VPP_MV(g_fft_result.harmonic2_magnitude);
            sum_sq += vpp2 * vpp2;
        }
    }
    g_fft_result.rms_mv = sqrtf(sum_sq) / 1.4142136f;
}

/**
 * @brief 从 FFT 分量重构波形并计算总 Upp (所有分量初始相位 = 0)
 * @note  在 FFT_Analysis_Run 之后调用, 不影响 FFT 分析流程
 *        在基波的一个完整周期内均匀采样 4000 点重构, 保证谐波整周期对齐
 */
void FFT_ComputeUpp(void)
{
    float amp_mv[3], freq_hz[3];
    uint8_t n = 0U;
    float T, dt, max_v, min_v;
    uint32_t i, j;
    const uint32_t N_RECON = 4000U;

    if (g_fft_result.valid == 0U) { return; }

    /* 收集 FFT 分量 (FFT_MAG_TO_VPP_MV 输出的是幅值, 非 Vpp) */
    amp_mv[n]  = FFT_MAG_TO_VPP_MV(g_fft_result.peak_magnitude);
    freq_hz[n] = g_fft_result.fundamental_hz;
    n++;

    if (g_fft_result.harmonic1_hz > 0.0f)
    {
        amp_mv[n]  = FFT_MAG_TO_VPP_MV(g_fft_result.harmonic1_magnitude);
        freq_hz[n] = g_fft_result.harmonic1_hz;
        n++;
    }
    if (g_fft_result.harmonic2_hz > 0.0f)
    {
        amp_mv[n]  = FFT_MAG_TO_VPP_MV(g_fft_result.harmonic2_magnitude);
        freq_hz[n] = g_fft_result.harmonic2_hz;
        n++;
    }

    /* x̂(t) = Σ ampᵢ·sin(2π·fᵢ·t)   (全部 φ=0)
     * 在基波的一个周期 T 内采样 N_RECON 点, 保证所有谐波正好完成整数周期 */
    T  = 1.0f / freq_hz[0];
    dt = T / (float)N_RECON;
    max_v = -1e9f;
    min_v =  1e9f;

    for (i = 0U; i < N_RECON; i++)
    {
        float t   = (float)i * dt;
        float val = 0.0f;
        for (j = 0U; j < n; j++)
        {
            val += amp_mv[j] * arm_sin_f32(TWO_PI_F * freq_hz[j] * t);
        }
        if (val > max_v) { max_v = val; }
        if (val < min_v) { min_v = val; }
    }

    g_fft_result.upp_mv = (max_v - min_v) * 0.999f;
}

/**
 * @brief 直接从时域采样数据计算峰峰值
 * @note  不依赖FFT重构，保留真实相位，适用于任意波形
 *        ADC 12bit, Vref=3.3V, 中心值=2048
 *        跳过前后各2%的样本以去除边界毛刺
 */
void FFT_ComputeUppFromSamples(const uint16_t *samples,
                               uint32_t sample_count,
                               uint32_t stride)
{
    uint32_t i;
    uint32_t total;
    uint32_t skip;
    uint16_t min_code = 4095U;
    uint16_t max_code = 0U;

    if ((samples == NULL) || (sample_count < 10U) || (stride == 0U)) { return; }

    total = sample_count / stride;
    if (total < 10U) { return; }

    skip = total / 50U;  /* 前后各跳2% */
    if (skip < 1U) { skip = 1U; }

    for (i = skip; i < total - skip; i++)
    {
        uint16_t code = samples[i * stride] & ADC_DATA_MASK;
        if (code > max_code) { max_code = code; }
        if (code < min_code) { min_code = code; }
    }

    /* Upp_mV = (max_code - min_code) × 3300mV / 4096 */
    g_fft_result.upp_mv = (float)(max_code - min_code) * 0.805664f;
}

/* ════════════════════════════════════════════════════════════
   切比雪夫低通滤波器幅值补偿
   ════════════════════════════════════════════════════════════ */

#include <math.h>

/* ── 滤波器参数 ──────────────────────────────────────── */
static float  g_fc          = 1500000.0f;   /* 通带边界频率 Hz */
static float  g_epsilon_sq  = 0.1220f;      /* ε², 默认 0.5dB 纹波 */
static uint8_t g_filt_order = 7U;           /* 滤波器阶数 */
static uint8_t g_filt_init  = 0U;
static float  g_passband_gain = 1.255f;     /* 通带插入损耗补偿 (基波60mV/谐波40mV/20mV三点校准) */

#define COMP_MAX_LINEAR 100.0f   /* 补偿因子上限 40dB */

/**
 * @brief Chebyshev 多项式 T_N(x) — 递推法, 数值稳定
 */
static float chebyshev_Tn(uint8_t n, float x)
{
    float T_prev2, T_prev1, T_curr;
    uint8_t k;

    if (n == 0U) { return 1.0f; }
    if (n == 1U) { return x; }

    T_prev2 = 1.0f;
    T_prev1 = x;

    for (k = 2U; k <= n; k++)
    {
        T_curr  = 2.0f * x * T_prev1 - T_prev2;
        T_prev2 = T_prev1;
        T_prev1 = T_curr;
    }
    return T_curr;
}

/**
 * @brief 初始化切比雪夫滤波器补偿模型
 *
 *        |H(f)| = 1 / √(1 + ε² · T_N²(f/fc))
 *        ε² = 10^(R_dB/10) - 1
 */
void FilterComp_Init(float cutoff_hz, float ripple_db, uint8_t order)
{
    float R_linear;

    if (cutoff_hz > 0.0f) { g_fc = cutoff_hz; }
    if (order > 0U)       { g_filt_order = order; }

    /* ε² = 10^(R_dB/10) - 1  =  exp(R_dB × ln(10)/10) - 1 */
    R_linear = expf(ripple_db * 0.2302585093f);
    if (R_linear > 1.0f)
    {
        g_epsilon_sq = R_linear - 1.0f;
    }
    else
    {
        g_epsilon_sq = 0.0f;   /* 无纹波 → 退化为 Butterworth */
    }

    g_filt_init = 1U;
}

/**
 * @brief 获取频率 freq_hz 处的补偿因子 = 1 / |H(f)|
 */
float FilterComp_GetFactor(float freq_hz)
{
    float x, Tn, H_sq, factor;

    if ((g_filt_init == 0U) || (freq_hz <= 0.0f))
    {
        return 1.0f;
    }

    x  = freq_hz / g_fc;
    Tn = chebyshev_Tn(g_filt_order, x);

    /* |H(f)| = 1 / √(1 + ε²·T_N²) */
    H_sq = 1.0f + g_epsilon_sq * Tn * Tn;
    if (H_sq < 1.0f) { H_sq = 1.0f; }

    factor = sqrtf(H_sq);  /* 补偿因子, ≥ 1.0 */

    if (factor > COMP_MAX_LINEAR)
    {
        factor = COMP_MAX_LINEAR;
    }
    if (factor < 1.0f)
    {
        factor = 1.0f;
    }

    return factor;
}

/**
 * @brief 直接补偿 g_fft_result 中的基波+谐波幅值
 *        补偿 = Chebyshev频率补偿 × 通带插入损耗补偿
 */
void FilterComp_ApplyToResult(void)
{
    float comp;

    if (g_filt_init == 0U) { return; }

    /* 基波 */
    if (g_fft_result.fundamental_hz > 0.0f)
    {
        comp = FilterComp_GetFactor(g_fft_result.fundamental_hz);
        g_fft_result.peak_magnitude *= comp * g_passband_gain;
    }

    /* 2 次谐波 */
    if (g_fft_result.harmonic1_hz > 0.0f)
    {
        comp = FilterComp_GetFactor(g_fft_result.harmonic1_hz);
        g_fft_result.harmonic1_magnitude *= comp * g_passband_gain;
    }

    /* 3 次谐波 */
    if (g_fft_result.harmonic2_hz > 0.0f)
    {
        comp = FilterComp_GetFactor(g_fft_result.harmonic2_hz);
        g_fft_result.harmonic2_magnitude *= comp * g_passband_gain;
    }
}
