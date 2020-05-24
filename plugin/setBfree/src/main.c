/* setBfree - DSP tonewheel organ
 *
 * Copyright (C) 2003-2004 Fredrik Kilander <fk@dsv.su.se>
 * Copyright (C) 2008-2018 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2012 Will Panther <pantherb@setbfree.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#if defined _WIN32 && !defined WIN32
#define WIN32 // jack2 sysdeps
#endif

#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>

#ifndef _WIN32
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#else
#include <windows.h>
#endif
#include "global_inst.h"
#include "main.h"
#include "pgmParser.h"
#include "program.h"
#include "state.h"
#include "vibrato.h"

#ifdef HAVE_ASEQ
#include "midi_aseq.h"
#endif

#ifdef HAVE_ZITACONVOLVE
#include "convolution.h"
#endif

#ifdef HAVE_SRC
#include <math.h>
#include <samplerate.h>
#endif

#define NOF_CFG_OVERS (32) /* Max command-line config commands */

#ifndef SHAREDIR
#define SHAREDIR "."
#endif

static const char* templateConfigFile    = SHAREDIR "/cfg/default.cfg";
static const char* templateProgrammeFile = SHAREDIR "/pgm/default.pgm";

static char* defaultConfigFile    = NULL;
static char* defaultProgrammeFile = NULL;

#define AUDIO_CHANNELS (2)
#define CHN_LEFT (0)
#define CHN_RIGHT (1)

#ifdef HAVE_ASEQ
int aseq_stop = 0;
#endif
static int use_jack_midi = 1;

static char* midi_port  = NULL;
static char* jack_ports = NULL;
static char* jack_port[AUDIO_CHANNELS];

static const char* portnames[AUDIO_CHANNELS] = {
	"out_left", "out_right"
};

static const char* jack_client_name = "setBfree";

static float bufA[BUFFER_SIZE_SAMPLES];
static float bufB[BUFFER_SIZE_SAMPLES];
static float bufC[BUFFER_SIZE_SAMPLES];

double SampleRateD = 48000.0;
int    SampleRateI = 48000;

static int synth_ready = 0;

b_instance inst;

#define JACK_MIDI_QUEUE_SIZE (256)

static my_midi_event_t event_queue[JACK_MIDI_QUEUE_SIZE];
static int             queued_events_start = 0;
static int             queued_events_end   = 0;

static void
mctl_cb (int fnid, const char* fn, unsigned char val, midiCCmap* mm, void* arg)
{
	while (mm) {
		unsigned char v = val & 0x7f;
		if (getCtrlFlag (inst.midicfg, mm->channel, mm->param) & MFLAG_INV) {
			v = 127 - v;
		}

		// printf("MIDI FEEDBACK chn:%d param:%d val:%d\n", mm->channel, mm->param, v);

		if (((queued_events_start + 1) % JACK_MIDI_QUEUE_SIZE) == queued_events_end) {
			return;
		}
		event_queue[queued_events_start].size      = 3;
		event_queue[queued_events_start].buffer[0] = 0xb0 | (mm->channel & 0x0f);
		event_queue[queued_events_start].buffer[1] = (mm->param & 0x7f);
		event_queue[queued_events_start].buffer[2] = v;
		queued_events_start                        = (queued_events_start + 1) % JACK_MIDI_QUEUE_SIZE;

		mm = mm->next;
	}
}

#ifdef HAVE_ZITACONVOLVE
static void
mixdown (float** inout, const float** in2, int nchannels, int nsamples)
{
	int c, i;
	for (c = 0; c < nchannels; c++)
		for (i = 0; i < nsamples; i++)
			inout[c][i] += in2[c][i];
}
#endif

#ifndef MIN
#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#endif

