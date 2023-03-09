#include "dsp_process.h"

#ifdef DISPLAY_ON

#define       DSP_NAME              "ESP32-LyraT DSP v0.1"
const         char dsp_build_date[] = __DATE__ " " __TIME__; 

#define       SCREEN_WIDTH          128        
#define       SCREEN_HEIGHT         64        

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire ); 

#define       TEXT_HEIGHT           8
#define       BAR_MAX_WIDTH         88
#define       BAR_HEIGHT            8
#define       BAR_LABEL_WIDTH       10
#define       BAR_PEAK_WIDTH        2
#define       BAR_PEAK_TIME         1000    // Milliseconds
#define       IND_BLINK_DELAY       400     // Milliseconds   
#define       BAR_SPACER            1
#define       BAR_SECTION_SPACER    3
#define       BAR_CLIPPING_OFFSET   4
//#define     BAR_DB_SCALE  

#define       IND_SYMBOL_X          122
#define       IND_SYMBOL_Y          4         
#define       IND_SYMBOL_SIZE       2  

#define       INPUT_LABEL_X         0
#define       INPUT_LABEL_Y         (TEXT_HEIGHT + BAR_SECTION_SPACER)
#define       INPUT_BAR_X           BAR_LABEL_WIDTH
#define       INPUT_BAR_Y           (INPUT_LABEL_Y + TEXT_HEIGHT)

#define       OUTPUT_LABEL_X        0
#define       OUTPUT_LABEL_Y        (INPUT_BAR_Y + (BAR_HEIGHT + BAR_SPACER)*(DSP_NUM_CHANNELS - 1) + BAR_HEIGHT + BAR_SECTION_SPACER)

#define       OUTPUT_BAR_X          BAR_LABEL_WIDTH
#define       OUTPUT_BAR_Y          (OUTPUT_LABEL_Y + TEXT_HEIGHT)
 
sample_t      disp_input_level[DSP_NUM_CHANNELS] = {0,0};
sample_t      disp_output_level[DSP_NUM_CHANNELS] = {0,0};

static int    disp_input_peak[DSP_NUM_CHANNELS] = {0,0};
static int    disp_output_peak[DSP_NUM_CHANNELS] = {0,0};

static int    disp_input_clip_count[DSP_NUM_CHANNELS] = {0,0};
static int    disp_output_clip_count[DSP_NUM_CHANNELS] = {0,0};

static unsigned long peak_start;
static unsigned long ind_start;
static bool     ind_blink_flag; 

static bool     error_flag = false;
static bool     error_displayed = false;

//------------------------------------------------------------------------------------ 
// Display initialization
//------------------------------------------------------------------------------------
bool dsp_display_init() {
  
  // Note: Display MUST be set up after DSP init
  
  Wire.begin( SDA_PIN, SCL_PIN );
  
  if( !display.begin( SSD1306_SWITCHCAPVCC, 0x3C ) ) { 
    SERIAL.println( "SSD1306 allocation failed" );
    return( false );
  }
    
  display.clearDisplay();
  display.display();
  
  display.setTextSize( 1 );
  display.setTextColor( WHITE );
  display.setCursor( 0, 0 );
  display.cp437( true );

  // Display IP address
#ifdef WIFI_ON
  display.write( "IP:" );
  display.write( strIPAddress );
#else
  display.write( "WiFi OFF" );
#endif

  // Display labels
  display.setCursor( INPUT_LABEL_X, INPUT_LABEL_Y );
  display.write( "Input" );

  display.setCursor( INPUT_LABEL_X + BAR_LABEL_WIDTH + BAR_MAX_WIDTH + BAR_CLIPPING_OFFSET, INPUT_LABEL_Y );    
  display.write( "Clip" );
  
  display.setCursor( INPUT_LABEL_X, INPUT_BAR_Y );  
  display.write( "L" );
  
  display.setCursor( INPUT_LABEL_X, INPUT_BAR_Y + BAR_HEIGHT + BAR_SPACER );    
  display.write( "R" );
  
  display.setCursor( OUTPUT_LABEL_X, OUTPUT_LABEL_Y );    
  display.write( "Output" );

  display.setCursor( OUTPUT_LABEL_X, OUTPUT_BAR_Y );        
  display.write( "A" );

  display.setCursor( OUTPUT_LABEL_X, OUTPUT_BAR_Y + BAR_HEIGHT + BAR_SPACER );      
  display.write( "B" );
  
  peak_start = millis();
  ind_start = millis();
  ind_blink_flag = true;
    
  display.display();
  
  return( true );
}


