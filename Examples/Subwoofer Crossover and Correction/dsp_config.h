// Example: Dual subwoofers mixing left and right channels with -24 dB per octave crossover at 120 Hz and peak filters at 35 and 60 Hz

// Channel configuration
dsp_channel_t DSP_Channels[DSP_NUM_CHANNELS] = {
  {
    "Subwoofer 1",      // Channel name
    {1,1},              // Input channel(s) - Left/Right
    0,                  // Gain in dB
    0,                  // Delay in milliseconds
  },
  {
    "Subwoofer 2",
    {1,1},
    0,
    0
  }
};

// Frequency specified filters
filter_def_t FREQ_Filters[] = {  // Channel, Filter type, Center frequency, Q value, Gain
  {0, DSP_FILTER_LOW_PASS, 120, 0.7, 0.0 },
  {0, DSP_FILTER_LOW_PASS, 120, 0.7, 0.0 },
  {0, DSP_FILTER_PEAK_EQ, 35, 2.0, 4.0 },
  {0, DSP_FILTER_PEAK_EQ, 60, 5.0, -6.0 },

  {1, DSP_FILTER_LOW_PASS, 120, 0.7, 0.0 },
  {1, DSP_FILTER_LOW_PASS, 120, 0.7, 0.0 },
  {1, DSP_FILTER_PEAK_EQ, 35, 2.0, 4.0 },
  {1, DSP_FILTER_PEAK_EQ, 60, 5.0, -6.0 },
 };

// BiQuad specified filters
biquad_def_t BIQUAD_Filters[] = { // Channel, Biquad filter coefficients (b0, b1, b2, a1, a2)
};