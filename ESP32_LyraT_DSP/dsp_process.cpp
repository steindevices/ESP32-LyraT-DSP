#include "dsp_process.h"
#include "dsp_config.h"

#define I2S_NUM         I2S_NUM_0

#define I2S_READLEN     DSP_MAX_SAMPLES*sizeof( sample_t )
static  sample_t        i2s_input_buffer[DSP_MAX_SAMPLES];
static  sample_t        i2s_output_buffer[DSP_MAX_SAMPLES];

#define I2C_NUM         I2C_NUM_0
#define ES8388_ADDR     0x20

static  bool            dsp_filters_enabled   = true;
static  bool            dsp_output_enabled    = true; 
static  bool            dsp_ok_flag           = true;       


//------------------------------------------------------------------------------------ 
// ES8388 write register
//------------------------------------------------------------------------------------
static esp_err_t es_write_reg(uint8_t slave_add, uint8_t reg_add, uint8_t data)
{
  esp_err_t res = ESP_OK;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  res |= i2c_master_start(cmd);
  res |= i2c_master_write_byte(cmd, slave_add, 1 /*ACK_CHECK_EN*/);
  res |= i2c_master_write_byte(cmd, reg_add, 1 /*ACK_CHECK_EN*/);
  res |= i2c_master_write_byte(cmd, data, 1 /*ACK_CHECK_EN*/);
  res |= i2c_master_stop(cmd);
  res |= i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);

  return( res );
}


//------------------------------------------------------------------------------------ 
// ES8388 initialization
//------------------------------------------------------------------------------------
static esp_err_t es8388_init()
{
  esp_err_t res = ESP_OK;

  i2c_config_t i2c_config;
  i2c_config.mode = I2C_MODE_MASTER;
  i2c_config.sda_io_num = SDA_PIN;
  i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_config.scl_io_num = SCL_PIN;
  i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_config.master.clk_speed = 100000;
  
  res |= i2c_param_config(I2C_NUM, &i2c_config);
  res |= i2c_driver_install(I2C_NUM, i2c_config.mode, 0, 0, 0);

  /* mute DAC during setup, power up all systems, slave mode */
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL3, 0x04);
  res |= es_write_reg(ES8388_ADDR, ES8388_CONTROL2, 0x50);
  res |= es_write_reg(ES8388_ADDR, ES8388_CHIPPOWER, 0x00);
  res |= es_write_reg(ES8388_ADDR, ES8388_MASTERMODE, 0x00);

  /* power up DAC and enable only LOUT1 / ROUT1, ADC sample rate = DAC sample rate */
  res |= es_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0x30);
  res |= es_write_reg(ES8388_ADDR, ES8388_CONTROL1, 0x12);

  /* DAC I2S setup: 32 bit word length, I2S format; MCLK / Fs = 256*/
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL1, DSP_DAC_WORD_LENGTH << 3 ); 
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL2, 0x02);

  /* DAC to output route mixer configuration */
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL16, 0x00);
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL17, 0x90);
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL20, 0x90);

  /* DAC and ADC use same LRCK, enable MCLK input; output resistance setup */
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL21, 0x80);
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL23, 0x00);

  /* DAC volume control: 0dB (maximum, unattenuated)  */
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL5, 0x00);
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL4, 0x00);

  /* power down ADC while configuring; volume: +9dB for both channels */
  res |= es_write_reg(ES8388_ADDR, ES8388_ADCPOWER, 0xff);
  res |= es_write_reg(ES8388_ADDR, ES8388_ADCCONTROL1, 0x33);

  /* select LINPUT2 / RINPUT2 as ADC input; stereo; 16 bit word length, format right-justified, MCLK / Fs = 256 */
  res |= es_write_reg(ES8388_ADDR, ES8388_ADCCONTROL2, 0x50);
  res |= es_write_reg(ES8388_ADDR, ES8388_ADCCONTROL3, 0x00);
  res |= es_write_reg(ES8388_ADDR, ES8388_ADCCONTROL4, DSP_DAC_WORD_LENGTH << 3 );
  res |= es_write_reg(ES8388_ADDR, ES8388_ADCCONTROL5, 0x02);

  /* set ADC volume */
  res |= es_write_reg(ES8388_ADDR, ES8388_ADCCONTROL8, DSP_ADC_ATTENUATE);
  res |= es_write_reg(ES8388_ADDR, ES8388_ADCCONTROL9, DSP_ADC_ATTENUATE);

  /* set LOUT1 / ROUT1 volume: 0dB (unattenuated) */
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL24, 0x21);
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL25, 0x21);

  /* power up and enable DAC; power up ADC (no MIC bias) */
  res |= es_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0x3c);
  res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL3, 0x00);
  res |= es_write_reg(ES8388_ADDR, ES8388_ADCPOWER, 0x09);

  i2c_driver_delete(I2C_NUM);
    
  return( res );
}


