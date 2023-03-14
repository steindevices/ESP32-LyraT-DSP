# ESP32_LyraT_DSP

An audio DSP built with the Espressif ESP32 LyraT for around $20.

## Why did you build this?

In building my new home theater, I planned to dabble a bit into the DIY DSP world to try to tame some room anomalies without spending a bunch of cash. As I have experience in C development and the Arduino world, I figured I would try building my own inexpensive processor. I settled on the Espressif ESP32-Lyrat developer board as a platform as it is open source, cheap ($20), contains a fast processor (ESP32), and includes onboard WIFI as well as a 24-bit DAC/ADC (ES8388).

After some weeks of development, my DSP is now up and running and has been incorporated into my home theater.

## What can it do?

A DSP is like an equalizer - on steroids. Using digital filters, it allows highly detailed control over a signal before it gets sent to an output device. Typically it is used to do things like adjust subwoofer behaviour (e.g. adding a bass bump at 30Hz), equalize a speaker's overall response, or compensate for how a room colors sound at specific listening positions. These are all possible with this $20 project. And with this DSP, you can either design the filters yourself or use a program like REW to generate the filters for you. 

The DSP supports the following:

- 2 channel stereo input/output
- Input channel mixing
- Pre-defined filter types including low-pass, high-pass, band-pass, shelf, notch, APF, and peak EQ
- Support for user-defined biquad filters
- Independent channel delay
- Independent channel gain/attenuation
- Transfer function plotting
- Support for varying sample rates (e.g. 44 Khz, 48Khz)
- 16 and 24-bit dynamic range support
- 3D printed case developed in Autodesk Fusion 360

Filter settings, delays, etc. are controlled via updates to the source code, recompiling and uploading.

## Could I damage my speakers with this DSP?

**It's possible but unlikely if you take care.** You should ALWAYS lower your amplifier level whenever you upload new firmware to the DSP. This is for two reasons; 

1. The act of uploading into the DSP will generate a short burst of white noise at the tail end of the upload. The volume of this white noise cannot be controlled and can be quite loud. 

2. IMPORTANT: If you are implementing your own biquads, the filters may become unstable. Unstable filters generate VERY LOUD RANDOM NOISE VERY QUICKLY which can easily damage speakers if the amplifier volume is set high.

ALWAYS lower your amplifier volume initially when testing new filter uploads. Keep in mind that this is an open development project and as such it is up to you to ensure that your equipment is kept safe.

## What do I need for this project?

You'll need:

- Experience with the Arduino develpment environment
- An ESP32-Lyrat development board (ESP32-LyraT Overview | Espressif Systems) (around $20) available (for example) [here](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-LYRAT/9381704).
- (Optional) 0.96" IIC OLED LCD Screen LED 128X64 (<$5)
- (Optional) 3D printer

Of course, keep in mind that a DSP does not have the ability to power speakers directly. You will also need an amplifier as you would with any other DSP.

Note that this application currently only works with development board shown below: ![Image of ESP32-T LyraT Board](/Docs/Images/ESP32-LyraT (WROVER-E)_1.png)

There are a number of other ESP32-Lyrat boards, but they are configured with different features that may or may not work properly with this code. My guess is that most of this code will work on the other boards, but some modifications would probably needed. Besides, most of the other boards are more expensive and have fewer useful features for a pure audio application.

## Do I need anything else for this project?

You'll need two micro-USB power supply cables, as well as two stereo 3.5 mm audio cables. The former are used to power the DSP - a 1A 5V supply should be fine - and to interface with the ESP32 chip for uploads. Once the firmware has been uploaded the first time, you will be able to program the board over WiFi through the Arduino OTA interface, and will only need one cable for power after that. The two stereo cables are used to connect the DSP to an input source (e.g. receiver) and an output amplifier. If you want, you can also add an optional display to the DSP to show what it is doing. I have coded for a small 0.91" OLED that can readily be purchased on Amazon and will provide additional details on how to add it on request.

## I've built projects with the ESP8266 before. How does the ESP32 differ?

It's a lot more powerful, with 4.5 MB of memory and 2 CPUs running at up to 800 MHz. This makes it a good fit for this application. It also has WiFi and Bluetooth on board. Unless you are already developing for the ESP32, you will need to install the board through the Board Manager in the Arduino IDE. You can find instructions for installing the ESP32 boards [here](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/). 

The specific ESP32 for the ESP32-Lyrat is the ESP32 Wrover. This is the one you need to pick from the board list once the ESP32 boards have been installed.

Beyond that, working with this board in Arduino is very similar to the ESP8266 with ONE important exception; when uploading the firmware from the serial port, you MUST press the RESET and BOOT buttons in a certain sequence. THIS IS VERY IMPORTANT. 

As you initiate the upload you must:

*** (Image of board)

1. PRESS and HOLD the Reset button.
2. PRESS and HOLD the Boot button.
3. RELEASE the Reset button
4. RELEASE tbe Boot button

Fortunately, once the firmware for the DSP has been loaded the first time, you will be able to upload over WiFi through the Arduino OTA.

## How do I set up my Arduino environment to work with the ESP32-LyraT?

You need a few things:

### Arduino IDE

You will find it much easier to work with the ESP32 using the older Arduino 1.xx IDEs. There are some annoying bugs in the Arduino 2.xx IDE that make working with the EspressIf boards a bit of a pain. Until these get worked out, it's just easier to stick with the 1.xx IDE. If you don't aleady have it installed, here is a link to the most recent version ***. 

### Arduino libraries

You will need to install the following libraries through the Arduino library manager:

- ArduinoOTA	- To support over-the-air updates to the device
- TelnetSpy	- To support user commands to the DSP from a Telnet connection
- AdaFruitGFX 	- (if adding a Display)
- ***

