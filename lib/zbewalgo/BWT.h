/*
 * Copyright (c) 2018 Benjamin Warnke <4bwarnke@informatik.uni-hamburg.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef ZBEWALGO_BWT_H
#define ZBEWALGO_BWT_H

#include "include.h"

/*
 * implementation of the Burrows-Wheeler transformation. Optimized for speed
 * and small input sizes
 */
static __always_inline int compress_bwt(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem,
	const uint16_t source_length)
{
	uint16_t * const C = (uint16_t *) wrkmem;
	uint16_t i;
	uint16_t alphabet;
	uint8_t * const op = dest + 3;
	*dest = source[source_length - 1];
	*((uint16_t *) (dest + 1)) = source_length;
	memset(C, 0, 512);
	for (i = 0; i < source_length; i++)
		C[source[i]]++;
	alphabet = (C[0] > 0);
	for (i = 1; i < 256; i++) {
		alphabet += (C[i] > 0);
		C[i] += C[i - 1];
	}
	if (alphabet > zbewalgo_bwt_max_alphabet)
		return -1;
	i = source_length - 1;
	C[source[i]]--;
	op[C[source[i]]] = source[i - 1];
	i--;
	while (i > 0) {
		C[source[i]]--;
		op[C[source[i]]] = source[i - 1];
		i--;
	}
	C[source[0]]--;
	op[C[source[0]]] = source[source_length - 1];
	return source_length + 3;
}

/*
 * reverses the transformation
 */
static __always_inline int decompress_bwt(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem)
{
	const uint16_t dest_length = *((uint16_t *) (source + 1));
	uint16_t * const C = (uint16_t *) wrkmem;
	uint8_t * const L = wrkmem + 512;
	uint8_t key = *source;
	uint8_t *dest_end = dest + dest_length;
	const uint8_t *ip = source + 3;
	int i, j;

	memset(C, 0, 512);
	for (i = 0; i < dest_length; i++)
		C[ip[i]]++;
	for (i = 1; i < 256; i++)
		C[i] += C[i - 1];
	j = 0;
	for (i = 0; i < 256; i++)
		while (j < C[i])
			L[j++] = i;
	do {
		C[key]--;
		*--dest_end = L[C[key]];
		key = ip[C[key]];
	} while (dest < dest_end);
	return dest_length;
}

static int init_bwt(void)
{
	return 0;
}

static void exit_bwt(void)
{
}

static struct zbewalgo_alg alg_bwt = {
	.name = "bwt", //
	.flags = ZBEWALGO_ALG_FLAG_TRANSFORM, //
	.wrkmem_size = PAGE_SIZE << 1, //
	.init = init_bwt, //
	.exit = exit_bwt, //
	.compress = compress_bwt, //
	.decompress = decompress_bwt //
};

#endif
