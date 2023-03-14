// Example: Room curve correction loaded from REW EQ filter export

// Channel configuration
dsp_channel_t DSP_Channels[DSP_NUM_CHANNELS] = {
  {
    "Left",             // Channel name
    {1,0},              // Input channel(s) - Left/Right
    0,                  // Gain in dB
    0,                  // Delay in milliseconds
  },
  {
    "Right",
    {0,1},
    0,
    0
  }
};

// Frequency specified filters
filter_def_t FREQ_Filters[] = {  // Channel, Filter type, Center frequency, Q value, Gain
};

// BiQuad specified filters
biquad_def_t BIQUAD_Filters[] = { // Channel, Biquad filter coefficients (b0, b1, b2, a1, a2)
};