#include "dsp_process.h"

static const char   compile_date[] = __DATE__ " " __TIME__;
static float        Biquad_Buff_F32[ DSP_MAX_SAMPLES ];  // Single channel input buffer for biquad function
static char*        filter_name[] = {"Low Pass", "High Pass", "Band Pass", "Notch Pass", "All Pass", "Peak EQ", "Low Shelf", "High Shelf" };


//------------------------------------------------------------------------------------
// Send DSP information for all channels to serial output
//------------------------------------------------------------------------------------
void dsp_filter_info( dsp_channel_t* channels ) {

  dsp_channel_t*  channel;
  dsp_data_t*     dsp_data;
  filter_def_t*   filter_def;

  SERIAL.printf( "Compile date: %s\r\n", compile_date );
  SERIAL.printf( "I-DSP:   Sampling rate = %d\r\n", DSP_SAMPLE_RATE );
  SERIAL.printf( "I-DSP:   Sampling bits = %d\r\n", SAMPLE_BITS );
  SERIAL.printf( "I-DSP:   Sampling delay = %f ms\r\n", ((float) DSP_MAX_SAMPLES)*1000*2/DSP_SAMPLE_RATE );  
  SERIAL.printf( "I-DSP:   Dither = %s\r\n", DITHER_ON ? "ON" : "OFF" );  
  SERIAL.printf( "\r\n" );

  for( int channel_id = 0; channel_id < DSP_NUM_CHANNELS ; ++ channel_id ) {
    channel = &channels[channel_id];
    dsp_data = channel->data;

    SERIAL.printf( "I-DSP: Channel %c: %s\r\n", channel_id + 'A', channel->name );
    for( int i=0; i < DSP_NUM_CHANNELS; ++ i ) {
      SERIAL.printf( "I-DSP:   Input %d = %d\r\n", i, channel->inputs[i] );
    }
    SERIAL.printf( "I-DSP:   Gain = %f dB\r\n", channel->gain_dB );
    SERIAL.printf( "I-DSP:   Scaling factor = %f\r\n", dsp_data->scaling_factor );
    SERIAL.printf( "I-DSP:   User delay = %d millis\r\n", channel->delay_millis );
    SERIAL.printf( "I-DSP:   Delay samples = %d\r\n", dsp_data->delay_samples );
    SERIAL.printf( "I-DSP:   Input level max = %d\r\n", dsp_data->in_max_level );
    SERIAL.printf( "I-DSP:   Output level max = %d\r\n", dsp_data->out_max_level );    
    SERIAL.printf( "I-DSP:   Input clipping count = %d\r\n", dsp_data->in_clip_count );
    SERIAL.printf( "I-DSP:   Output clipping count = %d\r\n", dsp_data->out_clip_count );
    SERIAL.printf( "I-DSP:   Filter count = %d\r\n", dsp_data->num_filters );

    for( int i = 0; i < dsp_data->num_filters; ++ i ) {
      // Show BiQuad information
      SERIAL.printf( "I-DSP:   Filter %d coeffs = %16.14e %16.14e %16.14e %16.14e %16.14e (%s)\r\n",
        i+1, dsp_data->filter[i].coeffs_d[0], dsp_data->filter[i].coeffs_d[1],
        dsp_data->filter[i].coeffs_d[2], -dsp_data->filter[i].coeffs_d[3], 
        -dsp_data->filter[i].coeffs_d[4], dsp_data->filter[i].precision == PRC_DBL ? "DBL" : "FLT" );

      // Show filter information (if applicable)
      filter_def = dsp_data->filter[i].filter_def;
      
      if( filter_def != NULL ) {
        SERIAL.printf( "I-DSP:     %s: Frequency=%7.1f  Q=%16.14e  Gain=%4.1f\r\n", 
          filter_name[ filter_def->filter_type ], filter_def->frequency, filter_def->Q, filter_def->gain );
      }
    }
    SERIAL.printf( "\r\n" );
  }
}


