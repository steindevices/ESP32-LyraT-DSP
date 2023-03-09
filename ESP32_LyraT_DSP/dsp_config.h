// Channel configuration
dsp_channel_t DSP_Channels[DSP_NUM_CHANNELS] = {
  {
    "Channel A",        // Channel name
    {0,1},              // Input channel(s) - Left/Right
    0,                  // Gain in dB
    0,                  // Delay in milliseconds
  },
  {
    "Channel B",
    {1,0},  
    0,
    0
  }
};

// Frequency specified filters
filter_def_t FREQ_Filters[] = {  // Channel, Filter type, Center frequency, Q value, Gain, Precision
  // {0, DSP_FILTER_LOW_PASS, 2000.0, 1.0, 0, PRC_FLT },
  // {0, DSP_FILTER_HIGH_PASS, 2000.0, 1.0, 0, PRC_FLT },
 };

// BiQuad specified filters
biquad_def_t BIQUAD_Filters[] = { // Biquad filter coefficients (b0, b1, b2, a1, a2), Precision
  // {DSP_ALL_CHANNELS, 0.9988329621231924, -1.9878152272905232, 0.9892409143014066, 1.9878152272905232, -0.988073876424599, PRC_FLT}, 
};
