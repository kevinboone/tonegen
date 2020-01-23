/*==========================================================================

  boilerplate
  usage.c
  Copyright (c)2020 Kevin Boone
  Distributed under the terms of the GPL v3.0

==========================================================================*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "feature.h" 
#include "usage.h" 


/*==========================================================================
  usage_show
==========================================================================*/
void usage_show (FILE *fout, const char *argv0)
  {
  fprintf (fout, "Usage: %s [options]\n", argv0);
  fprintf (fout, "  -b,--buzz=time,f1       play buzz of f1 Hz\n");
  fprintf (fout, "  -d,--device=D           set ALSA device\n");
  fprintf (fout, "  -h,--help               show this message\n");
  fprintf (fout, "  -l,--list={sounds}      list of sounds -- see manual\n");
  fprintf (fout, "  -n,--noise=time         play noise\n");
  fprintf (fout, "  -o,--log-level=N        log level, 0-5 (default 2)\n");
  fprintf (fout, "  -r,--random=time,time2,f1,f2\n");
  fprintf (fout, "     play random tones of length time2, in range f1-f2 Hz\n");
  fprintf (fout, "  -s,--sweep=time,f1      play sweep from f1 to f2\n");
  fprintf (fout, "  -t,--tone=time,f1       play constant tone of f1 Hz\n");
  fprintf (fout, "  -v,--version            show version\n");
  fprintf (fout, "  -w,--wave=N             waveform number\n");
  fprintf (fout, "All times are in msec, all frequencies in Hz\n");
  }

 
