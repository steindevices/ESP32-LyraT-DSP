#include <ArduinoOTA.h>                 // OTA library
#include "credentials.h"
#include "dsp_process.h"

//------------------------------------------------------------------------------------ 
// Common data
//------------------------------------------------------------------------------------ 
char    message[256];


//------------------------------------------------------------------------------------ 
// TelnetSpy setup
//------------------------------------------------------------------------------------ 
TelnetSpy       SerialAndTelnet;

static void setupTelnetSpy() {

  char msg[] = "Successfully connected to ESP32 LyraT DSP Processor.\r\n";

  // Welcome message
  SerialAndTelnet.setWelcomeMsg( msg );   

  // Set the buffer size to 2000 characters
  SerialAndTelnet.setBufferSize( 2000 );
}

//------------------------------------------------------------------------------------ 
// TelnetSpy loop
//------------------------------------------------------------------------------------
static void loopTelnetSpy() {
  SerialAndTelnet.handle();
}


//------------------------------------------------------------------------------------ 
// SERIAL setup
//------------------------------------------------------------------------------------ 
static void setupSerial() {
  SERIAL.begin( 115200 );
  delay( 100 ); 
  
  SERIAL.println( "Serial connection established" );
}

//------------------------------------------------------------------------------------ 
// SERIAL input
//------------------------------------------------------------------------------------ 
static void loopSerialInput() {

  String input_text;
  
  if( SERIAL.available() > 0 ) {
    input_text = SERIAL.readStringUntil( '\n' );

    // Remove line feed character '\r'
    if( input_text[ input_text.length() - 1 ] == '\r' ) {
      input_text.remove( input_text.length() - 1 );
    }
    
    if( input_text.equals( "i" ) ) {        // Display filter information
      dsp_command( 'i' );
    } else if( input_text.equals( "e" ) ) { // Enable filter
      dsp_command( 'e' );
    } else if( input_text.equals( "d" ) ) { // Disable filter
      dsp_command( 'd' );
    } else if( input_text.equals( "s" ) ) { // Stop filter 
      dsp_command( 's' );
    } else if( input_text.equals( "r" ) ) { // Run filter        
      dsp_command( 'r' );
    } else if( input_text.equals( "p" ) ) { // Plot transfer function curve
      dsp_command( 'p' );         
    } else if( input_text.equals( "u" ) ) { // Override filters
      dsp_command( 'u' );       
    } else if( input_text.equals( "restart" ) ) { // Reboot DSP
      ESP.restart();      
    } else if( input_text.equals( "?" ) ) { // Show help
      SERIAL.println( "i - Display filter information" );
      SERIAL.println( "e - Enable filters" );
      SERIAL.println( "d - Disable filters" );
      SERIAL.println( "s - Stop output" );
      SERIAL.println( "r - Run output" ); 
      SERIAL.println( "u - Update filters" );     
      SERIAL.println( "p - Plot transfer function curve" );
      SERIAL.println( "restart - Reboot DSP" );      
    } else {
      SERIAL.println( "??? Unknown command" );
    }
  }
}


//------------------------------------------------------------------------------------ 
// WiFi setup
//------------------------------------------------------------------------------------ 
char    strIPAddress[16];

static void setupWiFi() {
  SERIAL.print( "Connecting to " );
  SERIAL.print( WIFI_SSID );

  WiFi.mode( WIFI_STA );
  WiFi.begin( WIFI_SSID, WIFI_PASSWORD );  
  
  while( WiFi.status() != WL_CONNECTED ) {
    SERIAL.print( "." );
    delay( 500 );
  }
  
  SERIAL.println();
  SERIAL.print( "Connected to " );
  SERIAL.println( WIFI_SSID );
  SERIAL.print( "IP Address is: " );
  strcpy( strIPAddress, WiFi.localIP().toString().c_str() );  
  SERIAL.println( strIPAddress );
}


//------------------------------------------------------------------------------------ 
// WiFi loop
//------------------------------------------------------------------------------------
char    strConnectTimestamp[21];

