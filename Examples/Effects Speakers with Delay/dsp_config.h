// Example: Left and Right effects speakers with 100 millisecond delay and high pass filter set at 120 Hz

// Channel configuration
dsp_channel_t DSP_Channels[DSP_NUM_CHANNELS] = {
  {
    "Effects Left",     // Channel name
    {1,0},              // Input channel(s) - Left/Right
    0,                  // Gain in dB
    100,                // Delay in milliseconds
  },
  {
    "Effects Right",
    {0,1},
    0,
    100
  }
};

// Frequency specified filters
filter_def_t FREQ_Filters[] = {  // Channel, Filter type, Center frequency, Q value, Gain
  {0, DSP_FILTER_HIGH_PASS, 120, 0.7, 0.0 },
  {0, DSP_FILTER_HIGH_PASS, 120, 0.7, 0.0 },
  {1, DSP_FILTER_HIGH_PASS, 120, 0.7, 0.0 },
  {1, DSP_FILTER_HIGH_PASS, 120, 0.7, 0.0 },
 };

// BiQuad specified filters
biquad_def_t BIQUAD_Filters[] = { // Channel, Biquad filter coefficients (b0, b1, b2, a1, a2)
};