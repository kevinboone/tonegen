/*==========================================================================

  tonegen 
  program.c
  Copyright (c)2020 Kevin Boone
  Distributed under the terms of the GPL v3.0

  This file contains the main body of the program. By the time
  program_run() has been called, RC files will have been read and comand-
  line arguments parsed, so all the contextual information will be in the
  ProgramContext. Logging will have been initialized, so the log_xxx
  methods will work, and be filtered at the appopriate levels.

  This files essentially processes the command-line arguments and decides
  what sounds to play. The real work is done in tonegen.c

==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <wchar.h>
#include <time.h>
#include <alsa/asoundlib.h>
#include "program_context.h" 
#include "feature.h" 
#include "program.h" 
#include "console.h" 
#include "string.h" 
#include "numberformat.h" 
#include "tonegen.h" 

// Largest possible number of number arguments in a command-line
//   value
#define MAX_NUM_ARGS 10

/*==========================================================================
  program_parse_nums
  Note that this method cannot fail, in itself. Any bad numbers are
   just skipped. We might end up with an empty list of arguments,
   but that's something for the caller to figure out
==========================================================================*/
int program_parse_nums (const char *_s, int nums[])
  {
  LOG_IN
  int ret = 0;
  char *s = strdup (_s);
  log_debug ("%s s=%s", __PRETTY_FUNCTION__, s);

  char *tok = strtok (s, ","); 
  int i = 0;
  while (tok && i < MAX_NUM_ARGS)
    {
    uint64_t num = 0;
     if (numberformat_read_integer (tok, &num, TRUE))
      {
      nums[i] = (int)num;
      i++;
      }
    else
      log_warning ("Argument %s is not a number -- ignoring it", tok);
    tok = strtok (NULL, ",");
    }
  ret = i; 
  free (s);
  return ret;
  LOG_OUT
  }

/*==========================================================================
  program_play_sound
==========================================================================*/
void program_play_sound (snd_pcm_t *handle, SoundType sound_type, 
     Waveform w, int volume,
     int *nums, int args, snd_pcm_sframes_t period_size)
  {
  switch (sound_type)
    {
    case sound_type_tone: 
      if (args == 2)
        tonegen_play_sound (handle, sound_type_tone, w, volume,
           nums[0], 0, nums[1], 0, period_size); 
      else
        log_error 
           (VERB_TONE "takes two values: duration (ms), frequency (Hz)");
      break;
    
    case sound_type_buzz: 
      if (args == 2)
        tonegen_play_sound (handle, sound_type_buzz, w, volume,
           nums[0], 0, nums[1], 0, period_size); 
      else
        log_error 
           (VERB_BUZZ " takes two values: duration (ms), frequency (Hz)");
      break;

    case sound_type_noise:
      if (args == 1)
        tonegen_play_sound (handle, sound_type_noise, w, volume,
           nums[0], 0, 0, 0, period_size); 
      else
        log_error 
           (VERB_NOISE " takes value: duration (ms)");
      break;

    case sound_type_silence:
      if (args == 1)
        tonegen_play_sound (handle, sound_type_silence, w, volume,
           nums[0], 0, 0, 0, period_size); 
      else
        log_error 
           (VERB_QUIET " takes value: duration (ms)");
      break;

    case sound_type_sweep:
      if (args == 3)
        tonegen_play_sound (handle, sound_type_sweep, w, volume,
           nums[0], 0, nums[1], nums[2], period_size); 
      else
        log_error 
           (VERB_SWEEP 
             " takes three values: duration (ms), start(Hz), end(Hz)");
       break;

    case sound_type_random:
      if (args == 4)
        tonegen_play_sound (handle, sound_type_random, w, volume,
           nums[0], nums[1], nums[2], nums[3], period_size); 
      else
        log_error 
           (VERB_RANDOM " takes three values: duration (ms), min(Hz), max(Hz)");
      break;
    }
  }

