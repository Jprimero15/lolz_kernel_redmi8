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

#ifndef ZBEWALGO_BITSHUFFLE_H
#define ZBEWALGO_BITSHUFFLE_H

#include "include.h"

/*
 * performs a static transformation on the data. Moves every eighth byte into
 * a consecutive range
 */
static __always_inline int compress_bitshuffle(
	const uint8_t *source, uint8_t *dest,
	uint8_t * const wrkmem __attribute__ ((unused)),
	uint16_t source_length)
{
	uint16_t i;
	*((uint16_t *) dest) = source_length;
	dest += 2;
	source_length = (source_length + 7) & (~7);
	for (i = 0; i < source_length; i += 8)
		*dest++ = source[i];
	for (i = 1; i < source_length; i += 8)
		*dest++ = source[i];
	for (i = 2; i < source_length; i += 8)
		*dest++ = source[i];
	for (i = 3; i < source_length; i += 8)
		*dest++ = source[i];
	for (i = 4; i < source_length; i += 8)
		*dest++ = source[i];
	for (i = 5; i < source_length; i += 8)
		*dest++ = source[i];
	for (i = 6; i < source_length; i += 8)
		*dest++ = source[i];
	for (i = 7; i < source_length; i += 8)
		*dest++ = source[i];
	return source_length + 2;
}

/*
 * reverses the transformation step
 */
static __always_inline int decompress_bitshuffle(
	const uint8_t *source,
	uint8_t *dest,
	uint8_t * const wrkmem __attribute__ ((unused)))
{
	uint16_t i;
	uint16_t dest_length = *((uint16_t *) source);
	uint16_t dest_length2 = (dest_length + 7) & (~7);

	source += 2;
	for (i = 0; i < dest_length2; i += 8)
		dest[i] = *source++;
	for (i = 1; i < dest_length2; i += 8)
		dest[i] = *source++;
	for (i = 2; i < dest_length2; i += 8)
		dest[i] = *source++;
	for (i = 3; i < dest_length2; i += 8)
		dest[i] = *source++;
	for (i = 4; i < dest_length2; i += 8)
		dest[i] = *source++;
	for (i = 5; i < dest_length2; i += 8)
		dest[i] = *source++;
	for (i = 6; i < dest_length2; i += 8)
		dest[i] = *source++;
	for (i = 7; i < dest_length2; i += 8)
		dest[i] = *source++;
	return dest_length;
}
static int init_bitshuffle(void)
{
	return 0;
}

static void exit_bitshuffle(void)
{
}

static struct zbewalgo_alg alg_bitshuffle = {
	.name = "bitshuffle",
	.flags = ZBEWALGO_ALG_FLAG_TRANSFORM,
	.wrkmem_size = 0,
	.init = init_bitshuffle,
	.exit = exit_bitshuffle,
	.compress = compress_bitshuffle,
	.decompress = decompress_bitshuffle
};
#endif