//------------------------------------------------------------------------------------
// Allocate and setup the DSP channel data 
//------------------------------------------------------------------------------------
static dsp_data_t* dsp_setup_channel( dsp_channel_t* channel ) {

  dsp_data_t*       dsp_data;
  int               delay_samples;  
  
  // Check if specified gain is within limits
  if( channel->gain_dB < -DSP_MAX_GAIN || channel->gain_dB > DSP_MAX_GAIN ) {
    SERIAL.printf( "E-DSP: ERROR: Invalid gain setting for channel '%s'\r\n", channel->name );
    return( NULL );
  }

  // Check if specified delay is within limits
  if( channel->delay_millis != 0 ) {
    if( channel->delay_millis < DSP_MIN_DELAY_MILLIS || channel->delay_millis > DSP_MAX_DELAY_MILLIS ) {
      SERIAL.printf( "E-DSP: Invalid delay setting for channel '%s'\r\n", channel->name );
      return( NULL );
    }
  }

  // Allocate the necessary data buffers for delay and biquad calculations
  channel->data = (dsp_data_t*) malloc( sizeof( dsp_data_t ) );

  if( channel->data == NULL ) {
    SERIAL.printf( "E-DSP: Unable to allocate data structure for channel '%s'\r\n", channel->name );
    return( NULL );
  }
  
  dsp_data = channel->data;

  // Set scaling factor
  dsp_data->scaling_factor = exp10( channel->gain_dB/20.0 );
  dsp_data->num_filters = 0;
  
  // Set channel clipping counts
  dsp_data->in_clip_count = 0;
  dsp_data->out_clip_count = 0;

  // Set max levels
  dsp_data->in_max_level = 0;
  dsp_data->out_max_level = 0;

  // Calculate number of delay samples required
  delay_samples = DSP_SAMPLE_RATE*channel->delay_millis/1000;

  if( delay_samples > 0 ) {
    // Set up the delay buffer
    dsp_data->delay_samples = delay_samples;
    dsp_data->delay_offset = 0;
    memset( dsp_data->delay_buff, 0, DSP_MAX_DELAY_SAMPLES*sizeof( sample_t ) );
  } else {
    // No delay buffer
    dsp_data->delay_samples = 0;
    dsp_data->delay_offset = 0;
  }  

  return( dsp_data );
}


//------------------------------------------------------------------------------------
// Load biquad definitions
//------------------------------------------------------------------------------------
static esp_err_t dsp_load_biquads( dsp_channel_t* channel, int channel_id, biquad_def_t* biquad_defs, int biquad_def_count ) {

  int         num_filters;
  dsp_data_t* dsp_data;

  dsp_data = channel->data;
  num_filters = dsp_data->num_filters;
  
  // Load each biquad defined filter
  for( int filter_id = 0; filter_id < biquad_def_count; ++filter_id ) {

    if( ( biquad_defs[filter_id].channel == channel_id ) || ( biquad_defs[filter_id].channel == DSP_ALL_CHANNELS ) ) {
      
      // Check if filter count is within limits
      if( num_filters > DSP_MAX_FILTERS ) {
        SERIAL.printf( "E-DSP: ERROR: Maximum filters exceeded for channel '%s'\r\n", channel->name );
        return( ESP_FAIL );
      }
      
      // Store biquads as floats and doubles (Note: reverse sign of a1 and a2 biquads)
      for( int i=0; i<5; ++i ) {
        dsp_data->filter[num_filters].coeffs_d[i] = biquad_defs[filter_id].coeffs[i];
        dsp_data->filter[num_filters].coeffs_f[i] = biquad_defs[filter_id].coeffs[i];          
      }

      dsp_data->filter[num_filters].precision = biquad_defs[filter_id].precision;
      
      dsp_data->filter[num_filters].w[0] = 0.0;
      dsp_data->filter[num_filters].w[1] = 0.0;
    
      dsp_data->filter[num_filters].filter_def = NULL;      

      ++ num_filters; 
    }
  }
  
  dsp_data->num_filters = num_filters;
  
  return( ESP_OK );
}


//------------------------------------------------------------------------------------
// Load frequency specified filter definitions
//------------------------------------------------------------------------------------
static esp_err_t dsp_load_filters( dsp_channel_t* channel, int channel_id, filter_def_t* filter_defs, int filter_def_count ) {

  int         num_filters;
  dsp_data_t* dsp_data;

  dsp_data = channel->data;
  num_filters = dsp_data->num_filters;

  // Load each frequency specified filter    
  for( int filter_id = 0; filter_id < filter_def_count; ++filter_id ) {

    if( ( filter_defs[filter_id].channel == channel_id ) || ( filter_defs[filter_id].channel == DSP_ALL_CHANNELS ) ) {
      
      // Check if filter count is within limits
      if( num_filters > DSP_MAX_FILTERS ) {
        SERIAL.printf( "E-DSP: ERROR: Maximum filters exceeded for channel '%s'\r\n", channel->name );
        return( ESP_FAIL );
      }

      if( dsp_get_biquad( &filter_defs[filter_id], &dsp_data->filter[num_filters].coeffs_d[0] ) != ESP_OK ) {
        return( ESP_FAIL );
      }

      // Save each double-precision coefficient also as floating point
      for( int i=0; i<5; ++i ) {
        dsp_data->filter[num_filters].coeffs_f[i] = dsp_data->filter[num_filters].coeffs_d[i];          
      }

      dsp_data->filter[num_filters].precision = filter_defs[filter_id].precision;
      
      dsp_data->filter[num_filters].w[0] = 0.0;
      dsp_data->filter[num_filters].w[1] = 0.0;
    
      dsp_data->filter[num_filters].filter_def = &filter_defs[filter_id];
      
      ++ num_filters;         
    }      
  }
  
  dsp_data->num_filters = num_filters;
  
  return( ESP_OK );  
}


