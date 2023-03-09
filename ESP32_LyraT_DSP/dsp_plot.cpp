#include "dsp_process.h"

#define     FREQ_RANGE_LOW      20.0
#define     FREQ_RANGE_HIGH     20480.0
#define     CHART_DB_LOW        -10
#define     CHART_DB_HIGH       +10
#define     CHART_HEIGHT        20
#define     CHART_INDENT        6
#define     COL_CHART_MAX       181
#define     COL_TICKS_MIN       6
#define     ROW_SCALING         ((float) (CHART_DB_HIGH - CHART_DB_LOW)/CHART_HEIGHT)
#define     ROW_TICKS           5
#define     LINE_DASH           '-'
#define     LINE_CROSS          '+'
#define     LINE_MARKER         'O'
#define     LINE_BAR            '|'
#define     LABEL_MAX_WIDTH     ((int) (log10( FREQ_RANGE_HIGH ) + 1))
#define     OUTPUT_WIDTH        (CHART_INDENT + COL_CHART_MAX + LABEL_MAX_WIDTH + 1)

static      float               ch_freq[ COL_CHART_MAX ];
static      float               ch_gain[ COL_CHART_MAX ];
static      int                 line_plot[ COL_CHART_MAX ];


//------------------------------------------------------------------------------------
// Calculate values for transfer function plot
//------------------------------------------------------------------------------------
static void dsp_xfer_func( dsp_channel_t* channel, float freq_range_low, float freq_range_high, int freq_range_bands, int sample_rate, float* frequency, float* gain ) {

  dsp_data_t* dsp_data;
  float       freq_interval;
  float       freq;
  float       w;
  float       phi[ freq_range_bands ];
  float       coeffs[ 5 ];
  int         band;
  int         filter;

  // Calculate interval based on the passed range
  freq_interval = exp( log( freq_range_high/freq_range_low )/( freq_range_bands - 1 ) );

  for( band = 0; band < freq_range_bands; ++ band ) {

    // Calculate band frequency
    freq = freq_range_low*pow( freq_interval, band );
    frequency[ band ] = freq;

    // Calculate w value for the band
    w = 2*PI*freq/sample_rate;

    // Calculate and save phi value for the band
    phi[ band ] = 4*pow( sin( w/2 ),2 );
  }

  // Calculate the dB value for each band by calculating for each filter
  dsp_data = channel->data;
  for( band = 0; band < freq_range_bands; ++ band ) {
    gain[ band ] = 0.0;

    for( filter=0; filter < dsp_data->num_filters; ++ filter ) {

      for( int i = 0; i < 5; ++ i ) {
        coeffs[ i ] = dsp_data->filter[ filter ].coeffs_f[ i ];
      }

      gain[ band ] +=
        10*log10(pow(coeffs[0]+coeffs[1]+coeffs[2],2)+(coeffs[0]*coeffs[2]*phi[band]-(coeffs[1]*(coeffs[0]+coeffs[2])+4*coeffs[0]*coeffs[2]))*phi[band]) -
        10*log10(pow(1-coeffs[3]-coeffs[4],2)+(-coeffs[4]*phi[band]-(-coeffs[3]*(1-coeffs[4])-4*coeffs[4]))*phi[band]);        
    }
  }
}


//------------------------------------------------------------------------------------
// Plot the transfer curve for each channel to the serial output
//------------------------------------------------------------------------------------
void dsp_plot( dsp_channel_t* channels ) {

  int       row;
  int       col;
  int       first_row;
  int       last_row;
  float     dB_value;
  int       point;
  char      text_line[ OUTPUT_WIDTH ];
  char      freq_text[ LABEL_MAX_WIDTH + 1];
  int       doubling_count;
  int       doubling_columns;  
  int       tick_count;
  int       tick_columns;  
  int       chart_columns;

  // Calculate chart width and column spacing for optimal scale
  doubling_count = log( FREQ_RANGE_HIGH/FREQ_RANGE_LOW )/log(2);
  doubling_columns = ( COL_CHART_MAX - 1 )/doubling_count;
  tick_count = 0;
  while( doubling_columns/( tick_count + 1 ) >= COL_TICKS_MIN ) {
    ++ tick_count;
  }
  
  tick_columns = doubling_columns/tick_count;
  chart_columns = tick_columns*tick_count*doubling_count + 1;

  for( int chan_id = 0; chan_id < DSP_NUM_CHANNELS; ++ chan_id ) {

    dsp_xfer_func( &channels[ chan_id ], FREQ_RANGE_LOW, FREQ_RANGE_HIGH, chart_columns, DSP_SAMPLE_RATE, ch_freq, ch_gain );

    for( col = 0; col < chart_columns; ++ col ) {

      // Convert y value to display range
      dB_value = ch_gain[ col ] + channels[ chan_id ].gain_dB;

      if( dB_value < CHART_DB_LOW ) {
        dB_value = CHART_DB_LOW;
      } else if( dB_value > CHART_DB_HIGH ) {
        dB_value = CHART_DB_HIGH;
      }

      row = (round( -dB_value ) + CHART_DB_HIGH)/ROW_SCALING;

      line_plot[col] = row;
    }

    dB_value = CHART_DB_HIGH;
    text_line[ chart_columns ] = '\0';

    SERIAL.printf( "Channel %c: %s\r\n", chan_id + 'A', channels[ chan_id ].name );
    for( row = 0; row <= CHART_HEIGHT; ++ row ) {
      // Blank out the line
      if( ( ( row % ROW_TICKS ) == 0 ) || ( row == CHART_HEIGHT ) ) {
        memset( text_line, LINE_DASH, chart_columns );
      } else {
        memset( text_line, ' ', chart_columns );
      }

      for( col = 0; col < chart_columns; ++ col ) {
        if( ( col == 0 ) || ( col == chart_columns - 1 ) ) {
          text_line[ col ] = LINE_BAR;
        } else if( ( ( col % tick_columns ) == 0 ) && ( ( row % ROW_TICKS ) == 0 ) ) {
          text_line[ col ] = LINE_CROSS;
        }

        first_row = line_plot[ col ];

        if( row == first_row ) {
          text_line[ col ] = LINE_MARKER;
        } else if( col != chart_columns - 1 ) {
          last_row = line_plot[ col + 1 ];
          if( ( row - first_row )*( last_row - row ) > 0 ) {
            if( abs( row - first_row ) <= abs( row - last_row ) ) {
              text_line[ col ] = LINE_MARKER;
            } else {
              text_line[ col + 1 ] = LINE_MARKER;
            }
          }
        }
      }

      SERIAL.printf( "%+5.1f %s\r\n", dB_value, text_line );
      dB_value -= ROW_SCALING;
    }

    memset( text_line, ' ', CHART_INDENT + chart_columns + LABEL_MAX_WIDTH );
    text_line[ CHART_INDENT + chart_columns + LABEL_MAX_WIDTH ] = '\0';

    for( col = 0; col < chart_columns; ++ col ) {
      if( ( col % tick_columns ) == 0 ) {
          // Place the frequency value into the text line
          sprintf( freq_text, "%d", (int) ( ch_freq[ col ] + 0.5 ) );
          memcpy( text_line + CHART_INDENT + col - strlen( freq_text )/2, freq_text, strlen( freq_text ) );
      }
    }
    SERIAL.printf( "%s\r\n", text_line );

    SERIAL.println();
  }
}