/*==========================================================================
  program_play_list
==========================================================================*/
void program_play_list (snd_pcm_t *handle, const char *arg, 
    snd_pcm_sframes_t period_size, Waveform init_w, int init_vol)
  {
  LOG_IN
  char *s = NULL;
  if (strcmp (arg, "-") == 0)
    {
    // Use stdin
    int c;
    int buffsize = 10;
    int i = 0;
    char *buff = malloc (buffsize + 1);
    while ((c = getc (stdin)) > 0)
      {
      if (i > buffsize)
        {
        buffsize *= 2;
        buff = realloc (buff, buffsize + 1);
        }
      buff[i] = c;
      i++;
      }
    buff[i] = 0;
    buff = realloc (buff, strlen (buff) + 10);
    strcat (buff, " stop");
    s = strdup (buff);
    free (buff);
    }
  else
    {
    s = malloc (strlen (arg) + 10);
    strcpy (s, arg);
    strcat (s, " stop"); 
    }

  Waveform w = init_w; 
  int vol = init_vol;
  
  BOOL stop = FALSE;
  char *tok = strtok (s, " \t\n,");
  int mode = 0;
  int args = 0;
  SoundType t = -1;
  int nums[MAX_NUM_ARGS];
  if (tok) do
    {
    log_debug ("tok=%s", tok);
    if (mode == 0) // looking for verb 
      {
      if (strcmp (tok, VERB_TONE) == 0)
        t = sound_type_tone;
      else if (strcmp (tok, VERB_NOISE) == 0)
        t = sound_type_noise;
      else if (strcmp (tok, VERB_BUZZ) == 0)
        t = sound_type_buzz;
      else if (strcmp (tok, VERB_QUIET) == 0)
        t = sound_type_silence;
      else if (strcmp (tok, VERB_RANDOM) == 0)
        t = sound_type_random;
      else if (strcmp (tok, VERB_SWEEP) == 0)
        t = sound_type_sweep;
      else if (strcmp (tok, VERB_WAVE) == 0)
        t = 100;
      else if (strcmp (tok, VERB_VOLUME) == 0)
        t = 101;

      if (t >= 0)
        {
        log_debug ("got verb, looking for number");
        mode = 1;
        }
      else
        {
        log_error ("%s is neither a sound type nor a number", tok);
        }
      }
    else if (mode == 1) // Expecting number
      {
      log_debug ("Expecting number, tok=%s", tok);
      uint64_t v;
      SoundType t2 = -2;
      if (strcmp (tok, VERB_TONE) == 0)
        t2 = sound_type_tone;
      else if (strcmp (tok, VERB_NOISE) == 0)
        t2 = sound_type_noise;
      else if (strcmp (tok, "stop") == 0)
        t2 = -1; 
      else if (strcmp (tok, VERB_BUZZ) == 0)
        t2 = sound_type_buzz;
      else if (strcmp (tok, VERB_QUIET) == 0)
        t2 = sound_type_silence;
      else if (strcmp (tok, VERB_RANDOM) == 0)
        t2 = sound_type_random;
      else if (strcmp (tok, VERB_SWEEP) == 0)
        t2 = sound_type_sweep;
      else if (strcmp (tok, VERB_WAVE) == 0)
        t2 = 100; // Ugh -- tidy this up
      else if (strcmp (tok, VERB_VOLUME) == 0)
        t2 = 101; // Ugh -- tidy this up
      if (t2 != -2)
        {
        log_debug ("Got %s whilst expecting number", tok);
        if (args > 0)
          {
          if (t == 100)
            {
            if (args == 1)
              {
              w = (Waveform) nums[0];
              }
            else
              log_error (VERB_WAVE " takes one argument -- 0 or 1");
            }
          else if (t == 101)
            {
            if (args == 1)
              {
              vol = nums[0];
              if (vol > 100) vol = 100;
              if (vol < 0) vol = 0;
              }
            else
              log_error (VERB_VOLUME " takes one argument -- 0 or 1");
            }
          else
            {
            program_play_sound (handle, t, w, vol,
              nums, args, period_size); 
            }
          log_debug ("got %d args, sound %d", args, t);
          args = 0;
          mode = 1;
          }
        t = t2; 
        }
      else if (numberformat_read_integer (tok, &v, TRUE))
        {
        if (args < MAX_NUM_ARGS)
          nums[args] = v;
        args++;
        }
      else
        {
        log_warning ("%s is neither a sound type nor a number", tok); 
        }
      if (t == -1) stop = TRUE;
      }

    tok = strtok (NULL, " \t,\n");
    } while (tok && !stop); 

  free (s);
  LOG_OUT
  }


