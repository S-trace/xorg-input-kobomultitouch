/*
 * Taken and modified from:
 *
 *  tslib/plugins/dejitter.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Problem: some touchscreens read the X/Y values from ADC with a
 * great level of noise in their lowest bits. This produces "jitter"
 * in touchscreen output, e.g. even if we hold the stylus still,
 * we get a great deal of X/Y coordinate pairs that are close enough
 * but not equal. Also if we try to draw a straight line in a painter
 * program, we'll get a line full of spikes.
 *
 * Solution: we apply a smoothing filter on the last several values
 * thus excluding spikes from output. If we detect a substantial change
 * in coordinates, we reset the backlog of pen positions, thus avoiding
 * smoothing coordinates that are not supposed to be smoothed. This
 * supposes all noise has been filtered by the lower-level filter,
 * e.g. by the "variance" module.
 */
#include "jitter_filter.h"

/**
 * This filter works as follows: we keep track of latest N samples,
 * and average them with certain weights. The oldest samples have the
 * least weight and the most recent samples have the most weight.
 * This helps remove the jitter and at the same time doesn't influence
 * responsivity because for each input sample we generate one output
 * sample; pen movement becomes just somehow more smooth.
 */

#define NR_SAMPHISTLEN 8
#define MAX_FINGERS 2

/* To keep things simple (avoiding division) we ensure that
 * SUM(weight) = power-of-two. Also we must know how to approximate
 * measurements when we have less than NR_SAMPHISTLEN samples.
 */
static const unsigned char weight [NR_SAMPHISTLEN - 1][NR_SAMPHISTLEN + 1] =
{
	/* The last element is pow2(SUM(0..3)) */
	{ 5, 3, 0, 0, 0, 0, 0, 0, 3 },	/* When we have 2 samples ... */
	{ 8, 5, 3, 0, 0, 0, 0, 0, 4 },	/* When we have 3 samples ... */
	{ 6, 4, 3, 3, 0, 0, 0, 0, 4 },	/* When we have 4 samples ... */
	{ 10, 8, 5, 5, 4, 0, 0, 0, 5 },	/* When we have 5 samples ... */
	{ 9, 7, 5, 4, 4, 3, 0, 0, 5 },	/* When we have 6 samples ... */
	{ 9, 6, 5, 4, 3, 3, 2, 0, 5 },	/* When we have 7 samples ... */
	{ 9, 5, 4, 3, 3, 3, 3, 2, 5 },	/* When we have 8 samples ... */
}; 

struct ts_hist {
	int x;
	int y;
	unsigned int p;
};

struct tslib_dejitter {
	int delta;
	int x;
	int y;
	int down;
	int nr;
	int head;
	struct ts_hist hist[NR_SAMPHISTLEN];
};

static struct tslib_dejitter *djts = NULL;

static int sqr (int x)
{
	return x * x;
}

static void average(struct tslib_dejitter *djt, struct FingerState *samp)
{
	const unsigned char *w;
	int sn = djt->head;
	int i, x = 0, y = 0;
	unsigned int p = 0;

        w = weight[djt->nr - 2];

	for (i = 0; i < djt->nr; i++) {
		x += djt->hist [sn].x * w [i];
		y += djt->hist [sn].y * w [i];
		p += djt->hist [sn].p * w [i];
		sn = (sn - 1) & (NR_SAMPHISTLEN - 1);
	}

	samp->position_x = x >> w [NR_SAMPHISTLEN];
	samp->position_y = y >> w [NR_SAMPHISTLEN];
	samp->pressure = p >> w [NR_SAMPHISTLEN];
//#ifdef DEBUG
	//fprintf(stderr,"DEJITTER----------------> %d %d %d (nr: %d)\n",
	//	samp->position_x, samp->position_y, samp->pressure, djt->nr);
//#endif
}

static void init()
{
	int i;

	djts = malloc(sizeof(struct tslib_dejitter) * MAX_FINGERS);
	memset(djts, 0, sizeof(struct tslib_dejitter) * MAX_FINGERS);

	for (i = 0; i < MAX_FINGERS; i++) {
		djts[i].delta = 15;
		djts[i].head = 0;
	}
}

void jitter_finger(struct FingerState *s)
{
	struct tslib_dejitter *djt;

	djt = &djts[s->tracking_id];

	/* If the pen moves too fast, reset the backlog. */
	if (djt->nr) {
		int prev = (djt->head - 1) & (NR_SAMPHISTLEN - 1);
		if (sqr (s->position_x - djt->hist [prev].x) +
			sqr (s->position_y - djt->hist [prev].y) > djt->delta) {
#ifdef DEBUG
			//fprintf (stderr, "DEJITTER: pen movement exceeds threshold\n");
#endif
			djt->nr = 0;
		}
	}

	djt->hist[djt->head].x = s->position_x;
	djt->hist[djt->head].y = s->position_y;
	djt->hist[djt->head].p = s->pressure;
	if (djt->nr < NR_SAMPHISTLEN)
		djt->nr++;

	/* We'll pass through the very first sample since
	 * we can't average it (no history yet).
	 */
	if (djt->nr > 1) {
		average (djt, s);
	}

	djt->head = (djt->head + 1) & (NR_SAMPHISTLEN - 1);
}

int jitter_filter(struct MTState *state)
{
	int i;
	static prev_fingers[MAX_FINGERS];

	if (djts == NULL) {
		init();
		for (i = 0; i < MAX_FINGERS; i++)
			prev_fingers[i] = 0;
	}

	for (i = 0; i < state->nfinger; i++) {
		struct FingerState *s = &state->finger[i];
		if (s->tracking_id < 0 || s->tracking_id >= MAX_FINGERS)
			continue;

		if (prev_fingers[s->tracking_id] == 0) {
			// first event - leave it as it is
			// and place in history
			djts[s->tracking_id].nr = 0;
		}

		jitter_finger(s);
	}

	for (i = 0; i < MAX_FINGERS; i++)
		prev_fingers[i] = 0;
	for (i = 0; i < state->nfinger; i++) {
		struct FingerState *s = &state->finger[i];
		prev_fingers[s->tracking_id] = 1;
	}

	return 1;
}

