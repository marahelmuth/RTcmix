/* SubSampled Noise Generator Class, by Perry R. Cook, 1995-96
   White noise as often as you like.
   (Seed arg and method, and minor tweaks, added by JGG.)

   Returns pseudo-random numbers in the range (-1.0, 1.0).

   Note: This is like randh in other packages, but without amp scaling
   in tick and with freq specied in samples rather than Hz.  -JGG
*/

#if !defined(__SubNoise_h)
#define __SubNoise_h

#include "Noise.h"

class SubNoise : public Noise
{
  protected:  
     int counter;
     int howOften;
  public:
    SubNoise();
    ~SubNoise();
    SubNoise(int subSample, unsigned int aSeed = 0);
    void setHowOften(int howOft);
    MY_FLOAT tick();
};

#endif
