/* VOCODESYNTH - channel vocoder driving an oscillator bank

   Performs a filter-bank analysis of the input channel (the modulator),
   and uses the time-varying energy measured in the filter bands to trigger
   wavetable notes with corresponding frequencies (the carrier).  Note that
   this is not the same as a vocoder whose carrier is processed by a second
   filter bank (as is the case with the VOCODE and VOCODE2 instruments).

   p0  = output start time
   p1  = input start time (must be 0 for aux bus)
   p2  = duration
   p3  = amplitude multiplier (post-processing)

   Two ways to specify filter bank center frequencies:

     (1) Spread evenly above a given center frequency:
            p4 = number of filter bands (greater than 0)
            p5 = lowest filter center frequency (in Hz or oct.pc)
            p6 = center frequency spacing multiplier (greater than 1)
                 (multiplies each cf by this to get next higher cf)

     (2) A list of center frequencies, given in function table 4:
            p4 = 0 (must be zero: tells instrument to look for function table)
            p5 = transposition of function table, in oct.pc  (subtly
                 different from p7, which leaves cf's alone while transposing
                 the carrier oscillators.)
            p6 = function table format (0: octave.pc, 1: linear octave)
                 (This applies to values below 15; values 15 and above are
                 interpreted as Hz, regardless of p6.)
            Number of filter bands determined by length of function table.

   p7  = amount to transpose carrier oscillators (in Hz or oct.pc)
   p8  = filter bandwidth proportion of center frequency (greater than 0)
   p9  = power gauge window length (seconds)  [optional; default is 0.01]
         Determines how often changes in modulator power are measured.
   p10 = smoothness -- how much to smooth the power gauge output (0-1)
         (this has more effect for longer window length (p9) times)
         [optional; default is 0.5]
   p11 = threshold -- below which no synthesis for a band occurs (0-1)
         [optional; default is 0.0]
   p12 = attack time -- how long it takes the oscillator for a band to turn
         on fully once the modulator power for that band rises above the
         threshold [optional; default is 0.001]
   p13 = release time -- how long it takes the oscillator for a band to turn
         off fully once the modulator power for that band falls below the
         threshold [optional; default is 0.01]
   p14 = amount of high-passed modulator signal to mix with output
         (amplitude multiplier)  [optional; default is 0]
   p15 = cutoff frequency for high pass filter applied to modulator.
         This pfield ignored if p10 is zero.  [optional; default is 5000 Hz]
   p16 = input channel [optional; default is 0]
   p17 = percent to left channel  [optional; default is 0.5]

   Function tables:

      1  amplitude curve for note.  This curve, combined with the amplitude
         multiplier, affect the signal after synthesis.

      2  waveform for carrier oscillators

      3  scaling curve for carrier notes, as <frequency, amplitude> pairs

      4  list of center frequencies, for use with the second method of
         specifying center frequencies

   NOTES:

     -  If using first method for specifying center frequencies...
           p6 = 2.0 will make a stack of octaves
           p6 = 1.5 will make a stack of perfect (Pythagorian) fifths
        Use this to get stacks of an equal tempered interval (in oct.pc):
           p6 = cpspch(interval) / cpspch(0.0)

     - If using second method for specifying center frequencies...
       Make function table 4, for example, by using gen 2.  So, to make
       cf's from a list of pitches in oct.pc notation:
          num_bands = 5
          makegen(2, 2, num_bands, 8.00, 8.07, 9.00, 9.07, 10.02)
       You can transpose these by the number of semitones given in p5.
       Or, to specify cf's in Hz:
          makegen(2, 2, 9, 100, 200, 300, 400, 500, 600, 700, 800, 900)
       Transposition (still specified as oct.pc) works here also, and it
       preserves harmonic structure.

   John Gibson (johgibso@indiana.edu), 8/7/03.
*/
#include "VOCODESYNTH.h"