Current versions of these libraries should be fine.

## Where do I get the code from?

The code is located in the GitHub repository [here]. All of the source files are contained in the one ESP_LyraT_DSP directory. Like any other Arduino project, transfer these files to a new directory named ESP32_Lyrat_DSP in your Arduino application directory and you are good to go.

## How do I configure the DSP for my WiFi?

Just update the file named credentials.h with your WiFi SSID and password. The DSP will automatically connect to your network.

## How do I add my own designed filters?

You add your filters by updating the dsp_config.h file. Example configurations can be found in the [Examples] *** directory for different applications. You can specify filters either in frequency form or as biquads. Both sets are compiled into the program and uploaded with the firmware. Ccombined, the maximum filters you can have is 20 per channel. The types of frequency filters that can be defined are:

- DSP_FILTER_LOW_PASS
- DSP_FILTER_HIGH_PASS
- DSP_FILTER_BAND_PASS
- DSP_FILTER_NOTCH
- DSP_FILTER_APF
- DSP_FILTER_PEAK_EQ
- DSP_FILTER_LOW_SHELF
- DSP_FILTER_HIGH_SHELF

User specified biquad filters are also supported in the dsp_config.h file in a similar fashion. Sequencing of the biquad filter coefficients is b0, b1, b2, a1, a2. Of course, it is up to the user to calculate the appropriate biquads for the filters they are implementing. [Here] *** is a link to a spreadsheet that will assist you in defining biquads if you decide to go this route.

## How do I configure my outputs settings?

In the dsp_config.h file, you specify the name of each output channel, amount of delay, gain, and mixing of the inputs. For example, in a dual subwoofer environment you might mix the two input channels and add filters to boost specific frequencies. If you are bullding a sub-satellite arrangement, you would typically specify for onbe channel a crossover with matching low and high pass filters for the sub and satellite speaker. To adjust for room behaviour, you might import filters from an external program like REW where the appropriate freqeuencies and Q's have been calculated. To add simple effects speakers to a room, you might specify a delay with a high pass filter. Configuration files for each of these situations is provided in the [Examples] *** directory.


## What is the maximum number of filters I can define?

You can define up to 20 filters per output channel. This includes your own filters and those imported from an external application like REW. If you try to upload more, the DSP will show an error in a telnet session or on the OLED display if one is attached.

## How do I import filters from REW?

If you are familiar with REW, you simply export the EQ filters from the application and then insert the contents into the file called dsp_import.h. Note that you must use the filter export format called XXX from REW, and that the contents must be pasted exactly as exported into the right location in the dsp_import.h file. 

If you are unfamilar with how to use REW to generate EQ filters, [here] *** is a link that shows how its done.

## How can I see what the DSP is doing?

When you connect the board directly via the serial port, you can issue commands that provide information as to the board status including filter information as well as errors. When not directly connected to the serial port, you can also use Putty or any other Telnet application to receive information from the DSP as it is running over WiFi. Simply create a telnet session to the DSP's IP address.

In both cases, here are the commands you can enter:

- i - Display DSP config information for all channels. Also displayed at start-up.
- p - Print text-based transfer curve (frequency response) curve for each channel. The width and height of the outputted plot can be changed by updating parameters in dsp_plot.cpp
- d - Disable DSP processing (pass-through mode)
- e - Enable DSP processing (apply filters mode - default)
- s - Stop the DSP (mute)
- r - Run the DSP (un-mute)
- restart - Reboot the DSP

## How can I upload updates to the DSP via Wifi?

Uploading the DSP via WiFi employs the same steps as uploading to other Arduino boards. You can find a description of the proces [here] ***.

## How do I add a display to my board?

If you are interested in adding a display, let me know via GitHub e-mail and I will provide details.

## I hear some background noise. What can I do about it?

Make sure you are using a good quality 5V power supply. Try different ones. If you can't seem to completely eliminate the noise, what you can do is add a bit of low level random noise in the DSP (called dither) to mask it. This is done by setting the variable DSP_DITHER to 1 in the file dsp_process.h.

## What about a case? Is there one available?

If you have access to a 3D printer, you will find an STL file for a case here. You will need 4 3mm screws to secure the board, and 4 2mm screws for the lid.

## I'd like to be able to upload new filters without needing to reprogram the DSP. Is this possible?

Not at the moment though I intend to build that capability into a future version. Stay tuned.

## How do I know if the DSP is clipping?

It is posssible to overdrive the DSP into clipping by either having input levels too high or by too much gain specified in the dsp_config.h file. The DSP will indicate this situation by flashing the green LED on the board. It will also display the number of times the input and output levels have clipped on the display if one is attached. This information is also shown through the 'i' command via the serial port/Telnet interface.

Note that the green LED will light when clipping occurs at either the input or output so you will need do determine where the clipping is occurring.

## My board is not responding to Telnet and I can't program it over WiFi. What happened and what can I do?

It's possible the ESP32 cannot connect to your WiFi. Sometimes it's just a matter of unplugging and plugging the board and it will reconnect. If you've made changes to the code or have made a mistake when configuring, the board may be in a reboot cycle. When this happens you will need to fix your changes and re-upload to the board via the serial port

## I'd like to better understand and create my own biquads. How can I do that?

There is a lot of information on Z-Transforms, IIR filters, biquads, etc. on the Internet, so best just to do a search. That said, [here] *** is a spreadsheet that will generate biquads for you without the need to understand the math. 

## How do I change the range of the DSP plot? (e.g. from 20 Hz to 120 Hz)

You can change the plot layout by making changes to the dsp_plot.cpp file and recompiling. Setting FREQ_RANGE_LOW to 20.0 and FREQ_RANGE_HEIGHT to 120.0 in the code will change the plot as requested here.