//------------------------------------------------------------------------------------
// Initialize the DSP filters
//------------------------------------------------------------------------------------
esp_err_t dsp_filter_init( dsp_channel_t* channels, biquad_def_t* biquad_defs, int biquad_def_count, filter_def_t* filter_defs, int filter_def_count ) {

  dsp_channel_t*    channel;
  dsp_data_t*       dsp_data;
  biquad_def_t*     import_defs;
  int               import_def_count;

  // Load imported filters
  import_defs = dsp_import_filters( &import_def_count );
  if( import_defs == NULL ) { 
    return( ESP_FAIL );
  }

  for( int channel_id = 0; channel_id < DSP_NUM_CHANNELS; ++ channel_id ) {

    channel = &channels[channel_id];

    // Set up the channel data
    dsp_data = dsp_setup_channel( channel );
    if( dsp_data == NULL ) {
      return( ESP_FAIL );
    }

    if( dsp_load_biquads( channel, channel_id, import_defs, import_def_count ) == ESP_FAIL ) {
      return( ESP_FAIL );
    }      

    if( dsp_load_biquads( channel, channel_id, biquad_defs, biquad_def_count ) == ESP_FAIL ) {
      return( ESP_FAIL );
    }

    if( dsp_load_filters( channel, channel_id, filter_defs, filter_def_count ) == ESP_FAIL ) {
      return( ESP_FAIL );
    }
  }

  return( ESP_OK );
};


//------------------------------------------------------------------------------------
// Process the input buffer
//------------------------------------------------------------------------------------
static esp_err_t dsp_process_input( dsp_channel_t* channel, int sample_count, sample_t* input_buffer, bool* clip_flag, bool filters_enabled ) {

  dsp_data_t*       dsp_data;
  int               num_inputs;
  int               delay_samples;
  int               delay_offset;
  sample_t*         delay_buff;
  int               input_channel;
  sample_t          input_value; 
  int               max_level;  

  dsp_data = channel->data;

  // Determine number of input channels
  num_inputs = 0;
  for( int i = 0; i < DSP_NUM_CHANNELS; ++ i ) {
    if( channel->inputs[i] ) {
      ++ num_inputs;
    }
  }

  // If no input channels, set to 1 to avoid DIV/0
  if( num_inputs == 0 ) {
    num_inputs = 1;
  }  

  // Set delay parameters
  if( filters_enabled ) {
    delay_samples = dsp_data->delay_samples;
    delay_offset = dsp_data->delay_offset;
  } else {
    delay_samples = 0;
    delay_offset = 0;
  }
  
  delay_buff = &dsp_data->delay_buff[0]; 

  max_level = 0;  
 
  for( int i = 0; i < sample_count; ++ i ) {
    // Output the delayed samples from the delay buffer
    Biquad_Buff_F32[i] = delay_buff[delay_offset];

    // Replace the delay buffer sample with the next sample(s) from the input stream
    input_value = 0;
    for( input_channel = 0; input_channel < DSP_NUM_CHANNELS; ++ input_channel ) {
      input_value += ( ( input_buffer[i*DSP_NUM_CHANNELS + input_channel]/num_inputs )>>SAMPLE_NULL_BITS )*channel->inputs[input_channel];
    }
    delay_buff[delay_offset] = input_value;

    if( abs( input_value ) > max_level ) {
      max_level = abs( input_value );
    }

    if( abs( input_value ) >= DSP_MAX_LEVEL ) {      
       // Set clipping flag
      *clip_flag = true;        
      ++dsp_data->in_clip_count;          
    }
    
    // Increment the delay buffer pointer and wrap it when at end of delay buffer     
    if( delay_offset == delay_samples ) {
      delay_offset = 0;
    } else {
      ++ delay_offset;   
    }
  }

  // Update the buffer pointer
  dsp_data->delay_offset = delay_offset;

  // Set the input max level
  dsp_data->in_max_level = max_level; 

  return( ESP_OK );
}


