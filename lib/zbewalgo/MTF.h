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

#ifndef ZBEWALGO_MTF_H
#define ZBEWALGO_MTF_H

#include "include.h"

/*
 * Move To Front encoding
 */
static __always_inline int compress_mtf(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem,
	const uint16_t source_length)
{
	const uint8_t *source_end = source + source_length;
	const uint8_t *ip = source;
	uint8_t *op = dest + 2;
	uint16_t i;
	uint8_t tmp;
	*(uint16_t *) dest = source_length;
	for (i = 0; i < 256; i++)
		wrkmem[i] = i;
	do {
		i = 0;
		while (wrkmem[i] != *ip)
			i++;
		ip++;
		*op++ = i;
		tmp = wrkmem[i];
		while (i > 0) {
			wrkmem[i] = wrkmem[i - 1];
			i--;
		}
		wrkmem[0] = tmp;
	} while (ip < source_end);
	return source_length + 2;
}

static __always_inline int decompress_mtf(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem)
{
	const uint16_t dest_length = *(uint16_t *) source;
	uint8_t *dest_end = dest + dest_length;
	uint16_t i;
	uint8_t tmp;
	const uint8_t *ip = source + 2;
	uint8_t *op = dest;

	for (i = 0; i < 256; i++)
		wrkmem[i] = i;
	do {
		i = *ip++;
		*op++ = wrkmem[i];
		tmp = wrkmem[i];
		while (i > 0) {
			wrkmem[i] = wrkmem[i - 1];
			i--;
		}
		wrkmem[0] = tmp;
	} while (op < dest_end);
	return dest_length;
}

static int init_mtf(void)
{
	return 0;
}

static void exit_mtf(void)
{
}

static struct zbewalgo_alg alg_mtf = {
	.name = "mtf", //
	.flags = ZBEWALGO_ALG_FLAG_TRANSFORM, //
	.wrkmem_size = 1 << 8, //
	.init = init_mtf, //
	.exit = exit_mtf, //
	.compress = compress_mtf, //
	.decompress = decompress_mtf //
};
#endif
