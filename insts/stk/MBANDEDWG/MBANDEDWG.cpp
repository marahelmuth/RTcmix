/* MBANDEDWG - the "BandedWG" physical model instrument in
	Perry Cook/Gary Scavone's "stk" (synthesis tookkit).

   p0 = output start time
   p1 = duration
   p2 = amplitude multiplier
   p3 = frequency (Hz)
   p4 = strike position (0.0-1.0)
   p5 = pluck flag (0: no pluck, 1: pluck)
   p6 = max velocity (0.0-1.0, I think...)
   p7 = preset #
         - Uniform Bar = 0
         - Tuned Bar = 1
         - Glass Harmonica = 2
         - Tibetan Bowl = 3
   p8 = bow pressure (0.0-1.0) 0.0 == strike only
   p9 = mode resonance (0.0-1.0) 0.99 == normal strike
   p10 = integration constant (0.0-1.0) 0.0 == normal?
   p11 = percent of signal to left output channel [optional, default is .5]

   Assumes function table 1 is amplitude curve for the note.
   Or you can just call setline. If no setline or function table 1, uses
   flat amplitude curve.

   Assumes function table 2 is velocity curve for the note (interacts with
   p8)
*/

#include <Stk.h>
#include <BandedWG.h> // from the stk library

#include <iostream.h>        /* needed only for cout, etc. if you want it */
#include <stdio.h>
#include <stdlib.h>
#include <ugens.h>
#include <Ougens.h>
#include <math.h>
#include <mixerr.h>
#include <Instrument.h>      /* the base class for this instrument */
#include "MBANDEDWG.h"         /* declarations for this instrument class */
#include <rt.h>
#include <rtdefs.h>



MBANDEDWG :: MBANDEDWG() : Instrument()
{
}

MBANDEDWG :: ~MBANDEDWG()
{
	delete theBar;
}

int MBANDEDWG :: init(double p[], int n_args)
{
	Stk::setSampleRate(SR);

	nsamps = rtsetoutput(p[0], p[1], this);

	amp = p[2] * 30.0; // for some reason this needs normalizing...
	if (floc(1)) { // the amplitude array has been created in the score
		theEnv = new Ooscili(SR, 1.0/p[1], 1);
	} else {
		amparray[0] = amparray[1] = 1.0;
		advise("MBANDEDWG", "Setting phrase curve to all 1's.");
		theEnv = new Ooscili(SR, 1.0/p[1], amparray);
	}

	if (floc(2)) { // the velocity array has been created in the score
		theVeloc = new Ooscili(SR, 1.0/p[1], 2);
	} else {
		velarray[0] = velarray[1] = 1.0;
		advise("MBANDEDWG", "Setting velocity curve to all 1's.");
		theVeloc = new Ooscili(SR, 1.0/p[1], velarray);
	}

	theBar = new BandedWG(); // 50 Hz is default lowest frequency
	theBar->setFrequency(p[3]);
	theBar->setStrikePosition(p[4]);

	if (p[5] < 1.0) theBar->setPluck(false);
	else theBar->setPluck(true);

	theBar->setBowPressure(p[8]);
	theBar->setModeResonance(p[9]);
	theBar->setIntegration(p[10]);

	theBar->setPreset((int)p[7]);
	theBar->noteOn(p[3], p[6]); // this will initialize the maxVelocity

	pctleft = n_args > 11 ? p[11] : 0.5;                /* default is .5 */

	return nsamps;
}

int MBANDEDWG :: run()
{
	int   i;
	float out[2];

	for (i = 0; i < framesToRun(); i++) {
		out[0] = theBar->tick(theVeloc->next()) * amp * theEnv->next(currentFrame());

		if (outputChannels() == 2) {
			out[1] = out[0] * (1.0 - pctleft);
			out[0] *= pctleft;
		}

		rtaddout(out);
		increment();
	}

	return framesToRun();
}


Instrument *makeMBANDEDWG()
{
	MBANDEDWG *inst;

	inst = new MBANDEDWG();
	inst->set_bus_config("MBANDEDWG");

	return inst;
}

void rtprofile()
{
	RT_INTRO("MBANDEDWG", makeMBANDEDWG);
}


