#include "../H/ugens.h"
#include "../H/sfheader.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>

/* fplot syntax:

      fplot(genslot[, pause[, "plotcmds"]])

   fplot with just the <genslot> argument plots the function at that
   slot with ascii chars to stdout.

   fplot with more than one argument sends data to gnuplot.  The first
   argument tells gnuplot how long to keep the window up before quitting.
   (This is the "pause" parameter in gnuplot.)  The second, optional, 
   argument is any string that gnuplot understands after a plot command.
   This is most useful for controlling the type of plot, e.g., with lines,
   points, dots, impulses, etc.  See the gnuplot manual for more info.

   Examples:

      makegen(2, 10, 1000, 1)       [a sine wave]

      fplot(2)                      [an ascii plot]

      fplot(2, 10)                  [a gnuplot plot, on screen for 10 seconds,
                                     using gnuplot default for plot type]

      fplot(2, 10, "with lines")    [plot with lines, interpolating btw. points]
      fplot(2, 10, "with points")   [plot with points]
      fplot(2, 10, "with linespoints")   [plot with both lines and points]

   Other useful options are "with impulses," "with steps," "with dots,"
   "with boxes," etc.

   Note that gnuplot quits when <pause> seconds elapse.  But you can get rid
   of the window earlier by typing 'q' at it.

   If something goes wrong with the gnuplot syntax, there'll be some leftover
   files in the /tmp directory, with names like "rtcmix_plot_data_xAb5x8."

   -JGG, 12/10/01
*/

double
fplot(float p[], short n_args, double pp[])
{
	if (n_args > 1) {					/* use gnuplot */
		int genslot = (int) p[0];
		float *array = floc(genslot);
		if (array) {
			int i, len = fsize(genslot);
			int pause = (int) p[1];
			char cmd[256], *plotcmds;
			char data_file[256] = "/tmp/rtcmix_plot_data_XXXXXX";
			char cmd_file[256] = "/tmp/rtcmix_plot_cmds_XXXXXX";
			FILE *fdata, *fcmd;

			if (mkstemp(data_file) == -1 || mkstemp(cmd_file) == -1)
				die("fplot", "Can't make temp files for gnuplot.");

			fdata = fopen(data_file, "w");
			fcmd = fopen(cmd_file, "w");
			if (fdata == NULL || fcmd == NULL)
				die("fplot", "Can't open temp files for gnuplot.");

			for (i = 0; i < len; i++)
				fprintf(fdata, "%d %.6f\n", i, array[i]);
			fclose(fdata);

			plotcmds = (char *) ((int) pp[2]);
			fprintf(fcmd, 
						"set title \"Function %d\"\n"
						"set grid\n"
						"plot '%s' notitle %s\n"
						"pause %d\n"
						"!rm '%s' '%s'\n",
						genslot, data_file, n_args > 2 ? plotcmds : "",
						pause, data_file, cmd_file);
			fclose(fcmd);

			snprintf(cmd, 255, "gnuplot %s &", cmd_file);
			cmd[255] = 0;
			system(cmd);
		}
		else
			die(NULL, "You must make a gen before plotting it!");
	}
	else {
		float out[80],si,*f1,phase;
		int i,k,j,len,wave;
		static char line[80];

		wave = p[0];
		phase = 0;
		len = fsize(wave);
		si = (float) len/79.;
		f1 = (float *) floc(wave);
		for(i = 0; i < 80; i++) {
			out[i] =  oscil(1.,si,f1,len,&phase);
		}
		si = 1./11.;
		phase = 1.;
		for(i = 0; i < 23; i++) {
			k = 0;
			for(j = 0; j < 79; j++) {
				if((out[j]>=phase) && (out[j]<phase+si)) {
					line[j] = (out[j+1] > phase+si) ? '/' : '-';
					if(out[j+1] < phase) line[j] = '\\';
					k = j;
					}
							 else if (((out[j]<phase)&&(out[j+1]>phase+si)) ||
					((out[j]>phase+si)&&(out[j+1]<phase))) {
						line[j] = '|';
				 k = j;
				 }
				else line[j] = ' ';
			}
			if ((0>=phase) && (0<phase+si)) {
					for(j = 0; j < 79; j++) line[j] = '-';
					k = 78;
					}
			line[k+1] = '\0';
			puts(line);
			phase = phase - si;
		}
	}
	return 0.0;
}

