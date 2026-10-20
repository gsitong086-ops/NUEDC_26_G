#include "digital_filter.h"

#include "main.h"

/*
 * 64-tap linear-phase low-pass FIR, Fs=4 MHz, Fc=750 kHz.
 * Kaiser window (beta=7.85726), normalized for unity DC gain.
 * Calculated response: about 0.001 dB at 500 kHz and -82.6 dB at 1 MHz.
 */
static const float digital_filter_coeffs[DIGITAL_FILTER_TAP_COUNT] =
{
    -1.5004502139e-05f, -6.0824796408e-05f,
    -2.2572477660e-05f,  1.6105232273e-04f,
     2.5157614235e-04f, -8.7768564295e-05f,
    -6.3188766877e-04f, -4.9746956485e-04f,
     6.7458320056e-04f,  1.5817599099e-03f,
     4.1055375707e-04f, -2.2484079655e-03f,
    -2.8506259083e-03f,  8.3806779992e-04f,
     5.2248520443e-03f,  3.6371675814e-03f,
    -4.4348351217e-03f, -9.4824604373e-03f,
    -2.2719187053e-03f,  1.1613036498e-02f,
     1.3886776820e-02f, -3.8906694899e-03f,
    -2.3365201784e-02f, -1.5853043940e-02f,
     1.9097436343e-02f,  4.1018373017e-02f,
     1.0087840680e-02f, -5.4552196373e-02f,
    -7.2260944459e-02f,  2.4271640364e-02f,
     2.0640633651e-01f,  3.5336477877e-01f,
     3.5336477877e-01f,  2.0640633651e-01f,
     2.4271640364e-02f, -7.2260944459e-02f,
    -5.4552196373e-02f,  1.0087840680e-02f,
     4.1018373017e-02f,  1.9097436343e-02f,
    -1.5853043940e-02f, -2.3365201784e-02f,
    -3.8906694899e-03f,  1.3886776820e-02f,
     1.1613036498e-02f, -2.2719187053e-03f,
    -9.4824604373e-03f, -4.4348351217e-03f,
     3.6371675814e-03f,  5.2248520443e-03f,
     8.3806779992e-04f, -2.8506259083e-03f,
    -2.2484079655e-03f,  4.1055375707e-04f,
     1.5817599099e-03f,  6.7458320056e-04f,
    -4.9746956485e-04f, -6.3188766877e-04f,
    -8.7768564295e-05f,  2.5157614235e-04f,
     1.6105232273e-04f, -2.2572477660e-05f,
    -6.0824796408e-05f, -1.5004502139e-05f
};

uint8_t DigitalFilter_ProcessDecimate(const uint16_t *input,
                                      uint32_t input_count,
                                      uint16_t *output,
                                      uint32_t output_count)
{
    float state[DIGITAL_FILTER_TAP_COUNT] = {0.0f};
    uint32_t state_index = 0U;
    uint32_t produced = 0U;
    uint32_t n;

    if ((input == 0) || (output == 0) ||
        (input_count != DIGITAL_FILTER_INPUT_SIZE) ||
        (output_count < DIGITAL_FILTER_OUTPUT_SIZE))
    {
        return 0U;
    }

    for (n = 0U; n < input_count; n++)
    {
        state[state_index] =
            (float)(input[n] & ADC_DATA_MASK) - 2048.0f;

        if ((n & 1U) != 0U)
        {
            float accumulator = 0.0f;
            uint32_t state_pos = state_index;
            uint32_t tap;
            int32_t output_code;

            for (tap = 0U; tap < DIGITAL_FILTER_TAP_COUNT; tap++)
            {
                accumulator += digital_filter_coeffs[tap] * state[state_pos];
                state_pos = (state_pos == 0U) ?
                            (DIGITAL_FILTER_TAP_COUNT - 1U) :
                            (state_pos - 1U);
            }

            output_code = (int32_t)(accumulator + 2048.5f);

            if (output_code < 0)    { output_code = 0; }
            if (output_code > 4095) { output_code = 4095; }
            output[produced++] = (uint16_t)output_code;
        }

        state_index++;
        if (state_index >= DIGITAL_FILTER_TAP_COUNT)
        {
            state_index = 0U;
        }
    }

    return (produced == DIGITAL_FILTER_OUTPUT_SIZE) ? 1U : 0U;
}
