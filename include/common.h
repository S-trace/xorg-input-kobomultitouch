/***************************************************************************
 *
 * Multitouch X driver
 * Copyright (C) 2008 Henrik Rydberg <rydberg@euromail.se>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 **************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#include "xorg-server.h"
#include <xf86.h>
#include <xf86_OSproc.h>
#include <xf86Xinput.h>
#include <errno.h>
#include <mtdev-mapping.h>

#define DIM_FINGER 32
#define DIM2_FINGER (DIM_FINGER * DIM_FINGER)

/* year-proof millisecond event time */
typedef __u64 mstime_t;

/* all bit masks have this type */
typedef unsigned int bitmask_t;

#define BITMASK(x) (1U << (x))
#define BITONES(x) (BITMASK(x) - 1U)
#define GETBIT(m, x) (((m) >> (x)) & 1U)
#define SETBIT(m, x) (m |= BITMASK(x))
#define CLEARBIT(m, x) (m &= ~BITMASK(x))
#define MODBIT(m, x, b) ((b) ? SETBIT(m, x) : CLEARBIT(m, x))

static inline int maxval(int x, int y) { return x > y ? x : y; }
static inline int minval(int x, int y) { return x < y ? x : y; }

static inline int clamp15(int x)
{
	return x < -32767 ? -32767 : x > 32767 ? 32767 : x;
}

/* absolute scale is assumed to fit in 15 bits */
static inline int dist2(int dx, int dy)
{
	dx = clamp15(dx);
	dy = clamp15(dy);
	return dx * dx + dy * dy;
}

/* Count number of bits (Sean Eron Andersson's Bit Hacks) */
static inline int bitcount(unsigned v)
{
	v -= ((v>>1) & 0x55555555);
	v = (v&0x33333333) + ((v>>2) & 0x33333333);
	return (((v + (v>>4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

/* Return index of first bit [0-31], -1 on zero */
#define firstbit(v) (__builtin_ffs(v) - 1)

/* boost-style foreach bit */
#define foreach_bit(i, m)						\
	for (i = firstbit(m); i >= 0; i = firstbit((m) & (~0U << i + 1)))

/* robust system ioctl calls */
#define SYSCALL(call) while (((call) == -1) && (errno == EINTR))

#endif
