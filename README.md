# ESP32_LyraT_DSP
The backstory:

In building my new home theater, I planned to dabble a bit into the DIY DSP world to try to tame some room anomalies without spending a bunch of cash. As I have experience in C/C++ and the Arduino world, I figured I would try building my own inexpensive processor. I settled on the Espressif ESP32-Lyrat developer board as a platform as it is open source, cheap ($20), contains a fast processor (ESP32), and includes onboard WIFI as well as a 24-bit DAC/ADC (ES8388).

After some weeks of development, my DSP is now up and running and has been incorporated into my home theater.

The DSP:

As per my personal requirements, my DSP project supports the following:

-2 channel INPUT/OUTPUT;
-Channel mixing;
-Pre-defined filter types including low-pass, high-pass, band-pass, shelf, notch, APF, and peak eq;
-Support for user-defined biquad filters;
-Independent channel delay;
-Independent channel gain/attenuation;
-Transfer function plotting;
-Support for varying sample rates (e.g. 44 Khz, 48Khz);
-3D printed case developed in Autodesk Fusion 360

As this was for my personal use, it is a bit rough around the edges and filter settings, delays, etc. are controlled via updates to the source code/recompiles. That said, it works and could be the foundation for a more comprehensive cheap DIY DSP effort.

The materials required:
-Experience with the Arduino develpment environment
-1 ESP32-Lyrat development board (ESP32-LyraT Overview | Espressif Systems) (around $20)
-1 (Optional) 0.96" IIC OLED LCD Screen LED 128X64 (<$5)
-1 (Optional) 3D printer

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