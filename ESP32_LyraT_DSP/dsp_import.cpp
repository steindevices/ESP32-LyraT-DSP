#define   IMPORT_MINIDSP

#include "dsp_process.h"

static char dsp_filter_import[] = 
#include "dsp_import.h"
;

static biquad_def_t   biquad_defs[DSP_MAX_FILTERS];


//------------------------------------------------------------------------------------
// Parse the filter data imported from the REW application
//------------------------------------------------------------------------------------
static biquad_def_t* dsp_import_REW( int* import_filter_count ) {

  char*         token;
  double        coeff;
  char          delimiters[10];
  int           num_filters;
  char*         str_end;  
  filter_def_t  filter;

  num_filters = 0;

  strcpy( delimiters, "\r\n" );
  
  // Read in header (3 records)
  token = strtok( dsp_filter_import, delimiters );
  token = strtok( NULL, delimiters );
  token = strtok( NULL, delimiters );

  // Read in filter records
  strcpy( delimiters, " \r\n\t" );
  token = strtok( NULL, delimiters );
    
  while( token != NULL ) {
       
    if( num_filters == DSP_MAX_FILTERS ) {
      SERIAL.printf( "E-DSP: ERROR: Maximum filters exceeded in REW import\r\n" );      
      return( NULL );
    }    
    
    // Discard next 3 tokens
    for( int i=1; i<=3; ++i ) {
      
      token = strtok( NULL, delimiters );
      if( token == NULL ) {
        SERIAL.printf( "E-DSP: ERROR: Invalid filter definition in REW file\r\n" );  
        return( NULL );
      }   
    }

    if( strcmp( token, "None" ) != 0 ) { 

      // Define the filter
      filter.filter_type = DSP_FILTER_PEAK_EQ;      
      filter.channel = DSP_ALL_CHANNELS;
#ifdef DOUBLE_PRECISION
      filter.precision = PRC_FLT;
#endif
  
      // Frequency
      token = strtok( NULL, delimiters );
      if( token == NULL ) {
        SERIAL.printf( "E-DSP: ERROR: Invalid frequency value in REW import\r\n" );
        return( NULL );
      } 
        
      filter.frequency = strtof( token, &str_end );
      
      // Check if valid number format
      if( *str_end != '\0' ) {
        SERIAL.printf( "E-DSP: ERROR: Invalid frequency value in REW import\r\n" );
        return( NULL );
      }      
  
      // Gain
      token = strtok( NULL, delimiters );
      if( token == NULL ) {
        SERIAL.printf( "E-DSP: ERROR: Invalid gain value in REW import\r\n" );
        return( NULL );
      }
       
      filter.gain = strtof( token, &str_end ); 

      // Check if valid number format
      if( *str_end != '\0' ) {
        SERIAL.printf( "E-DSP: ERROR: Invalid gain value in REW import\r\n" );
        return( NULL );
      }

      // Q
      token = strtok( NULL, delimiters );
      if( token == NULL ) {
        SERIAL.printf( "E-DSP: ERROR: Invalid Q value in REW import\r\n" );
        return( NULL );
      }

      filter.Q = strtof( token, &str_end );

      // Check if valid number format
      if( *str_end != '\0' ) {
        SERIAL.printf( "E-DSP: ERROR: Invalid Q value in REW import\r\n" );
        return( NULL );
      }      

      // Calculate the biquad for this filter
      dsp_get_biquad( &filter, &biquad_defs[num_filters].coeffs[0] ); 
      
#ifdef DOUBLE_PRECISION
      biquad_defs[num_filters].precision = PRC_FLT;
#endif
      biquad_defs[num_filters].channel = DSP_ALL_CHANNELS;            

      // Skip next value
      token = strtok( NULL, delimiters );              

      ++ num_filters;  
    }
    token = strtok( NULL, delimiters );              
  }
  
  *import_filter_count = num_filters;
  
  return( &biquad_defs[0] );  
}


//------------------------------------------------------------------------------------
// Parse the BiQuad data imported from the HouseCurve app
//------------------------------------------------------------------------------------
static biquad_def_t* dsp_import_HouseCurve( int* import_filter_count ) {
  
  char*         token;
  double        coeff;
  char*         str_end;
  char          delimiters[] = ", \r\n\t";
  int           num_filters;  
  const char*   prefix[5] = {"b0=", "b1=", "b2=", "a1=", "a2="}; 
  dsp_filter_t  filter;
   
  num_filters = 0;

  token = strtok( dsp_filter_import, delimiters );
  while( token != NULL ) {
    
    if( num_filters == DSP_MAX_FILTERS ) {
      SERIAL.printf( "E-DSP: ERROR: Maximum filters exceeded in HouseCurve import\r\n" );      
      return( NULL );
    }
    
    for( int i=0; i<5; ++i ) {
      token = strtok( NULL, delimiters );

      // Check if all values entered
      if( token == NULL ) {
        SERIAL.printf( "E-DSP: ERROR: Too few coefficients in HouseCurve import\r\n" );
        return( NULL );
      }

      // Check if valid prefixes
      if( strncmp( token, prefix[i], 3 )!= 0 ) {
        SERIAL.printf( "E-DSP: ERROR: Invalid input in HouseCurve import\r\n" );
        return( NULL );
      }
        
      // Convert token to double precision
      coeff = strtod( token+3, &str_end );

      // Check if valid number format
      if( *str_end != '\0' ) {
        SERIAL.printf( "E-DSP: ERROR: Invalid coefficient value in HouseCurve import\r\n" );
        return( NULL );
      }

      // Reverse sign of a1 and a2 biquads
      biquad_defs[num_filters].coeffs[i] = coeff; 
    }
#ifdef DOUBLE_PRECISION
    biquad_defs[num_filters].precision = PRC_FLT;
#endif
    biquad_defs[num_filters].channel = DSP_ALL_CHANNELS;
    
    token = strtok( NULL, delimiters ); 
    ++ num_filters;
  }

  *import_filter_count = num_filters;
  
  return( &biquad_defs[0] );
}


//------------------------------------------------------------------------------------
// Import filter set from include file
//------------------------------------------------------------------------------------
biquad_def_t* dsp_import_filters( int* import_filter_count ) {

#ifdef IMPORT_MINIDSP
  return( dsp_import_REW( import_filter_count ) );
#else
  return( dsp_import_HouseCurve( import_filter_count ) );
#endif
  return( NULL );
}