static int
jack_audio_callback (jack_nframes_t nframes, void* arg)
{
	jack_default_audio_sample_t** out = j_output_bufferptrs;
	int                           i;

	void*          jack_midi_inbuf  = NULL;
	void*          jack_midi_outbuf = NULL;
	int            midi_events      = 0;
	jack_nframes_t midi_tme_proc    = 0;

	if (jack_midi_output) {
		jack_midi_outbuf = jack_port_get_buffer (jack_midi_output, nframes);
		jack_midi_clear_buffer (jack_midi_outbuf);
	}

	if (!synth_ready) {
		for (i = 0; i < AUDIO_CHANNELS; i++) {
			out[i] = (jack_default_audio_sample_t*)jack_port_get_buffer (j_output_port[i], nframes);
			memset (out[i], 0, nframes * sizeof (jack_default_audio_sample_t));
		}
		return (0);
	}

	if (use_jack_midi) {
		jack_midi_inbuf = jack_port_get_buffer (jack_midi_port, nframes);
		midi_events     = jack_midi_get_event_count (jack_midi_inbuf);
	}

	for (i = 0; i < AUDIO_CHANNELS; i++) {
		out[i] = (jack_default_audio_sample_t*)jack_port_get_buffer (j_output_port[i], nframes);
	}

	static int boffset = BUFFER_SIZE_SAMPLES;

	jack_nframes_t written = 0;

	while (written < nframes) {
		int nremain = nframes - written;

		if (boffset >= BUFFER_SIZE_SAMPLES) {
			if (use_jack_midi) {
				for (i = 0; i < midi_events; i++) {
					jack_midi_event_t ev;
					jack_midi_event_get (&ev, jack_midi_inbuf, i);
					if (ev.time >= written && ev.time < (written + BUFFER_SIZE_SAMPLES))
						parse_raw_midi_data (arg, ev.buffer, ev.size);
				}
				midi_tme_proc = written + BUFFER_SIZE_SAMPLES;
			}
			boffset = 0;
			oscGenerateFragment (inst.synth, bufA, BUFFER_SIZE_SAMPLES);
			preamp (inst.preamp, bufA, bufB, BUFFER_SIZE_SAMPLES);
			reverb (inst.reverb, bufB, bufC, BUFFER_SIZE_SAMPLES);

#ifdef HAVE_ZITACONVOLVE
			whirlProc2 (inst.whirl,
			            bufC,
			            NULL, NULL,
			            bufH[0], bufH[1],
			            bufD[0], bufD[1],
			            BUFFER_SIZE_SAMPLES);

			const float* horn[2] = { bufH[0], bufH[1] };
			const float* drum[2] = { bufD[0], bufD[1] };
			float*       out[2]  = { bufJ[0], bufJ[1] };

			convolve (horn, out, 2, BUFFER_SIZE_SAMPLES);
			mixdown (out, drum, AUDIO_CHANNELS, BUFFER_SIZE_SAMPLES);
#else
			whirlProc (inst.whirl, bufC, bufJ[0], bufJ[1], BUFFER_SIZE_SAMPLES);
#endif
		}

		int nread = MIN (nremain, (BUFFER_SIZE_SAMPLES - boffset));

		for (i = 0; i < AUDIO_CHANNELS; i++) {
			memcpy (&out[i][written], &bufJ[i][boffset], nread * sizeof (float));
		}
		written += nread;
		boffset += nread;
	}

	if (use_jack_midi) {
		/* process remaining MIDI events
     * IF nframes < BUFFER_SIZE_SAMPLES OR nframes != N * BUFFER_SIZE_SAMPLES
     */
		for (i = 0; i < midi_events; i++) {
			jack_midi_event_t ev;
			jack_midi_event_get (&ev, jack_midi_inbuf, i);
			if (ev.time >= midi_tme_proc)
				parse_raw_midi_data (arg, ev.buffer, ev.size);
		}
	}

	if (jack_midi_output && nframes > 0) {
		while (queued_events_end != queued_events_start) {
			jack_midi_event_write (jack_midi_outbuf,
			                       nframes - 1,
			                       event_queue[queued_events_end].buffer,
			                       event_queue[queued_events_end].size);
			queued_events_end = (queued_events_end + 1) % JACK_MIDI_QUEUE_SIZE;
		}
	}

	return (0);
}



