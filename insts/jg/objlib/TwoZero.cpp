/*******************************************/
/*  Two Zero Filter Class,                 */
/*  by Perry R. Cook, 1995-96              */ 
/*  See books on filters to understand     */
/*  more about how this works.  Nothing    */
/*  out of the ordinary in this version.   */
/*******************************************/

#include "TwoZero.h"


TwoZero :: TwoZero() : Filter()
{
   inputs = new MY_FLOAT [2];
   zeroCoeffs[0] = (MY_FLOAT) 0.0;
   zeroCoeffs[1] = (MY_FLOAT) 0.0;
   gain = (MY_FLOAT) 1.0;
   this->clear();
   outputs = NULL;             // unused
}


TwoZero :: ~TwoZero()
{
   delete [] inputs;
}


void TwoZero :: clear()
{
   inputs[0] = (MY_FLOAT) 0.0;
   inputs[1] = (MY_FLOAT) 0.0;
   lastOutput = (MY_FLOAT) 0.0;
}


void TwoZero :: setZeroCoeffs(MY_FLOAT *coeffs)
{
   zeroCoeffs[0] = coeffs[0];
   zeroCoeffs[1] = coeffs[1];
}


void TwoZero :: setGain(MY_FLOAT aValue)
{
   gain = aValue;
}


// see csound areson (ugens5.c) for the friendlier version
// NO! arith in y0 = ... equation too different from tick below?


// NOTE: same as CLM zpolar filter -- see clm.html
// freq is angle and width (notchwidth) is radius of a zero
// 0 <= freq <= srate/2; 0 <= width < 1
// To get (approx) width from bandwidth (in hz): width = exp(-PI * bw / SR);

void TwoZero :: setFreqAndWidth(MY_FLOAT freq, MY_FLOAT width)
{
   zeroCoeffs[0] = (MY_FLOAT) -2.0 * width * cos(TWO_PI * (double)(freq / SR));
   zeroCoeffs[1] = width * width;
}


// y0 = g x(n) + a1 x(n-1) + a2 x(n-2)
// Note: history takes g x(n), not x(n)

MY_FLOAT TwoZero :: tick(MY_FLOAT sample)
{
   lastOutput = zeroCoeffs[0] * inputs[0];
   lastOutput += zeroCoeffs[1] * inputs[1];
   inputs[1] = inputs[0];
   inputs[0] = gain * sample;
   lastOutput += inputs[0];
   return lastOutput;
}