/* ----------------------------------------------------------- VOCODESYNTH -- */
VOCODESYNTH :: VOCODESYNTH() : Instrument()
{
   branch = 0;
   inringdown = 0;
   in = NULL;
   amptable = NULL;
   scaletable = NULL;
   hipassmod = NULL;
}


/* ---------------------------------------------------------- ~VOCODESYNTH -- */
VOCODESYNTH :: ~VOCODESYNTH()
{
   delete [] in;
   for (int i = 0; i < numbands; i++) {
      delete carrier_osc[i];
      delete modulator_filt[i];
      delete gauge[i];
      delete smoother[i];
      delete envelope[i];
   }
   delete amptable;
   if (scaletable)
      free(scaletable);    // NB: allocated by sys/makegen.c: resample_gen()
   delete hipassmod;
}


/* -------------------------------------------------------- compare_floats -- */
/* Comparison function for qsort call below. */
int compare_floats(const void *a, const void *b)
{
   const float *fa = (const float *) a;
   const float *fb = (const float *) b;

   return (*fa > *fb) - (*fa < *fb);
}


/* ------------------------------------------------------------------ init -- */
int VOCODESYNTH :: init(float p[], int n_args)
{
   int   i, window_len, wavetablelen = 0;
   float outskip, inskip, dur, lowcf, spacemult, bwpct, cf[MAXOSC];
   float carrier_transp, responsetime, attack_time, release_time, hipasscf;

   outskip = p[0];
   inskip = p[1];
   dur = p[2];
   amp = p[3];
   numbands = (int) p[4];
   lowcf = p[5];
   spacemult = p[6];
   carrier_transp = p[7];
   bwpct = p[8];
   responsetime = n_args > 9 ? p[9] : 0.01;        // default: .01 secs
   smoothness = n_args > 10 ? p[10] : 0.5;         // default: .5 secs
   threshold = n_args > 11 ? p[11] : 0.0;          // default: 0
   attack_time = n_args > 12 ? p[12] : 0.001;      // default: 0.001
   release_time = n_args > 13 ? p[13] : 0.01;      // default: 0.01
   hipass_mod_amp = n_args > 14 ? p[14] : 0.0;     // default: 0
   hipasscf = n_args > 15 ? p[15] : 5000.0;        // default: 5000 Hz
   inchan = n_args > 16 ? (int) p[16] : 0;         // default: left
   pctleft = n_args > 17 ? p[17] : 0.5;            // default: center

   if (bwpct <= 0.0)
      die("VOCODESYNTH", "Bandwidth proportion must be greater than 0.");

   window_len = (int) (responsetime * SR + 0.5);
   if (window_len < 2) {
      warn("VOCODESYNTH", "Response time too short ... changing to %.8f.",
                                                                     2.0 / SR);
      // Otherwise, can get ear-splitting output.
      window_len = 2;
   }

   if (smoothness < 0.0 || smoothness > 1.0)
      die("VOCODESYNTH", "Smoothness must be between 0 and 1.");
   /* Filter pole coefficient is non-linear -- very sensitive near 1, and
      not very sensitive below .5, so we use a log to reduce this nonlinearity
      for the user.  Constrain to range [1, 10], then take log10.
   */
   smoothness = (float) log10((double) ((smoothness * 9.9) + 0.1));
   if (smoothness > 0.9999)   // 1.0 results in all zeros for power signal
      smoothness = 0.9999;
   DPRINT1("smoothness: %f\n", smoothness);

   if (threshold < 0.0 || threshold > 1.0)
      die("VOCODESYNTH", "Threshold must be between 0 and 1.");
   threshold *= 32768.0;
   if (attack_time < 0.0)
      die("VOCODESYNTH", "Attack time must be positive.");
   if (release_time < 0.0)
      die("VOCODESYNTH", "Release time must be positive.");
   attack_rate = attack_time ? (1.0 / SR) / attack_time : 1.0;
   release_rate = release_time ? (1.0 / SR) / release_time : 1.0;

   if (hipass_mod_amp > 0.0) {
      hipassmod = new Butter();
      hipassmod->setHighPass(hipasscf);
   }

   nsamps = rtsetoutput(outskip, dur + release_time, this);
   rtsetinput(inskip, this);

   if (outputChannels() > 2)
      die("VOCODESYNTH", "Output must be either mono or stereo.");
   if (inchan >= inputChannels())
      die("VOCODESYNTH", "You asked for channel %d of a %d-channel file.",
                                                      inchan, inputChannels());

   /* <numbands> pfield lets user specify filter center frequencies either by
      interval, in the form of a frequency multiplier, or by function table.
   */
   if (numbands > 0) {           // specify by interval
      if (numbands > MAXOSC)
         die("VOCODESYNTH", "Can only use %d filters.", MAXOSC);
      if (spacemult <= 1.0)
         die("VOCODESYNTH",
               "Center frequency spacing factor must be greater than 1.");
      if (lowcf < 15.0)                // interpreted as oct.pc
         lowcf = cpspch(lowcf);
      advise("VOCODESYNTH", "Building center freqs above %g...", lowcf);
      for (i = 0; i < numbands; i++)
         cf[i] = lowcf * (float) pow((double) spacemult, (double) i);
   }
   else if (numbands == 0) {     // specify by function table
      float transp = lowcf;            // p5 and p6 change meaning
      int format = (int) spacemult;
      float *freqtable = floc(4);
      if (freqtable == NULL)
         die("VOCODESYNTH",
                  "You haven't made the center freq. function (table 4).");
      numbands = fsize(4);
      if (numbands > MAXOSC)
         die("VOCODESYNTH", "Can only use %d filters.", MAXOSC);
      advise("VOCODESYNTH", "Reading center freqs from function table 4...");
      for (i = 0; i < numbands; i++) {
         float freq = freqtable[i];
         if (freq < 15.0) {            // interpreted as oct.pc or linoct
            if (format == 0) {            // oct.pc
               if (transp)
                  cf[i] = cpsoct(octpch(freq) + octpch(transp));
               else
                  cf[i] = cpspch(freq);
            }
            else {                        // linoct
               if (transp)
                  cf[i] = cpsoct(freq + octpch(transp));
               else
                  cf[i] = cpsoct(freq);
            }
         }
         else {                        // interpreted as Hz
            if (transp)
               cf[i] = cpsoct(octcps(freq) + octpch(transp));
            else
               cf[i] = freq;
         }
      }
      // sort cf array, so that scaling curve will work correctly
      qsort(&cf, numbands, sizeof(float), compare_floats);
   }
   else
      die("VOCODESYNTH", "<numbands> must be zero or more.");

   for (i = 0; i < numbands; i++) {
      if (cf[i] > SR * 0.5) {
         warn("VOCODESYNTH", "A cf was above Nyquist. Correcting...");
         cf[i] = SR * 0.5;
      }
   }

   advise("VOCODESYNTH", "centerfreq  bandwidth");
   for (i = 0; i < numbands; i++)
      advise(NULL, "              %10.4f %10.4f", cf[i], bwpct * cf[i]);

   if (carrier_transp)
      carrier_transp = octpch(carrier_transp);

   insamps = (int) (dur * SR + 0.5);
   skip = (int) (SR / (float) resetval);
   aamp = amp;                  // in case amptable == NULL

   // function tables -------------------------------------------------------

   float *function = floc(1);
   if (function) {
      int len = fsize(1);
      amptable = new TableL(dur, function, len);
   }
   else
      advise("VOCODESYNTH", "Setting phrase curve to all 1's.");

   car_wavetable = floc(2);
   if (car_wavetable)
      wavetablelen = fsize(2);

   function = floc(3);
   if (function) {
      int len = fsize(3);
      scaletable = resample_gen(function, len, numbands, LINEAR_INTERP);
      if (scaletable == NULL)
         die("VOCODESYNTH", "No memory for resizing scaling table.");
   }
   else
      die("VOCODESYNTH", "You haven't made the scaling curve (table 3).");

   // make filters, oscillators ---------------------------------------------

   for (i = 0; i < numbands; i++) {
      float thecf = cf[i];

      modulator_filt[i] = new Butter();
      modulator_filt[i]->setBandPass(thecf, bwpct * thecf);

      if (carrier_transp)
         thecf = cpsoct(octcps(thecf) + carrier_transp);

      if (car_wavetable)
         carrier_osc[i] = new OscilN(0, car_wavetable, wavetablelen);
      else
         carrier_osc[i] = new OscilN(0, NULL, 2000);
      carfreq[i] = thecf;

      gauge[i] = new RMS();
      gauge[i]->setWindowSize(window_len);

      if (smoothness > 0.0) {
         smoother[i] = new OnePole();
         smoother[i]->setPole(smoothness);
      }

      envelope[i] = new Envelope();
      state[i] = belowThreshold;
   }

   return nsamps;
}


