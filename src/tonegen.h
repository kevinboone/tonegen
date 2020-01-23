/*============================================================================

  tonegen 
  tonegen.h
  Copyright (c)2020 Kevin Boone, GPL v3.0

============================================================================*/

#pragma once

#include <stdint.h>
#include "defs.h"

// Types of sound available
typedef enum {sound_type_random=0, sound_type_sweep, sound_type_silence,
  sound_type_noise, sound_type_buzz, sound_type_tone} 
  SoundType;

// Types of waveform available
typedef enum {waveform_sine=0, waveform_square}
  Waveform;

BEGIN_DECLS

BOOL       tonegen_setup_sound (snd_pcm_t **handle, const char *device, 
             snd_pcm_sframes_t *period_size);

void      tonegen_play_sound (snd_pcm_t *handle, SoundType sound_type, 
              Waveform waveform, int volume,
              const int duration, const int sub_duration, const int f1, 
              const int f2, snd_pcm_sframes_t period_size);

void      tonegen_wait (snd_pcm_t *handle);

END_DECLS