//------------------------------------------------------------------------------------
// Process the filters 
//------------------------------------------------------------------------------------
static esp_err_t dsp_process_filters( dsp_data_t* dsp_data,int sample_count ) {

  esp_err_t         res;

  // Process each biquad filter in the channel
  if( dsp_data->num_filters > 0 ){
    
    int filter_id = 0;
    while( true ) {
      if( dsp_data->filter[filter_id].precision == PRC_DBL ) {
        res = dsps_biquad_f32_dbl( Biquad_Buff_F32, Biquad_Buff_F32, sample_count, dsp_data->filter[filter_id].coeffs_d, dsp_data->filter[filter_id].w );        
      } else {
        res = dsps_biquad_f32_ae32( Biquad_Buff_F32, Biquad_Buff_F32, sample_count, dsp_data->filter[filter_id].coeffs_f, dsp_data->filter[filter_id].w );
      }
      
      if( res != ESP_OK ) {
        SERIAL.printf( "E-DSP: ERROR: Failure during biquad processing = '%d'\r\n", res );
        return( res );
      }

      ++ filter_id;
      if( filter_id == dsp_data->num_filters ) {
        break;
      }
    }
  }

  return( res );
}


//------------------------------------------------------------------------------------
// Process the output buffer
//------------------------------------------------------------------------------------
static esp_err_t dsp_process_output( dsp_channel_t* channel, int channel_id, int sample_count, sample_t* output_buffer, bool* clip_flag, bool filters_enabled ) {

  dsp_data_t*       dsp_data;  
  sample_t          output_value;
  sample_t          prev_value;
  int               max_level;
  float             scaling_factor;

  dsp_data = channel->data;

  if( filters_enabled ) {
    scaling_factor = dsp_data->scaling_factor;      
  } else {
    scaling_factor = 1.0;
  }
    
  // Copy results of filter processing to the output filter
  prev_value = 0;
  max_level = 0;
  
  for( int i = 0; i < sample_count; ++ i ) {
    output_value = (int32_t) ( Biquad_Buff_F32[i]*scaling_factor );
    
    if( DITHER_ON ) {
      output_value = dsp_dither( output_value );
    }
    
    // Check if value out of range
    if( output_value < -DSP_MAX_LEVEL || output_value > DSP_MAX_LEVEL ) {          
       // Set clipping flag
      *clip_flag = true;
      ++ dsp_data->out_clip_count;

      // Set sample to limit audible distortion
      output_value = ( ( DSP_MAX_LEVEL*( output_value < 0 ? -1 : 1 ) ) + prev_value )/2;
    }

    output_buffer[i*DSP_NUM_CHANNELS + channel_id] = output_value << SAMPLE_NULL_BITS;
    
    if( abs( output_value ) > max_level ) {
      max_level = abs( output_value );
    }
    
    prev_value = output_value;
  }

  // Set the output max level
  dsp_data->out_max_level = max_level;

  return( ESP_OK );
}


//------------------------------------------------------------------------------------
// Process the audio stream by cascading the biquad filters and applying delay/gain
//------------------------------------------------------------------------------------
esp_err_t dsp_filter( dsp_channel_t* channels, sample_t* input_buffer, sample_t* output_buffer, int buffer_len, bool filters_enabled, bool* clip_flag ) {

  esp_err_t         res;
  dsp_channel_t*    channel;
  int               channel_id;
  dsp_data_t*       dsp_data;
  int               sample_count;
  
  // Check if input sample count exceeded
  sample_count = buffer_len/sizeof( sample_t )/2;

  if( sample_count > DSP_MAX_SAMPLES ) {
    SERIAL.printf( "E-DSP: Too many samples = '%d'\r\n", sample_count );
    return( ESP_FAIL );
  }

  // Reset the clipping flag
  *clip_flag = false;

  for( channel_id = 0; channel_id < DSP_NUM_CHANNELS ; ++ channel_id ) {
        
    channel = &channels[channel_id];
    dsp_data = channel->data;

    // Process the input buffer
    dsp_process_input( channel, sample_count, input_buffer, clip_flag, filters_enabled );      

#ifdef DISPLAY_ON
    // Send input level for display
    dsp_display_input( channel_id, dsp_data->in_max_level, dsp_data->in_clip_count );
#endif    

    // Apply the filters
    if( filters_enabled ) {
      dsp_process_filters( dsp_data, sample_count );
    }

    // Process the output buffer
    dsp_process_output( channel, channel_id, sample_count, output_buffer, clip_flag, filters_enabled );
    
#ifdef DISPLAY_ON
    // Send output level for display
    dsp_display_output( channel_id, dsp_data->out_max_level, dsp_data->out_clip_count );
#endif
  }
  return( ESP_OK );
}
