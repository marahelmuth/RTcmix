/* ADSR Subclass of the Envelope Class, by Perry R. Cook, 1995-96
   This is the traditional ADSR (Attack, Decay, Sustain, Release) envelope.
   It responds to simple KeyOn and KeyOff messages, keeping track of it's
   state. There are two tick (update value) methods, one returns the value,
   and other returns the state (0 = A, 1 = D, 2 = S, 3 = R).
*/

#define USE_ADSR 1
#include "ADSR.h"    


ADSR :: ADSR() : Envelope()
{
   target = (MY_FLOAT) 0.0;
   value = (MY_FLOAT) 0.0;
   attackRate = (MY_FLOAT) 0.001;
   decayRate = (MY_FLOAT) 0.001;
   sustainLevel = (MY_FLOAT) 0.5;
   releaseRate = (MY_FLOAT) 0.01;
   state = ATTACK;
}


ADSR :: ~ADSR()
{
   /* Nothing to do here */
}


void ADSR :: keyOn()
{
   target = (MY_FLOAT) 1.0;
   rate = attackRate;
   state = ATTACK;
}


void ADSR :: keyOff()
{
   target = (MY_FLOAT) 0.0;
   rate = releaseRate;
   state = RELEASE;
}


void ADSR :: setAttackRate(MY_FLOAT aRate)
{
   if (aRate < 0.0) {
      fprintf(stderr, "Negative rates not allowed! Correcting...\n");
      attackRate = -aRate;
   }
   else
      attackRate = aRate;
}


void ADSR :: setDecayRate(MY_FLOAT aRate)
{
   if (aRate < 0.0) {
      fprintf(stderr, "Negative rates not allowed! Correcting...\n");
      decayRate = -aRate;
   }
   else
      decayRate = aRate;
}


void ADSR :: setSustainLevel(MY_FLOAT aLevel)
{
   if (aLevel < 0.0 ) {
      fprintf(stderr, "Sustain level out of range! Correcting...\n");
      sustainLevel = (MY_FLOAT) 0.0;
   }
   else
      sustainLevel = aLevel;
}


void ADSR :: setReleaseRate(MY_FLOAT aRate)
{
   if (aRate < 0.0) {
      fprintf(stderr, "Negative rates not allowed! Correcting...\n");
      releaseRate = -aRate;
   }
   else
      releaseRate = aRate;
}


void ADSR :: setAttackTime(MY_FLOAT aTime)
{
   if (aTime < 0.0) {
      fprintf(stderr, "Negative times not allowed! Correcting...\n");
      attackRate = (1.0 / SR) / -aTime;
   }
   else
      attackRate = (1.0 / SR) / aTime;
}


void ADSR :: setDecayTime(MY_FLOAT aTime)
{
   if (aTime < 0.0) {
      fprintf(stderr, "Negative times not allowed! Correcting...\n");
      decayRate = (1.0 / SR) / -aTime;
   }
   else
      decayRate = (1.0 / SR) / aTime;
}


void ADSR :: setReleaseTime(MY_FLOAT aTime)
{
   if (aTime < 0.0) {
      fprintf(stderr, "Negative times not allowed! Correcting...\n");
      releaseRate = (1.0 / SR) / -aTime;
   }
   else
      releaseRate = (1.0 / SR) / aTime;
}


void ADSR :: setAllTimes(
   MY_FLOAT attTime, MY_FLOAT decTime, MY_FLOAT susLevel, MY_FLOAT relTime)
{
   this->setAttackTime(attTime);
   this->setDecayTime(decTime);
   this->setSustainLevel(susLevel);
   this->setReleaseTime(relTime);
}


void ADSR :: setTarget(MY_FLOAT aTarget)
{
   target = aTarget;
   if (value < target) {
      state = ATTACK;
      this->setSustainLevel(target);
      rate = attackRate;
   }
   if (value > target) {
      this->setSustainLevel(target);
      state = DECAY;
      rate = decayRate;
   }
}


void ADSR :: setValue(MY_FLOAT aValue)
{
   state = SUSTAIN;
   target = aValue;
   value = aValue;
   this->setSustainLevel(aValue);
   rate = (MY_FLOAT) 0.0;
}


MY_FLOAT ADSR :: tick()
{
   if (state == ATTACK) {
      value += rate;
      if (value >= target) {
         value = target;
         rate = decayRate;
         target = sustainLevel;
         state = DECAY;
      }
   }
   else if (state == DECAY) {
      value -= decayRate;
      if (value <= sustainLevel) {
         value = sustainLevel;
         rate = (MY_FLOAT) 0.0;
         state = SUSTAIN;
      }
   }
   else if (state == RELEASE) {
      value -= releaseRate;
      if (value <= 0.0) {
         value = (MY_FLOAT) 0.0;
         state = END;
      }
   }
   return value;
}


int ADSR :: informTick()
{
   this->tick();
   return state;
}


MY_FLOAT ADSR :: lastOut()
{
   return value;
}


