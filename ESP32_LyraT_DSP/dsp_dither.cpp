#include "dsp_process.h"

#define NOISE_MASK    ((1 << DITHER_BITS) - 1)
  
uint32_t              u = 10, v = 99;


//------------------------------------------------------------------------------------ 
// 32-bit random number generator
//------------------------------------------------------------------------------------
static int dsp_rand() {

  int     rnd;

  v = 36969*(v & 65535) + (v >> 16);
  u = 18000*(u & 65535) + (u >> 16);

  rnd = u & NOISE_MASK;
  if( rnd > (NOISE_MASK+1) >> 1 ) {
    rnd = -rnd;
  }
  
  return( rnd );
}

//------------------------------------------------------------------------------------ 
// Add dither to the output to reduce noise impact
//------------------------------------------------------------------------------------
int32_t dsp_dither( int32_t sample ) {

  return( ((sample >> DITHER_BITS) << DITHER_BITS) + dsp_rand() );
}
