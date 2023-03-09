#define DISPLAY_ON
#define DAC_24_BIT
#define WIFI_ON

#include <string.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2s.h>
#include <driver/i2c.h>
#include <TelnetSpy.h>
#include "es8388_registers.h"

#ifdef DISPLAY_ON
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif


//------------------------------------------------------------------------------------
// Constant definitions
//------------------------------------------------------------------------------------

#define SDA_PIN                 GPIO_NUM_18       // SDA pin number for I2C bus
#define SCL_PIN                 GPIO_NUM_23       // SCL pin number for I2C bus

#define CORE_MAIN               1                 // Core running main loop 
#define CORE_DSP                0                 // Core running DSP loop

#define TASK_DELAY              10

#define PRC_FLT                 0                 // Set filter precision to float
#define PRC_DBL                 1                 // Set filter precision to double        

#define DSP_ALL_CHANNELS        -1                // Specify all channels processed
#define DSP_NUM_CHANNELS        2                 // Number of channels
#define DSP_MAX_FILTERS         20                // Max number of biquad filters
#define DSP_SAMPLE_RATE         44100             // The sample rate
#define DSP_MAX_GAIN            24                // Maximum gain for the channel
#define DSP_MAX_SAMPLES         96                // Maximum number of samples per channel each loop
#define DSP_MIN_DELAY_MILLIS    ((DSP_MAX_SAMPLES*1000)/DSP_SAMPLE_RATE+1)
#define DSP_MAX_DELAY_MILLIS    250               // Maximum delay allowed in milliseconds
#define DSP_MAX_DELAY_SAMPLES   ((DSP_MAX_DELAY_MILLIS*DSP_SAMPLE_RATE)/1000+1)
#define DSP_ADC_ATTENUATE       0                 // Attenuation of input by 0.5 dBs

#define DSP_FILTER_LOW_PASS     0
#define DSP_FILTER_HIGH_PASS    1
#define DSP_FILTER_BAND_PASS    2
#define DSP_FILTER_NOTCH        3
#define DSP_FILTER_APF          4
#define DSP_FILTER_PEAK_EQ      5
#define DSP_FILTER_LOW_SHELF    6
#define DSP_FILTER_HIGH_SHELF   7

#ifdef DAC_24_BIT
typedef int32_t    sample_t;
#define SAMPLE_BITS             24
#else
typedef int16_t    sample_t;
#define SAMPLE_BITS             16
#endif

#define SAMPLE_NULL_BITS        (sizeof(sample_t)*8 - SAMPLE_BITS)

#define DSP_BITS_PER_SAMPLE     ((i2s_bits_per_sample_t) (sizeof(sample_t)*8))
#define DSP_DAC_WORD_LENGTH     (sizeof(sample_t)/2+2) // 16-bit = 011, 32-bit = 100
#define DSP_MAX_LEVEL           ((1 << (SAMPLE_BITS - 1)) - 1)

#define DITHER_ON               0
#define DITHER_RANGE_DB         96
#define DITHER_BITS             (SAMPLE_BITS - DITHER_RANGE_DB/6)

//------------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------------

typedef struct filter_def_t {
  int           channel;                          // Filter channel
  int           filter_type;                      // Type of filter to apply
  float         frequency;                        // Centre frequency of filter
  double        Q;                                // Q value
  float         gain;                             // Gain value in dB
  int           precision;                        // Implement as float or double
} filter_def_t;

typedef struct biquad_def_t {
  int           channel;                          // Associated channel
  double        coeffs[5];                        // Biquad coefficients
  int           precision;                        // Implement as float or double
} biquad_def_t;

typedef struct dsp_filter_t {
  double        coeffs_d[5];                      // The biquad coefficients for each of the filters (double precision)
  float         coeffs_f[5];                      // The biquad coefficients for each of the filters (float)
  float         w[2];                             // Array of historic W values for each biquad filter
  int           precision;                        // Precision calculation
  filter_def_t* filter_def;                       // Associated frequency defined filter
} dsp_filter_t;

typedef struct dsp_data_t {
  float         scaling_factor;                   // Factor used to scale values for specified gain
  int           delay_samples;                    // Number of calculated samples delayed in buffer
  int           delay_offset;                     // Offset within the delay buffer for storing next set of input values
  int           in_clip_count;                    // Number of times input audio clipped per channel
  int           out_clip_count;                   // Number of times output audio clipped per channel
  long int      in_max_level;                     // Max input level per last sample
  long int      out_max_level;                    // Max output level per last sample
  sample_t      delay_buff[DSP_MAX_DELAY_SAMPLES];// Sample delay buffer
  dsp_filter_t  filter[DSP_MAX_FILTERS];          // Filters for channel
  int           num_filters;                      // Total number of filters in the channel
} dsp_data_t;

typedef struct dsp_channel_t {
  const char*   name;                             // Name of the channel
  int           inputs[DSP_NUM_CHANNELS];         // Input channels from the source
  float         gain_dB;                          // The amount of gain added to the channel
  int           delay_millis;                     // The delay (in millseconds) introduced into the channel
  dsp_data_t*   data;                             // Data buffer for the channel
} dsp_channel_t;


//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

extern TelnetSpy      SerialAndTelnet;
extern char           strIPAddress[16];

#ifdef WIFI_ON
  #undef SERIAL        
  #define SERIAL      SerialAndTelnet
#else
  #undef SERIAL        
  #define SERIAL      Serial
#endif

//------------------------------------------------------------------------------------
// Global functions (C++)
//------------------------------------------------------------------------------------

esp_err_t         dsp_init( TaskHandle_t* taskDSP );
void              dsp_task( void* pvParameters );
void              dsp_command( char command );
void              dsp_filter_info( dsp_channel_t* channels );
void              dsp_plot( dsp_channel_t* channels );
esp_err_t         dsp_filter_init( dsp_channel_t* channels, biquad_def_t* biquad_defs, int biquad_def_count, filter_def_t* filter_defs, int filter_def_count );
esp_err_t         dsp_filter( dsp_channel_t* channels, sample_t* input_buffer, sample_t* output_buffer, int buffer_len, bool filters_enabled, bool* clip_flag );
esp_err_t         dsp_get_biquad( filter_def_t* filter, double* coeffs );
biquad_def_t*     dsp_import_filters( int* import_filter_count );
int32_t           dsp_dither( int32_t sample );


//------------------------------------------------------------------------------------
// Global functions (C)
//------------------------------------------------------------------------------------

extern "C" {
  esp_err_t       dsps_biquad_f32_ae32( const float* input, float* output, int len, float* coef, float* w );
}

extern "C" {
  esp_err_t       dsps_biquad_f32_dbl( const float *input, float *output, int len, double *coef, float* w);
}

#ifdef DISPLAY_ON
bool              dsp_display_init();
void              dsp_display_error();
void              dsp_display_output( int channel_id, sample_t level, int clip_count );
void              dsp_display_input( int channel_id, sample_t level, int clip_count );
void              dsp_display_loop();
#endif