static void loopCheckWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    
    SERIAL.println( "Wi-Fi is disconnected. Reconnecting..." );
    WiFi.disconnect();
    setupWiFi();  
  }
}


//------------------------------------------------------------------------------------ 
// FreeRTOS setup
//------------------------------------------------------------------------------------
TaskHandle_t        taskMain;
TaskHandle_t        taskDSP;

static void setupFreeRTOS() {

  // Set up Main loop to run on core 1
  xTaskCreatePinnedToCore(
                      main_task,          /* Function to implement the task */
                      "MAIN Loop",        /* Name of the task */
                      10000,              /* Stack size in words */
                      NULL,               /* Task input parameter */
                      1,                  /* Priority of the task */
                      &taskMain,          /* Task handle. */
                      CORE_MAIN );        /* Core where the task should run */
}


//------------------------------------------------------------------------------------ 
// OTA update setup
//------------------------------------------------------------------------------------ 
void setupOTA() {
 
  ArduinoOTA.setHostname( "ESP32_LyraT_DSP" );

  ArduinoOTA.onStart([]() {
    String      type;
    if( ArduinoOTA.getCommand() == U_FLASH ) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    SERIAL.println( "Start updating " + type );
    
    // Suspend DSP processing
    vTaskSuspend( taskDSP );    
  });
  
  ArduinoOTA.onEnd([]() {  
    SERIAL.println( "\nEnd" );

    // Resume DSP processing
    vTaskResume( taskDSP );     
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    SERIAL.printf( "Progress: %u%%\r", ( progress/(total/100) ) );
  });
  
  ArduinoOTA.onError([]( ota_error_t error ) {
    SERIAL.printf( "Error[%u]: ", error );
    if( error == OTA_AUTH_ERROR ) {
      SERIAL.println( "Auth Failed" );
    } else if( error == OTA_BEGIN_ERROR ) {
      SERIAL.println( "Begin Failed" );
    } else if (error == OTA_CONNECT_ERROR) {
      SERIAL.println( "Connect Failed" );
    } else if (error == OTA_RECEIVE_ERROR) {
      SERIAL.println( "Receive Failed" );
    } else if (error == OTA_END_ERROR) {
      SERIAL.println( "End Failed" );
    }
  });

  ArduinoOTA.begin();
}


//------------------------------------------------------------------------------------ 
// OTA update loop
//------------------------------------------------------------------------------------
static void loopArduinoOTA() {
  ArduinoOTA.handle();  
}


//------------------------------------------------------------------------------------ 
// DSP setup
//------------------------------------------------------------------------------------
static int setupDSP() {
  return( dsp_init( &taskDSP ) );   
}


#ifdef DISPLAY_ON
//------------------------------------------------------------------------------------ 
// Display setup
//------------------------------------------------------------------------------------
static void setupDisplay() {
  dsp_display_init();
}


//------------------------------------------------------------------------------------ 
// Display loop
//------------------------------------------------------------------------------------
static void loopDisplay() {
  dsp_display_loop();
}
#endif


//------------------------------------------------------------------------------------ 
// Main setup
//------------------------------------------------------------------------------------ 

void setup() {
  setupFreeRTOS();  
}


//------------------------------------------------------------------------------------ 
// Main loop
//------------------------------------------------------------------------------------
void loop() { 
}


//------------------------------------------------------------------------------------ 
// Main task
//------------------------------------------------------------------------------------
static void main_task( void * pvParameters ) {

#ifdef WIFI_ON  
  setupWiFi();
  setupOTA();
  setupTelnetSpy();    
#endif
  setupSerial();
  setupDSP();  
#ifdef DISPLAY_ON  
  setupDisplay();
#endif  

  while( true ) {
#ifdef WIFI_ON
    loopCheckWiFi();
    loopArduinoOTA();
    loopTelnetSpy(); 
#endif
    loopSerialInput();
#ifdef DISPLAY_ON  
    loopDisplay();
#endif
    vTaskDelay( TASK_DELAY );
  }
}