//------------------------------------------------------------------------------------ 
// Flash on board LED
//------------------------------------------------------------------------------------
static void esp_led_flash( bool flash, unsigned long duration ) {

  static  unsigned long  esp_led_flash_start  = 0;

  if( flash && (esp_led_flash_start == 0) ) {
    esp_led_flash_start = esp_timer_get_time()/1000;
    gpio_set_level(GPIO_NUM_22, 1);
  } else if( esp_timer_get_time()/1000 - esp_led_flash_start > duration ) {
    esp_led_flash_start = 0;
    gpio_set_level(GPIO_NUM_22, 0);
  }
}


//------------------------------------------------------------------------------------ 
// User input command processing
//------------------------------------------------------------------------------------
void dsp_command( char command ) {

  if( !dsp_ok_flag ) {
    SERIAL.printf( "E-DSP: DSP initialization error. Reload.\r\n" );
    return;
  }

  switch( command ) {
    case 'i' :
      dsp_filter_info( DSP_Channels );
      break;

    case 'e' :
      dsp_filters_enabled = true;
      SERIAL.printf("I-DSP: DSP processing ENABLED\r\n");
      break;

    case 'd' :
      dsp_filters_enabled = false;
      SERIAL.printf("I-DSP: DSP processing DISABLED\r\n");
      break;

    case 's' :
      dsp_output_enabled = false;
      SERIAL.printf("I-DSP: DSP is now STOPPED\r\n");
      break;

    case 'r' :
      dsp_output_enabled = true;
      SERIAL.printf("I-DSP: DSP is now RUNNING\r\n");
      break;

    case 'p' :
      dsp_plot( DSP_Channels );
      break;
  }
}