//------------------------------------------------------------------------------------ 
// Indicate error has occurred
//------------------------------------------------------------------------------------
void dsp_display_error() {

  // Disable further DSP displays
  error_flag = true;
}


//------------------------------------------------------------------------------------ 
// Set the input level for the display
//------------------------------------------------------------------------------------
void dsp_display_input( int channel_id, sample_t level, int clip_count ) {
  
  disp_input_level[channel_id] = level;
  disp_input_clip_count[channel_id] = clip_count;
}


//------------------------------------------------------------------------------------ 
// Set the output level for the display
//------------------------------------------------------------------------------------
void dsp_display_output( int channel_id, sample_t level, int clip_count ) {
  
  disp_output_level[channel_id] = level;
  disp_output_clip_count[channel_id] = clip_count;
}


//------------------------------------------------------------------------------------ 
// Convert long value to last 4 digits
//------------------------------------------------------------------------------------ 
static char* i_to_a4( int count, char* text, int buff_len ) {

  count %= 10000;
  
  text[buff_len] = 0;
  text[-- buff_len] = (count % 10) + '0'; 

  while( -- buff_len >= 0 ) {
    count /= 10; 
    text[buff_len] = (count == 0 ? ' ' : (count % 10) + '0' );
  }
  
  return( text );
}


//------------------------------------------------------------------------------------ 
// Fast log2 calculation
//------------------------------------------------------------------------------------ 
static int dsp_log2( sample_t sample ) {

  for( int i=SAMPLE_BITS-1; i >= 0; --i ) {
    if( sample & (1 << i ) ) {
      return( i );
   }
  }
  return( -1 );
}


