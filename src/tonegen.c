/*==========================================================================

  tonegen 
  tonegen.c
  Copyright (c)2020 Kevin Boone
  Distributed under the terms of the GPL v3.0

  Functions for playing tones using ALSA

==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <wchar.h>
#include <time.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include "program_context.h" 
#include "feature.h" 
#include "program.h" 
#include "console.h" 
#include "string.h" 
#include "file.h" 
#include "list.h" 
#include "path.h" 
#include "numberformat.h" 
#include "tonegen.h" 

// Sample rate in Hz
#define RATE 48000

// Data format -- everybody should support signed 16-bit 
#define FORMAT SND_PCM_FORMAT_S16

// Output buffer size in usec
#define BUFFER_TIME 2000000

// Length of a single buffer update in usec. To get smooth whistles, this
//  needs to be < 20msec or so 
#define PERIOD_TIME 10000

/* ==========================================================================
  tonegen_generate_sine
  fill the buffer with sinewave, paying attention to the starting point
  (phase), which will have been carried forward from the previous
  period to avoid discontinuity
==========================================================================*/
static void tonegen_generate_sine (int volume, 
                 const snd_pcm_channel_area_t *areas,
		int count, double *_phase, int freq, int fade)
  {
  static double max_phase = 2. * M_PI;
  double phase = *_phase;
  double step = max_phase*freq/(double)RATE;
  unsigned char *samples[1];
  int steps [1];
  int format_bits = snd_pcm_format_width (FORMAT);
  unsigned int maxval = (1 << (format_bits - 1)) - 1;
  int bps = format_bits / 8; /* bytes per sample */
  int phys_bps = snd_pcm_format_physical_width (FORMAT) / 8;
  int big_endian = snd_pcm_format_big_endian (FORMAT) == 1;
  samples[0] = (((unsigned char *)areas[0].addr) 
    + (areas[0].first / 8));
  steps[0] = areas[0].step / 8;
  int start_count = count;

  int vol = maxval * volume / 100.0;  

  while (count-- > 0) 
    {
    int res, i;

    res = sin(phase) * vol; 
    if (freq == 0) res = 5;
    if (big_endian) 
      {
      if (fade)
        res = res * count / start_count;
      for (i = 0; i < bps; i++)
        *(samples [0] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
      } 
    else 
      {
      if (fade)
        res = res * count / start_count;
      for (i = 0; i < bps; i++)
        *(samples[0] + i) = (res >> i * 8) & 0xff;
      }
    samples[0] += steps[0];

    phase += step;
    if (phase >= max_phase)
      phase -= max_phase;
    }
  *_phase = phase;
  }

/* ==========================================================================
  tonegen_generate_square
  fill the buffer with sinewave, paying attention to the starting point
  (phase), which will have been carried forward from the previous
  period to avoid discontinuity
==========================================================================*/
static void tonegen_generate_square (int volume, 
                const snd_pcm_channel_area_t *areas,
		int count, double *_phase, int freq, int fade)
  {
  static double max_phase = 2. * M_PI;
  double phase = *_phase;
  double step = max_phase*freq/(double)RATE;
  unsigned char *samples[1];
  int steps [1];
  int format_bits = snd_pcm_format_width (FORMAT);
  unsigned int maxval = (1 << (format_bits - 1)) - 1;
  int bps = format_bits / 8; /* bytes per sample */
  int phys_bps = snd_pcm_format_physical_width (FORMAT) / 8;
  int big_endian = snd_pcm_format_big_endian (FORMAT) == 1;
  samples[0] = (((unsigned char *)areas[0].addr) 
    + (areas[0].first / 8));
  steps[0] = areas[0].step / 8;
  int start_count = count;
  int vol = volume * (int)maxval / 100;

  while (count-- > 0) 
    {
    int res, i;

    if (phase > M_PI) 
      res = vol;
    else 
      res = -vol;

    if (freq == 0) res = 5;
    if (big_endian) 
      {
      if (fade)
        res = res * count / start_count;
      for (i = 0; i < bps; i++)
        *(samples [0] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
      } 
    else 
      {
      if (fade)
        res = res * count / start_count;
      for (i = 0; i < bps; i++)
        *(samples[0] + i) = (res >> i * 8) & 0xff;
      }
    samples[0] += steps[0];

    phase += step;
    if (phase >= max_phase)
      phase -= max_phase;
    }
  *_phase = phase;
  }

/* ==========================================================================
  tonegen_generate_buzz
==========================================================================*/
static void tonegen_generate_buzz (const snd_pcm_channel_area_t *areas,
		int count, int freq, int fade)
  {
  static double max_phase = 2. * M_PI;
  double phase = 0;
  double step = max_phase*freq/(double)RATE;
  unsigned char *samples[1];
  int steps [1];
  int format_bits = snd_pcm_format_width (FORMAT);
  unsigned int maxval = (1 << (format_bits - 1)) - 1;
  int bps = format_bits / 8; /* bytes per sample */
  int phys_bps = snd_pcm_format_physical_width (FORMAT) / 8;
  int big_endian = snd_pcm_format_big_endian (FORMAT) == 1;
  samples[0] = (((unsigned char *)areas[0].addr) 
    + (areas[0].first / 8));
  steps[0] = areas[0].step / 8;
  int start_count = count;

  while (count-- > 0) 
    {
    int res, i;

    if (phase > M_PI)
      res = maxval / 4;
    else
      res = -maxval / 4;

    if (big_endian) 
      {
      if (fade)
        res = res * count / start_count;
      for (i = 0; i < bps; i++)
        *(samples [0] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
      } 
    else 
      {
      if (fade)
        res = res * count / start_count;
      for (i = 0; i < bps; i++)
        *(samples[0] + i) = (res >> i * 8) & 0xff;
      }
    samples[0] += steps[0];

    phase += step;
    if (phase >= max_phase)
      phase -= max_phase;
    }
  }

/* ==========================================================================
  tonegen_generate_silence
  fill the buffer with silence 
  Note that we must actively generate silence -- we can't just pause, 
  because the playback buffer would underrun
==========================================================================*/
static void tonegen_generate_silence (const snd_pcm_channel_area_t *areas, 
    int count)
  {
  unsigned char *samples[1];
  int steps [1];
  int format_bits = snd_pcm_format_width (FORMAT);
  int bps = format_bits / 8; /* bytes per sample */
  int phys_bps = snd_pcm_format_physical_width (FORMAT) / 8;
  int big_endian = snd_pcm_format_big_endian (FORMAT) == 1;
  samples[0] = (((unsigned char *)areas[0].addr) 
    + (areas[0].first / 8));
  steps[0] = areas[0].step / 8;
  samples[0] += 0; 

  while (count-- > 0) 
    {
    // We might think that zero would be a good sample value for silence but,  
    //  in fact, any constant value is silent. However, setting zero in my
    //  tests actually generates a low hiss -- no idea why
    int res = 5, i;

    if (big_endian) 
      {
      for (i = 0; i < bps; i++)
        *(samples [0] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
      } 
    else 
      {
      for (i = 0; i < bps; i++)
        *(samples[0] + i) = (res >> i * 8) & 0xff;
      }
    samples[0] += steps[0];

    }
  }


/* ==========================================================================
  tonegen_generate_noise
  fill the buffer with white noise 
==========================================================================*/
static void tonegen_generate_noise (const snd_pcm_channel_area_t *areas, 
    int count)
  {
  unsigned char *samples[1];
  int steps [1];
  int format_bits = snd_pcm_format_width (FORMAT);
  int bps = format_bits / 8; /* bytes per sample */
  int phys_bps = snd_pcm_format_physical_width (FORMAT) / 8;
  int big_endian = snd_pcm_format_big_endian (FORMAT) == 1;
  unsigned int maxval = (1 << (format_bits - 1)) - 1;
  samples[0] = (((unsigned char *)areas[0].addr) 
    + (areas[0].first / 8));
  steps[0] = areas[0].step / 8;
  samples[0] += 0; 

  while (count-- > 0) 
    {
    int i;
    int res = (double) rand() / RAND_MAX * (double) maxval;
    if (big_endian) 
      {
      for (i = 0; i < bps; i++)
        *(samples [0] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
      } 
    else 
      {
      for (i = 0; i < bps; i++)
        *(samples[0] + i) = (res >> i * 8) & 0xff;
      }
    samples[0] += steps[0];

    }
  }

  
/*=========================================================================
  tonegen_play_sound
=========================================================================*/
void tonegen_play_sound (snd_pcm_t *handle, SoundType sound_type, 
    Waveform waveform, int volume,
    const int duration, const int pitch_duration, const int f1, 
    const int f2, snd_pcm_sframes_t period_size)
  {
  double phase = 0;
  int16_t *ptr;
  int err, cptr;
  int16_t *samples;
  snd_pcm_channel_area_t *areas;

  int freq = f1;
  int time_per_period = PERIOD_TIME / 1000;
  int loops = duration / time_per_period;
  int f_increment = (f2 - f1) / loops;
  int loops_per_pitch_duration = pitch_duration / time_per_period;
  
  samples = malloc ((period_size * 
    snd_pcm_format_physical_width (FORMAT)) / 8);
  areas = malloc (sizeof (snd_pcm_channel_area_t));

  areas[0].addr = samples;
  areas[0].first = 0; 
  areas[0].step = snd_pcm_format_physical_width (FORMAT);

  int loop;
  for (loop = 0; loop < loops; loop++)
    {
    if (sound_type == sound_type_buzz)
      {
      if (loops_per_pitch_duration == 0 
           || (loop % loops_per_pitch_duration) == 0)
        {
        freq = f1 + (f2 - f1) * (double) rand() / RAND_MAX ;
        }
      tonegen_generate_buzz (areas, period_size, freq, loop == loops - 1);
      }
    else if (sound_type == sound_type_random)
      {
      if (loops_per_pitch_duration == 0 || 
           (loop % loops_per_pitch_duration) == 0)
        freq = f1 + (f2 - f1) * (double) rand() / RAND_MAX ;
      if (waveform == waveform_square)
        tonegen_generate_square (volume, areas, period_size, 
           &phase, freq, loop == loops - 1);
      else
        tonegen_generate_sine (volume, areas, period_size, 
           &phase, freq, loop == loops - 1);
      }
    else if (sound_type == sound_type_sweep)
      {
      freq += f_increment;
      if (waveform == waveform_square)
        tonegen_generate_square (volume, areas, period_size, 
           &phase, freq, loop == loops - 1);
      else
        tonegen_generate_sine (volume, areas, period_size, 
           &phase, freq, loop == loops - 1);
      }
    else if (sound_type == sound_type_tone)
      {
      if (waveform == waveform_square)
        tonegen_generate_square (volume, areas, period_size, &phase, 
          freq, loop == loops - 1);
      else
        tonegen_generate_sine (volume, areas, period_size, &phase, 
          freq, loop == loops - 1);
      }
    else if (sound_type == sound_type_noise)
      tonegen_generate_noise (areas, period_size);
    else
      tonegen_generate_silence (areas, period_size);
    ptr = samples;
    cptr = period_size;
    while (cptr > 0) 
      {
      err = snd_pcm_writei (handle, ptr, cptr);
      if (err == -EAGAIN)
        continue;
      if (err < 0) 
        {
        printf ("hello %d\n", err);
        // Underrun -- should never happen
        break; 
        }
      ptr += err;
      cptr -= err;
      }
    }

  free(areas);
  free(samples);
  }


/*==========================================================================
  tonegen_setup_hw_params
 
  Set up the ALSA hardware paramters, and return the available buffer
    size and period size -- which may not be exactly what were requested
==========================================================================*/
static int tonegen_set_hwparams(snd_pcm_t *handle, 
                snd_pcm_hw_params_t *params, 
                snd_pcm_sframes_t *buffer_size, 
                snd_pcm_sframes_t *period_size)
  {
  unsigned int rrate;
  snd_pcm_uframes_t size;
  int err, dir = 0;
  err = snd_pcm_hw_params_any(handle, params);
  if (err < 0) 
    {
    log_error ("No configurations available: %s",
       snd_strerror(err));
    return err;
    }
  err = snd_pcm_hw_params_set_access (handle, params, 
    SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) 
    {
    log_error ("Access type not available: %s", snd_strerror(err));
    return err;
    }
  err = snd_pcm_hw_params_set_format (handle, params, FORMAT);
  if (err < 0) 
    {
    log_error ("Sample format not available: %s", 
      snd_strerror(err));
    return err;
    }
  err = snd_pcm_hw_params_set_channels (handle, params, 1);
  if (err < 0) 
    {
    log_error ("Can't set mono playback: %s",  
      snd_strerror(err));
    return err;
    }
  rrate = RATE;
  err = snd_pcm_hw_params_set_rate_near (handle, params, &rrate, 0);
  if (err < 0) 
    {
    log_error ("Rate %iHz not available: %s", 
        RATE, snd_strerror(err));
    return err;
    }
  if (rrate != RATE) 
    {
    log_warning ("Warning: Rate not available (requested %iHz, get %iHz)\n", 
       RATE, err);
    }
  unsigned int buffer_time = BUFFER_TIME;
  err = snd_pcm_hw_params_set_buffer_time_near (handle, params, 
    &buffer_time, &dir);
  if (err < 0) 
    {
    log_error ("Unable to set buffer time %i: %s\n", 
      buffer_time, snd_strerror(err));
    return err;
    }
  err = snd_pcm_hw_params_get_buffer_size(params, &size);
  if (err < 0) 
    {
    log_error ("Unable to get buffer size: %s\n", 
      snd_strerror(err));
    return err;
    }
  *buffer_size = size;

  //snd_pcm_sframes_t buffer_size = size;
  unsigned int period_time = PERIOD_TIME;
  err = snd_pcm_hw_params_set_period_time_near (handle, params, 
      &period_time, &dir);
  if (err < 0) 
    {
    log_error ("Unable to set period time %i: %s\n", 
      period_time, snd_strerror(err));
    return err;
    }
  err = snd_pcm_hw_params_get_period_size (params, &size, &dir);
  if (err < 0) 
    {
    log_error ("Unable to get period size: %s\n", 
      snd_strerror(err));
    return err;
    }
  *period_size = size;
  err = snd_pcm_hw_params (handle, params);
  if (err < 0) 
    {
    log_error ("Unable to set hwparams: %s\n", snd_strerror(err));
    return err;
    }
  return 0;
  }

/*==========================================================================
  tonegen_setup_sw_params
  
  Set up buffering based on the buffer size and period size returned
    by the hardware
==========================================================================*/
static int tonegen_set_swparams(snd_pcm_t *handle, 
        snd_pcm_sw_params_t *swparams, 
        snd_pcm_sframes_t buffer_size, snd_pcm_sframes_t period_size)
  {
  int err;
  err = snd_pcm_sw_params_current(handle, swparams);
  if (err < 0) 
    {
    log_error ("Unable to determine current swparams: %s\n", 
      snd_strerror(err));
    return err;
    }
  err = snd_pcm_sw_params_set_start_threshold (handle, swparams, 
    (buffer_size / period_size) * period_size);
  if (err < 0) 
    {
    log_error ("Unable to set start threshold: %s", 
      snd_strerror(err));
    return err;
    }
  err = snd_pcm_sw_params_set_avail_min (handle, swparams, period_size);
  if (err < 0) 
    {
    log_error ("Unable to set avail min for playback: %s", 
      snd_strerror(err));
    return err;
    }
  err = snd_pcm_sw_params(handle, swparams);
  if (err < 0)  
    {
    log_error ("Unable to set sw params for playback: %s", 
      snd_strerror(err));
    return err;
    }
  return 0;
  }


/*==========================================================================
  tonegen_setup_sound 
==========================================================================*/
BOOL tonegen_setup_sound (snd_pcm_t **handle, const char *device, 
     snd_pcm_sframes_t *period_size)
  {
  LOG_IN
  BOOL ret = TRUE;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_sw_params_t *swparams;
  snd_pcm_hw_params_alloca (&hwparams);
  snd_pcm_sw_params_alloca (&swparams);
  int err;
  if ((err = snd_pcm_open (handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
    {
    log_error ("Can't open playback device %s: %s", device, 
      snd_strerror(err));
    ret = FALSE;  
    }

  snd_pcm_sframes_t buffer_size = 0;
  if (ret && (err = tonegen_set_hwparams (*handle, hwparams, 
        &buffer_size, period_size)) < 0) 
    {
    log_error ("Can't set hwparams: %s", snd_strerror(err));
    ret = FALSE;  
    }
  if (ret && (err = tonegen_set_swparams (*handle, swparams,  
      buffer_size, *period_size)) < 0) 
    {
    log_error ("Can't set swparams: %s", snd_strerror(err));
    ret = FALSE;
    }
  LOG_OUT
  return ret;
  }
  
/*==========================================================================
  tonegen_wait
  Wait for playback buffer to empty. 
==========================================================================*/
void tonegen_wait (snd_pcm_t *handle)
  {
  snd_pcm_drain (handle);
  snd_pcm_state_t state; 
  int wait_count = 0;
  do
    {
    state = snd_pcm_state (handle);
    usleep (100000);
    wait_count++;
    // Something is odd here -- the state doesn't seem
    //   to change as it should. We have to just use a
    //   delay arrived at by trial and error
    } while (state == SND_PCM_STATE_RUNNING && wait_count < 50); 
  }