/* ------------------------------------------------------------------- run -- */
int VOCODESYNTH :: run()
{
   float modsig, out[MAXBUS];

   if (in == NULL)            // first time, so allocate it
      in = new float [RTBUFSAMPS * inputChannels()];

   Instrument :: run();

   const int frames = framesToRun() * inputChannels();
   rtgetin(in, this, frames);

   for (int i = 0; i < frames; i += inputChannels()) {
      if (currentFrame() < insamps) {
         if (--branch < 0) {
            if (amptable)
               aamp = amptable->tick(currentFrame(), 1.0) * amp;
            branch = skip;
         }
         modsig = in[i + inchan];
      }
      else {
         inringdown = 1;
         modsig = 0.0;
      }

      out[0] = 0.0;
      for (int j = 0; j < numbands; j++) {
         float power, car = 0.0;

         if (inringdown) {
            if (state[j] == aboveThreshold) {      // turn this band off
               state[j] = belowThreshold;
               envelope[j]->setRate(release_rate);
               envelope[j]->keyOff();
            }
            power = lastpower[j];
         }
         else {
            float mod = modulator_filt[j]->tick(modsig);
            power = gauge[j]->tick(mod);
            if (smoothness > 0.0)
               power = smoother[j]->tick(power);
            lastpower[j] = power;                  // save for ringdown
            if (power >= threshold) {
               if (state[j] == belowThreshold) {
                  state[j] = aboveThreshold;
                  envelope[j]->setRate(attack_rate);
                  envelope[j]->keyOn();
               }
            }
            else {
               if (state[j] == aboveThreshold) {
                  state[j] = belowThreshold;
                  envelope[j]->setRate(release_rate);
                  envelope[j]->keyOff();
               }
            }
         }
         float env = envelope[j]->tick();
         if (env > 0.0) {
            float gain = power * env * scaletable[j];
            car = carrier_osc[j]->tick(carfreq[j], gain);
         }
         out[0] += car;
//printf("%d: power=%f, state=%d, env=%f\n", currentFrame(), power, state[j], env);
      }

      if (hipass_mod_amp > 0.0) {
         float hpmodsig = hipassmod->tick(modsig);
         out[0] += hpmodsig * hipass_mod_amp;
      }

      out[0] *= aamp;
      if (outputChannels() == 2) {
         out[1] = out[0] * (1.0 - pctleft);
         out[0] *= pctleft;
      }

      rtaddout(out);
      increment();
   }

   return framesToRun();
}


/* ------------------------------------------------------- makeVOCODESYNTH -- */
Instrument *makeVOCODESYNTH()
{
   VOCODESYNTH *inst;

   inst = new VOCODESYNTH();
   inst->set_bus_config("VOCODESYNTH");

   return inst;
}


/* ------------------------------------------------------------- rtprofile -- */
void
rtprofile()
{
   RT_INTRO("VOCODESYNTH", makeVOCODESYNTH);
}