/*
 * Instantiate /classes/.
 */
static void
allocAll ()
{
	if (!(inst.state = allocRunningConfig ())) {
		fprintf (stderr, "FATAL: memory allocation failed for state memory.\n");
		exit (1);
	}
	if (!(inst.progs = allocProgs ())) {
		fprintf (stderr, "FATAL: memory allocation failed for program config.\n");
		exit (1);
	}
	if (!(inst.reverb = allocReverb ())) {
		fprintf (stderr, "FATAL: memory allocation failed for reverb.\n");
		exit (1);
	}
	if (!(inst.whirl = allocWhirl ())) {
		fprintf (stderr, "FATAL: memory allocation failed for whirl.\n");
		exit (1);
	}
	if (!(inst.synth = allocTonegen ())) {
		fprintf (stderr, "FATAL: memory allocation failed for tonegen.\n");
		exit (1);
	}
	if (!(inst.midicfg = allocMidiCfg (inst.state))) {
		fprintf (stderr, "FATAL: memory allocation failed for midi config.\n");
		exit (1);
	}
	if (!(inst.preamp = allocPreamp ())) {
		fprintf (stderr, "FATAL: memory allocation failed for midi config.\n");
		exit (1);
	}
}

/*
 * delete /class/ instances
 */
static void
freeAll ()
{
	freeReverb (inst.reverb);
	freeWhirl (inst.whirl);

	freeToneGenerator (inst.synth);
	freeMidiCfg (inst.midicfg);
	freePreamp (inst.preamp);
	freeProgs (inst.progs);
	freeRunningConfig (inst.state);
#ifdef HAVE_ZITACONVOLVE
	freeConvolution ();
#endif
}

/*
 * Ask each module to initialize itself.
 */
static void
initAll ()
{
	fprintf (stderr, "init.. ");
	fprintf (stderr, "Audio : ");
	fflush (stderr);

	/* Open the JACK connection and get samplerate. */
	if (open_jack ()) {
		perror ("could not connect to JACK.");
		exit (1);
	}

	fprintf (stderr, "Oscillators : ");
	fflush (stderr);
	initToneGenerator (inst.synth, inst.midicfg);

	fprintf (stderr, "Scanner : ");
	fflush (stderr);
	initVibrato (inst.synth, inst.midicfg);

	fprintf (stderr, "Overdrive : ");
	fflush (stderr);
	initPreamp (inst.preamp, inst.midicfg);

	fprintf (stderr, "Reverb : ");
	fflush (stderr);
	initReverb (inst.reverb, inst.midicfg, SampleRateD);

	fprintf (stderr, "Whirl : ");
	fflush (stderr);
	initWhirl (inst.whirl, inst.midicfg, SampleRateD);

#ifdef HAVE_ZITACONVOLVE
	fprintf (stderr, "Convolve : ");
	fflush (stderr);
	int                s_policy = 0;
	struct sched_param s_param;

	memset (&s_param, 0, sizeof (struct sched_param));
	if (jack_client_thread_id (j_client)) {
		pthread_getschedparam (jack_client_thread_id (j_client), &s_policy, &s_param);
	} else {
		fprintf (stderr, "zita-convolver: not using RT scheduling\n");
	}
	initConvolution (NULL, inst.midicfg, AUDIO_CHANNELS, BUFFER_SIZE_SAMPLES, s_param.sched_priority, s_policy);
#endif

	fprintf (stderr, "RC : ");
	fflush (stderr);
	initRunningConfig (inst.state, inst.midicfg);

	fprintf (stderr, "..done.\n");
}

const ConfigDoc*
mainDoc ()
{
	return doc;
}

static void
parse_preset (unsigned int* p, const char* c)
{
	unsigned int i;
	for (i = 0; i < strlen (c); ++i) {
		if (c[i] < '0' || c[i] > '9') {
			p[i] = 0;
		} else {
			p[i] = c[i] - '0';
		}
	}
	for (; i < 9; ++i) {
		p[i] = 0;
	}
}