/*==========================================================================
  program_run

  The return value will eventually become the exit value from the program.
==========================================================================*/
int program_run (ProgramContext *context)
  {
  LOG_IN
  //char ** const argv = program_context_get_nonswitch_argv (context);
  //int argc = program_context_get_nonswitch_argc (context);

  srand (time (NULL));

  Waveform w = program_context_get_integer (context, VERB_WAVE, 0);
  int volume = program_context_get_integer (context, VERB_VOLUME, 100);

  snd_pcm_t *handle;
  snd_pcm_sframes_t period_size;
  const char *device = program_context_get (context, "device"); 
  if (!device) device = "default";
  if (tonegen_setup_sound (&handle, device, &period_size))
    {
    int nums [MAX_NUM_ARGS];

    log_debug ("period_size=%ld\n", period_size);

    const char *v;
    if ((v = program_context_get (context, VERB_TONE)))
      {
      int args = program_parse_nums (v, nums);
      if (args == 2)
        {
        program_play_sound (handle, sound_type_tone, w, volume,
           nums, 2, period_size); 
        tonegen_wait (handle);
        }
      else
        log_error 
           ("--" VERB_TONE " takes two values: duration (ms), frequency (Hz)");
      }
    else if ((v = program_context_get (context, VERB_BUZZ)))
      {
      int args = program_parse_nums (v, nums);
      if (args == 2)
        {
        program_play_sound (handle, sound_type_buzz, w, volume,
           nums, 2, period_size); 
        tonegen_wait (handle);
        }
      else
        log_error 
           (VERB_BUZZ " takes two values: duration (ms), frequency (Hz)");
      }
    else if ((v = program_context_get (context, VERB_NOISE)))
      {
      int args = program_parse_nums (v, nums);
      if (args == 1)
        {
        program_play_sound (handle, sound_type_noise, w, volume,
           nums, 1, period_size); 
        tonegen_wait (handle);
        }
      else
        log_error 
           ("--" VERB_NOISE " takes value: duration (ms)");
      }
    else if ((v = program_context_get (context, VERB_QUIET)))
      {
      int args = program_parse_nums (v, nums);
      if (args == 1)
        {
        program_play_sound (handle, sound_type_silence, w, volume,
           nums, 1, period_size); 
        tonegen_wait (handle);
        }
      else
        log_error 
           ("--" VERB_QUIET " takes value: duration (ms)");
      }
    else if ((v = program_context_get (context, VERB_SWEEP)))
      {
      int args = program_parse_nums (v, nums);
      if (args == 3)
        {
        program_play_sound (handle, sound_type_sweep, w, volume,
           nums, 3, period_size); 
        tonegen_wait (handle);
        }
      else
        log_error 
           ("--" VERB_SWEEP 
              " takes three values: duration (ms), start(Hz), end(Hz)");
      }
    else if ((v = program_context_get (context, VERB_RANDOM)))
      {
      int args = program_parse_nums (v, nums);
      if (args == 4)
        {
        program_play_sound (handle, sound_type_random, w, volume,
           nums, 4, period_size); 
        tonegen_wait (handle);
        }
      else
        log_error 
           ("--" VERB_RANDOM 
        " takes four values: duration (ms), section (ms), start(Hz), end(Hz)");
      }
    else if ((v = program_context_get (context, VERB_LIST)))
      {
      program_play_list (handle, v, period_size, w, volume); 
      tonegen_wait (handle);
      }

    snd_pcm_close (handle);
    }
  else
    {
    // Error message will already have been displayed
    log_debug ("tonegen_setup_sound failed");
    }

  LOG_OUT
  return 0;
  }

