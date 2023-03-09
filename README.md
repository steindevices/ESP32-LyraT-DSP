# ESP32_LyraT_DSP
An audio subwoofer DSP built on the Espressif ESP32 LyraT.

(Note: This is the first time I have posted anything to GitHub and I am sure I am making a huge number of rookie mistakes. Please bear with me.)

As an avid home theater builder, I was looking for a way to reduce the differential bass output that is typically experienced by listeners in different room locations. This is a common problem wherein the characteristics of the room cause different modes at different locations. In my case, listeners at the back of the room sit higher (on a stage) than those in the front creating significant relative boominess in the back. After reading several papers on flattening overall room response by employing a program such as REW to analyze room response, and then applying the derived filters to multiple subwoofers, I thought I would try creating my own subwoofer DSP for fun. 

I recognize that this can also be accomplished by retail products such as the MiniDSP, but with the release of Espressif's ESP32 LyraT which includes:

- A fast ESP32 Wrover processor
- On-board WiFi capabilities
- 24-bit ADC/DAC
- Stereo IN/OUT
- Dual on-board microphones
- DSP library from Espressif

and costs less than $30, it seemed like everything was there to create my own two channel DSP for cheap. 

The goal of the project is to:

1. Support at least two subwoofers
2. Allow for cascading biquads (up to 10) for each channel at a sample rate of 44.1K without audible issues
3. Provide a means to also control gain and delay for each channel
4. Over-the-air (OTA) DSP update capability
5. A means to interact with the device remotely (since it would be buried in a cabinet)
6. Arduino support (just makes life easier overall)

The code provided here consists of the following:

- dsp_config.h			- Configures the two channels including gain, delay, and biquads. A maximum of 10 biquads are allowed for each channel.
- dsp_filter.cpp 		- Code that converts the input buffer supplied by the LyraT to the filtered result.
- dsp_plot.cpp			- Plots the transfer function on request to the serial output.
- dsp_process.cpp		- Initializes and acts as the main interface to the DSP. MUCH of this code I borrowed from https://github.com/Jeija/esp32-lyrat-passthrough.
- dsp_process.h			- Header file for the DSP.
- es8388_registers.h		- Defines the registers of the ES8388 codec.
- dsps_biquad_f32_ae32.S	- Assembly code provided by Espressif for calculating the Biquad filters.
- dsps_dotprod_f32_m_ae32.S	- Additional assembly code to support dot product calculations for Biquad filters.

Note: It was necessary to bring in the Espressif assembly code routines directly into the project as the libraries for the LyraT DSP are not available within the Arduino environment.

In order to support the requirements above, it was necessary to include the following additional Arduino libraries for the ESP32 Wrover:

- ArduinoOTA	- To support over-the-air updates to the device
- TelnetSpy	- To support user commands to the DSP from a Telnet connection

(Instructions regarding how to employ these Arduino libraries are available from their respective GitHubs).

When accessing the DSP from Telnet, the following commands are currently available:

- i - Display DSP config information for all channels. Also displayed at start-up.
- p - Print text-based transfer curve (frequency response) curve for each channel. The width and height of the outputted plot can be changed by updating parameters in dsp_plot.cpp
- d - Disable DSP processing (passthrough mode)
- e - Enable DSP processing (apply filters mode - default)
- s - Stop the DSP (mute)
- r - Run the DSP (un-mute)

The list of commands is not supposed to be comprehensive, but more a starting point. A quick review of the code will show how the commands can be expanded/changed.

I have placed this code in the public domain to see if anyone else might have some interest in using the LyraT as a formalized DSP including expanding its functionality. One obvious extension would be to use the onboard microphones to perform the room analysis as well, thereby eliminating the need for a program such as REW completely. That would be cool!

Aaron