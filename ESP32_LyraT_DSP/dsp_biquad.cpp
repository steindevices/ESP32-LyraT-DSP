#include "dsp_process.h"

#define _PI        3.14159265358979323846   /* pi    */
#define _LN2       0.69314718055994530942   /* ln(2) */
#define _LN_CONST  (_LN2 / 2)               /* ln(2)/2 */
#define _TWO_PI    (_PI * 2)                /* 2*pi */
#define _FS        DSP_SAMPLE_RATE          /* sampling frequency */


//------------------------------------------------------------------------------------
// Calculate BiQuad values for the passeed filter
//------------------------------------------------------------------------------------
esp_err_t dsp_get_biquad( filter_def_t* filter, double* coeffs )
{
  double  b0, b1, b2, a0, a1, a2;   // BiQuad coefficients
  double  A, W0, S, C, alpha;       // Intermediate calculation values

  W0 = (_TWO_PI * filter->frequency) / _FS;
  S  = sin(W0);
  C  = cos(W0);
  alpha = S / (2*filter->Q);
  A = pow( 10, filter->gain/40.0 );

  switch( filter->filter_type ) {

    case DSP_FILTER_LOW_PASS:
      b0 =  (1 - C)/2;
      b1 =   1 - C;
      b2 =  (1 - C)/2;
      a0 =   1 + alpha;
      a1 =   2*C;
      a2 = -(1 - alpha);
      break;

    case DSP_FILTER_HIGH_PASS:
      b0 =  (1 + C)/2;
      b1 = -(1 + C);
      b2 =  (1 + C)/2;
      a0 =  1 + alpha;
      a1 =  2*C;
      a2 = -(1 - alpha);
      break;
      
    case DSP_FILTER_BAND_PASS:
      b0 =  alpha;
      b1 =  0;
      b2 = -alpha;
      a0 =  1 + alpha;
      a1 =  2*C;
      a2 = -(1 - alpha);
      break;
      
    case DSP_FILTER_NOTCH:
      b0 =  1;
      b1 = -2*cos(W0);
      b2 =  1;
      a0 =  1 + alpha;
      a1 =  2*cos(W0);
      a2 =  -(1 - alpha);
      break;      

    case DSP_FILTER_APF:
      b0 =  1 - alpha;
      b1 = -2 * cos(W0);
      b2 =  1 + alpha;
      a0 =  1 + alpha;
      a1 =  2 * cos(W0);
      a2 =  -(1 - alpha);
      break;

    case DSP_FILTER_PEAK_EQ:
      b0 =  1 + alpha*A;
      b1 = -2 * cos(W0);
      b2 =  1 - alpha*A;
      a0 =  1 + alpha/A;
      a1 =  2 * cos(W0);
      a2 =  -(1 - alpha/A);
      break;      

    case DSP_FILTER_LOW_SHELF:
      b0 =    A*( (A+1) - (A-1)*cos(W0) + 2*sqrt(A)*alpha );
      b1 =  2*A*( (A-1) - (A+1)*cos(W0) );
      b2 =    A*( (A+1) - (A-1)*cos(W0) - 2*sqrt(A)*alpha );
      a0 =        (A+1) + (A-1)*cos(W0) + 2*sqrt(A)*alpha;
      a1 =    2*( (A-1) + (A+1)*cos(W0) );
      a2 =     -( (A+1) + (A-1)*cos(W0) - 2*sqrt(A)*alpha );
      break;      

    case DSP_FILTER_HIGH_SHELF:
      b0 =    A*( (A+1) + (A-1)*cos(W0) + 2*sqrt(A)*alpha );
      b1 = -2*A*( (A-1) + (A+1)*cos(W0) );
      b2 =    A*( (A+1) + (A-1)*cos(W0) - 2*sqrt(A)*alpha );
      a0 =        (A+1) - (A-1)*cos(W0) + 2*sqrt(A)*alpha;
      a1 =   -2*( (A-1) - (A+1)*cos(W0) );
      a2 =     -( (A+1) - (A-1)*cos(W0) - 2*sqrt(A)*alpha );
      break;      

    default:
      SERIAL.printf( "E-DSP: ERROR: Unknown BiQuad type '%d'\r\n", filter->filter_type );
      return( ESP_FAIL );    
  };

  // Normalize the BiQuad values
  a1 /= a0;
  a2 /= a0;
  b0 /= a0;
  b1 /= a0;
  b2 /= a0;

  // Return filter BiQuad values (
  coeffs[0] = b0;
  coeffs[1] = b1;
  coeffs[2] = b2;
  coeffs[3] = a1;
  coeffs[4] = a2;

  return( ESP_OK );
}