//------------------------------------------------------------------------------------ 
// DSP initialization
//------------------------------------------------------------------------------------
esp_err_t dsp_init( TaskHandle_t* taskDSP ) {
  
  esp_err_t res = ESP_OK;
  
  SERIAL.printf("I-DSP: Initializing audio codec via I2C...\r\n");

  res |= es8388_init();
  
  if (res != ESP_OK) {
    SERIAL.printf("E-DSP: Audio codec initialization failed!\r\n");
    dsp_ok_flag = false;
    return( res );
  } else {
    SERIAL.printf("I-DSP: Audio codec initialization OK\r\n");
  }

  /*******************/

  SERIAL.printf("I-DSP: Initializing input I2S...\r\n");

  i2s_config_t i2s_read_config;

  i2s_read_config.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX);
  i2s_read_config.sample_rate = DSP_SAMPLE_RATE;
  i2s_read_config.bits_per_sample = DSP_BITS_PER_SAMPLE;
  i2s_read_config.communication_format = I2S_COMM_FORMAT_I2S;
  i2s_read_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  i2s_read_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;
  i2s_read_config.dma_buf_count = 3;
  i2s_read_config.dma_buf_len = I2S_READLEN;
  i2s_read_config.use_apll = 1;
  i2s_read_config.tx_desc_auto_clear = 1;
  i2s_read_config.fixed_mclk = 0;

  i2s_pin_config_t i2s_read_pin_config;
  i2s_read_pin_config.bck_io_num = GPIO_NUM_5;
  i2s_read_pin_config.ws_io_num = GPIO_NUM_25;
  i2s_read_pin_config.data_out_num = GPIO_NUM_26;
  i2s_read_pin_config.data_in_num = GPIO_NUM_35;

  i2s_driver_install(I2S_NUM, &i2s_read_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &i2s_read_pin_config);

  // set clipping LED to output
  gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT);

  /*******************/

  SERIAL.printf("I-DSP: Initializing MCLK output...\r\n");

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
  WRITE_PERI_REG(PIN_CTRL, 0xFFF0);

  // Set up DSP to run on core 0
  xTaskCreatePinnedToCore(
                    dsp_task,           /* Function to implement the task */
                    "DSP Loop",         /* Name of the task */
                    10000,              /* Stack size in words */
                    NULL,               /* Task input parameter */
                    tskIDLE_PRIORITY,   /* Priority of the task */
                    taskDSP,            /* Task handle. */
                    CORE_DSP );         /* Core running the DSP process */    

  return( ESP_OK );
}


//------------------------------------------------------------------------------------ 
// DSP processing initialization
//------------------------------------------------------------------------------------
static esp_err_t dsp_processing_init() {
  
  esp_err_t res = ESP_OK;

  SERIAL.printf("I-DSP: Setting up channels...\r\n");

  res = dsp_filter_init( DSP_Channels, BIQUAD_Filters, sizeof( BIQUAD_Filters )/sizeof( biquad_def_t ), FREQ_Filters, sizeof( FREQ_Filters )/sizeof( filter_def_t ) );
  if( res != ESP_OK ) {
    dsp_ok_flag = false;
    return( res );
  }

  dsp_filter_info( DSP_Channels );

  return( res );
}


//------------------------------------------------------------------------------------ 
// DSP task 
//------------------------------------------------------------------------------------
void dsp_task( void * pvParameters ) {
  
  size_t    i2s_bytes_read;
  size_t    i2s_bytes_written;
  bool      clip_flag;
  esp_err_t res;
  
  // Setup the DSP channels
  res = dsp_processing_init();
  
  if( res == ESP_OK ) {
    // Run DSP processing
    i2s_bytes_read  = I2S_READLEN;
    
    while( true ) {
     if( dsp_output_enabled ) {   
        // Read buffer
        i2s_read( I2S_NUM, i2s_input_buffer, I2S_READLEN, &i2s_bytes_read, 100 );
  
        // Apply filters to buffer
        clip_flag = false;           
        dsp_filter( DSP_Channels, i2s_input_buffer, i2s_output_buffer, i2s_bytes_read, dsp_filters_enabled, &clip_flag );
    
        // Write out buffer     
        i2s_write( I2S_NUM, i2s_output_buffer, i2s_bytes_read, &i2s_bytes_written, 100 );
        //i2s_write( I2S_NUM, i2s_input_buffer, i2s_bytes_read, &i2s_bytes_written, 100 );
      } else {
#ifdef DISPLAY_ON
        // Reset the display bars
        for( int channel_id = 0; channel_id < DSP_NUM_CHANNELS; ++ channel_id ) {       
          dsp_display_input( channel_id, 0.0, 0 );
          dsp_display_output( channel_id, 0.0, 0 );        
        }
        vTaskDelay( TASK_DELAY );        
#endif
      }     
      
      // Check clipping LED
      esp_led_flash( clip_flag, 100 );
    }
  } else {
#ifdef DISPLAY_ON
    dsp_display_error();
#endif      
    while( true ) {
      vTaskDelay( TASK_DELAY );
    }
  }
}
