/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */


#ifndef _FLUID_RVOICE_H
#define _FLUID_RVOICE_H

#include "fluidsynth_priv.h"
#include "fluid_iir_filter.h"
#include "fluid_adsr_env.h"
#include "fluid_lfo.h"
#include "fluid_phase.h"
#include "fluid_sfont.h"

typedef struct _fluid_rvoice_envlfo_t fluid_rvoice_envlfo_t;
typedef struct _fluid_rvoice_dsp_t fluid_rvoice_dsp_t;
typedef struct _fluid_rvoice_buffers_t fluid_rvoice_buffers_t;
typedef struct _fluid_rvoice_t fluid_rvoice_t;

/* Smallest amplitude that can be perceived (full scale is +/- 0.5)
 * 16 bits => 96+4=100 dB dynamic range => 0.00001
 * 24 bits => 144-4 = 140 dB dynamic range => 1.e-7
 * 1.e-7 * 2 == 2.e-7 :)
 */
#define FLUID_NOISE_FLOOR 2.e-7


enum fluid_loop {
  FLUID_UNLOOPED = 0,
  FLUID_LOOP_DURING_RELEASE = 1,
  FLUID_NOTUSED = 2,
  FLUID_LOOP_UNTIL_RELEASE = 3
};

/*
 * rvoice ticks-based parameters
 * These parameters must be updated even if the voice is currently quiet.
 */
struct _fluid_rvoice_envlfo_t
{
	/* Note-off minimum length */
	unsigned int ticks;
	unsigned int noteoff_ticks;      

	/* vol env */
        fluid_adsr_env_t volenv;

	/* mod env */
        fluid_adsr_env_t modenv;
	fluid_real_t modenv_to_fc;
	fluid_real_t modenv_to_pitch;

	/* mod lfo */
        fluid_lfo_t modlfo;
	fluid_real_t modlfo_to_fc;
	fluid_real_t modlfo_to_pitch;
	fluid_real_t modlfo_to_vol;

	/* vib lfo */
        fluid_lfo_t viblfo;
	fluid_real_t viblfo_to_pitch;
};

/*
 * rvoice parameters needed for dsp interpolation
 */
struct _fluid_rvoice_dsp_t
{
	/* interpolation method, as in fluid_interp in fluidsynth.h */
	int interp_method;
	fluid_sample_t* sample;
	int check_sample_sanity_flag;   /* Flag that initiates, that sample-related parameters
					   have to be checked. */

	/* sample and loop start and end points (offset in sample memory).  */
	int start;
	int end;
	int loopstart;
	int loopend;	/* Note: first point following the loop (superimposed on loopstart) */
	enum fluid_loop samplemode;

	/* Stuff needed for portamento calculations */
	fluid_real_t pitchoffset;        /* the portamento range in midicents */
	fluid_real_t pitchinc;           /* the portamento increment in midicents */

	/* Stuff needed for phase calculations */

	fluid_real_t pitch;              /* the pitch in midicents */
	fluid_real_t root_pitch_hz;
	fluid_real_t output_rate;

	/* Stuff needed for amplitude calculations */

	int has_looped;                 /* Flag that is set as soon as the first loop is completed. */
	fluid_real_t attenuation;        /* the attenuation in centibels */
	fluid_real_t prev_attenuation;   /* the previous attenuation in centibels 
					used by fluid_rvoice_multi_retrigger_attack() */
	fluid_real_t min_attenuation_cB; /* Estimate on the smallest possible attenuation
					  * during the lifetime of the voice */
	fluid_real_t amplitude_that_reaches_noise_floor_nonloop;
	fluid_real_t amplitude_that_reaches_noise_floor_loop;
	fluid_real_t synth_gain; 	/* master gain */


	/* Dynamic input to the interpolator below */

	fluid_real_t *dsp_buf;		/* buffer to store interpolated sample data to */

	fluid_real_t amp;                /* current linear amplitude */
	fluid_real_t amp_incr;		/* amplitude increment value for the next FLUID_BUFSIZE samples */

	fluid_phase_t phase;             /* the phase (current sample offset) of the sample wave */
	fluid_real_t phase_incr;	/* the phase increment for the next FLUID_BUFSIZE samples */
	int is_looping;

};

/* Currently left, right, reverb, chorus. To be changed if we
   ever add surround positioning, or stereo reverb/chorus */
#define FLUID_RVOICE_MAX_BUFS (4)

/*
 * rvoice mixer-related parameters
 */
