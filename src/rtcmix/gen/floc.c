#include "../H/ugens.h"
#include <stdio.h>

/* these 3 defined in makegen.c */
extern float *farrays[];
extern int sizeof_farray[];
extern int f_goto[];

/* Returns the address of function number genno, or NULL if the
   function array doesn't exist.

   NOTE: It's the responsiblity of instruments to deal with a
   missing gen, either by using die() or supplying a default
   and alerting the user with advise().
*/
float *floc(int genno)
{
   int index = f_goto[genno];
   if (sizeof_farray[index] == 0)
      return NULL;
   return farrays[index];
}

