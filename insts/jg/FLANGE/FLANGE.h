#include <objlib.h>

class FLANGE : public Instrument {
   int     inchan, insamps, skip, flangetype;
   float   amp, resonance, moddepth, modspeed, spread, wetdrymix, maxdelsamps;
   float   *in, *amparray, amptabs[2];
   Butter  *filt;
   ZComb   *zcomb;
   ZNotch  *znotch;
   OscilN  *modoscil;

public:
   FLANGE();
   virtual ~FLANGE();
   int init(float *, int);
   int run();
};

