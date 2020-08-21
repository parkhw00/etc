/***
  This file is part of PulseAudio.
  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.
  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.
  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, see <http://www.gnu.org/licenses/>.
 ***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#define FR	48000
#define CH	2
#define FRAMES	256

double A[8] =
{
	0.4,
	0.4,
	0.4,
	0.4,
	0.4,
	0.4,
	0.4,
	0.4,
};

int freq[8] =
{
	400,
	400,
	400,
	400,
	400,
	400,
	400,
	400,
};

int main(int argc, char*argv[]) {
	/* The Sample format to use */
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S32LE,
		.rate = FR,
		.channels = 2
	};

	pa_simple *s = NULL;
	int ret = 1;
	int error;
	int n = 0;

	/* Create a new playback stream */
	if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		goto finish;
	}

	for (;;) {
		int buf[FRAMES*CH];
		ssize_t r;
		int c, i;

#if 0
		pa_usec_t latency;
		if ((latency = pa_simple_get_latency(s, &error)) == (pa_usec_t) -1) {
			fprintf(stderr, __FILE__": pa_simple_get_latency() failed: %s\n", pa_strerror(error));
			goto finish;
		}

		fprintf(stderr, "%0.0f usec    \r", (float)latency);
#endif

		for (i=0; i<FRAMES; i++)
		{
			for (c=0; c<CH; c++)
			{
				buf[i*CH + c] =
					A[c] * INT_MAX * sin (M_PI * 2 * freq[c] * n / FR);
			}
			n ++;
		}

		/* ... and play it */
		if (pa_simple_write(s, buf, sizeof (buf), &error) < 0) {
			fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
			goto finish;
		}
	}

	/* Make sure that every single sample was played */
	if (pa_simple_drain(s, &error) < 0) {
		fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
		goto finish;
	}

	ret = 0;

finish:

	if (s)
		pa_simple_free(s);

	return ret;
}
