// Example: Left channel speaker with crossover at 120 Hz with -24 dB slope

// Channel configuration
dsp_channel_t DSP_Channels[DSP_NUM_CHANNELS] = {
  {
    "Subwoofer",        // Channel name
    {1,0},              // Input channel(s) - Left/Right
    0,                  // Gain in dB
    0,                  // Delay in milliseconds
  },
  {
    "Bookshelf",
    {1,0},
    0,
    0
  }
};

// Frequency specified filters
filter_def_t FREQ_Filters[] = {  // Channel, Filter type, Center frequency, Q value, Gain
  {0, DSP_FILTER_LOW_PASS, 120, 0.7, 0.0 },
  {0, DSP_FILTER_LOW_PASS, 120, 0.7, 0.0 },
  {1, DSP_FILTER_HIGH_PASS, 120, 0.7, 0.0 },
  {1, DSP_FILTER_HIGH_PASS, 120, 0.7, 0.0 }
 };

// BiQuad specified filters
biquad_def_t BIQUAD_Filters[] = { // Channel, Biquad filter coefficients (b0, b1, b2, a1, a2)
};