//------------------------------------------------------------------------------------ 
// Display real-time information
//------------------------------------------------------------------------------------
void dsp_display_loop() {
  
  int       bar_width;
  char      text[5];

  // Display error message if error occurred
  if( error_flag ) {
    if( !error_displayed ) {
      display.clearDisplay();
      display.setCursor( 0, 0 );      
      display.write( "DSP-ERR: Init Error." );
      display.display();
      error_displayed = true;
    }
    return;
  }

  // Display indicator
  if( millis() - ind_start > IND_BLINK_DELAY ) {
    ind_blink_flag = !ind_blink_flag;
    ind_start = millis();

    if( ind_blink_flag ) {
      display.fillCircle( IND_SYMBOL_X, IND_SYMBOL_Y, IND_SYMBOL_SIZE, WHITE );
    } else {
      display.fillCircle( IND_SYMBOL_X, IND_SYMBOL_Y, IND_SYMBOL_SIZE, BLACK );
    }
  }
  
  // Blank out current input bars
  display.fillRect( INPUT_BAR_X, INPUT_BAR_Y, SCREEN_WIDTH - INPUT_BAR_X + 1, DSP_NUM_CHANNELS*BAR_HEIGHT + BAR_SPACER, BLACK );
  display.drawRect( INPUT_BAR_X, INPUT_BAR_Y, BAR_MAX_WIDTH, DSP_NUM_CHANNELS*BAR_HEIGHT + BAR_SPACER, WHITE );  

  for( int channel_id = 0; channel_id < DSP_NUM_CHANNELS; ++ channel_id ) {
    // Show the input level bars 

#ifdef BAR_DB_SCALE      
    bar_width = (max(dsp_log2(disp_input_level[channel_id]) - DITHER_BITS,0)*BAR_MAX_WIDTH)/(SAMPLE_BITS - DITHER_BITS - 1);  
#else
    bar_width = ((int64_t) disp_input_level[channel_id]*BAR_MAX_WIDTH)/DSP_MAX_LEVEL;
#endif
   
    display.fillRect( INPUT_BAR_X, INPUT_BAR_Y + channel_id*(BAR_HEIGHT + BAR_SPACER), bar_width, BAR_HEIGHT, WHITE );
    
    if( bar_width > disp_input_peak[channel_id] ) {
      disp_input_peak[channel_id] = bar_width;
    }

    // Display peak bar
    display.fillRect( INPUT_BAR_X + disp_input_peak[channel_id] - 1, INPUT_BAR_Y + channel_id*(BAR_HEIGHT + BAR_SPACER), BAR_PEAK_WIDTH, BAR_HEIGHT, WHITE );

    // Display clip counts
    display.setCursor( INPUT_BAR_X + BAR_MAX_WIDTH + BAR_CLIPPING_OFFSET, INPUT_BAR_Y + channel_id*(BAR_HEIGHT + BAR_SPACER) );
    display.write( i_to_a4( disp_input_clip_count[channel_id], text, 4 ) );    
  }

  // Blank out current output bars
  display.fillRect( OUTPUT_BAR_X, OUTPUT_BAR_Y, SCREEN_WIDTH - OUTPUT_BAR_X + 1, DSP_NUM_CHANNELS*BAR_HEIGHT + BAR_SPACER, BLACK );
  display.drawRect( OUTPUT_BAR_X, OUTPUT_BAR_Y, BAR_MAX_WIDTH, DSP_NUM_CHANNELS*BAR_HEIGHT + BAR_SPACER, WHITE );  

  for( int channel_id = 0; channel_id < DSP_NUM_CHANNELS; ++ channel_id ) {
    // Show the output level bars

#ifdef BAR_DB_SCALE    
    bar_width = (max(dsp_log2(disp_output_level[channel_id]) - DITHER_BITS,0)*BAR_MAX_WIDTH)/(SAMPLE_BITS - DITHER_BITS - 1);    
#else
    bar_width = ( (int64_t) disp_output_level[channel_id]*BAR_MAX_WIDTH)/DSP_MAX_LEVEL;    
#endif
        
    display.fillRect( OUTPUT_BAR_X, OUTPUT_BAR_Y + channel_id*(BAR_HEIGHT + BAR_SPACER), bar_width, BAR_HEIGHT, WHITE );

    if( bar_width > disp_output_peak[channel_id] ) {
      disp_output_peak[channel_id] = bar_width;
    }
    
    // Display peak bar
    display.fillRect( OUTPUT_BAR_X + disp_output_peak[channel_id] - 1, OUTPUT_BAR_Y + channel_id*(BAR_HEIGHT + BAR_SPACER), BAR_PEAK_WIDTH, BAR_HEIGHT, WHITE );  

    // Display clip counts
    display.setCursor( OUTPUT_BAR_X + BAR_MAX_WIDTH + BAR_CLIPPING_OFFSET, OUTPUT_BAR_Y + channel_id*(BAR_HEIGHT + BAR_SPACER) );
    display.write( i_to_a4( disp_output_clip_count[channel_id], text, 4 ) );
  }
  
  display.display();

  if( millis() - peak_start > BAR_PEAK_TIME ) {
    // Reset the peak bars
    
    for( int channel_id = 0; channel_id < DSP_NUM_CHANNELS; ++ channel_id ) {
      disp_input_peak[channel_id] = 0;
      disp_output_peak[channel_id] = 0;      
    }
    
    peak_start = millis();
  }
}
#endif
