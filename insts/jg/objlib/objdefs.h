#if !defined(__objdefs_h)
#define __objdefs_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <rtdefs.h>       // in RTcmix/H


// #define NDEBUG     /* define to disable asserts */

#define DEFAULT_CONTROL_RATE  200

/* Yer Basic Trigonometric constants  */
#if !defined(PI)
  #define PI (MY_FLOAT) 3.14159265359
#endif
#define TWO_PI (MY_FLOAT) 6.28318530718
#define ONE_OVER_TWO_PI (MY_FLOAT) 0.15915494309
#define SQRT_TWO (MY_FLOAT) 1.4142135623730950488

/* Machine dependent stuff, possibly useful for optimization.
 * For example, changing double to float here increasesf
 * performance (speed) by a whopping 4-6% on 486-flavor machines.
 * BUT!! a change from float to double here increases speed by
 * 30% or so on SGI machines.
*/
//#define MY_FLOAT      double
//#define MY_FLOAT_SIZE 8

#define MY_FLOAT      float
#define MY_FLOAT_SIZE 4

/* States for Envelopes, etc. */
#define ATTACK  0
#define DECAY   1
#define SUSTAIN 2
#define RELEASE 3
#define END     4

#endif