struct _fluid_rvoice_buffers_t
{
	unsigned int count; /* Number of records in "bufs" */
	struct {
		fluid_real_t amp;
		int mapping; /* Mapping to mixdown buffer index */
	} bufs[FLUID_RVOICE_MAX_BUFS];
};


/*
 * Hard real-time parameters needed to synthesize a voice
 */
struct _fluid_rvoice_t
{
	fluid_rvoice_envlfo_t envlfo;
	fluid_rvoice_dsp_t dsp; 
	fluid_iir_filter_t resonant_filter; /* IIR resonant dsp filter */
	fluid_rvoice_buffers_t buffers;
};


int fluid_rvoice_write(fluid_rvoice_t* voice, fluid_real_t *dsp_buf);

void fluid_rvoice_buffers_mix(fluid_rvoice_buffers_t* buffers, 
                              fluid_real_t* dsp_buf, int samplecount, 
                              fluid_real_t** dest_bufs, int dest_bufcount);
void fluid_rvoice_buffers_set_amp(fluid_rvoice_buffers_t* buffers, 
                                  unsigned int bufnum, fluid_real_t value);
void fluid_rvoice_buffers_set_mapping(fluid_rvoice_buffers_t* buffers,
                                      unsigned int bufnum, int mapping);

/* Dynamic update functions */
void fluid_rvoice_set_portamento(fluid_rvoice_t * voice, unsigned int countinc,
								 fluid_real_t pitchoffset);
void fluid_rvoice_multi_retrigger_attack(fluid_rvoice_t* voice);
void fluid_rvoice_noteoff(fluid_rvoice_t* voice, unsigned int min_ticks);
void fluid_rvoice_voiceoff(fluid_rvoice_t* voice);
void fluid_rvoice_reset(fluid_rvoice_t* voice);
void fluid_rvoice_set_output_rate(fluid_rvoice_t* voice, fluid_real_t output_rate);
void fluid_rvoice_set_interp_method(fluid_rvoice_t* voice, int interp_method);
void fluid_rvoice_set_root_pitch_hz(fluid_rvoice_t* voice, fluid_real_t root_pitch_hz);
void fluid_rvoice_set_pitch(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_synth_gain(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_attenuation(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_min_attenuation_cB(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_viblfo_to_pitch(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_modlfo_to_pitch(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_modlfo_to_vol(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_modlfo_to_fc(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_modenv_to_fc(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_modenv_to_pitch(fluid_rvoice_t* voice, fluid_real_t value);
void fluid_rvoice_set_start(fluid_rvoice_t* voice, int value);
void fluid_rvoice_set_end(fluid_rvoice_t* voice, int value);
void fluid_rvoice_set_loopstart(fluid_rvoice_t* voice, int value);
void fluid_rvoice_set_loopend(fluid_rvoice_t* voice, int value);
void fluid_rvoice_set_sample(fluid_rvoice_t* voice, fluid_sample_t* value);
void fluid_rvoice_set_samplemode(fluid_rvoice_t* voice, enum fluid_loop value);

/* defined in fluid_rvoice_dsp.c */

void fluid_rvoice_dsp_config (void);
int fluid_rvoice_dsp_interpolate_none (fluid_rvoice_dsp_t *voice);
int fluid_rvoice_dsp_interpolate_linear (fluid_rvoice_dsp_t *voice);
int fluid_rvoice_dsp_interpolate_4th_order (fluid_rvoice_dsp_t *voice);
int fluid_rvoice_dsp_interpolate_7th_order (fluid_rvoice_dsp_t *voice);


/*
 * Combines the most significant 16 bit part of a sample with a potentially present
 * least sig. 8 bit part in order to create a 24 bit sample.
 */
static FLUID_INLINE int32_t
fluid_rvoice_get_sample(const short int* dsp_msb, const char* dsp_lsb, unsigned int idx)
{
    /* cast sample to unsigned type, so we can safely shift and bitwise or
     * without relying on undefined behaviour (should never happen anyway ofc...) */
    uint32_t msb = (uint32_t)dsp_msb[idx];
    uint8_t lsb = 0U;
    
    /* most soundfonts have 16 bit samples, assume that it's unlikely we
     * experience 24 bit samples here */
    if(FLUID_UNLIKELY(dsp_lsb != NULL))
    {
        lsb = (uint8_t)dsp_lsb[idx];
    }
    
    return (int32_t)((msb << 8) | lsb);
}

#endif
