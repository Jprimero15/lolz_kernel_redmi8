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

#ifndef ZBEWALGO_RLE_H
#define ZBEWALGO_RLE_H

#include "include.h"

#define RLE_REPEAT 0x80
#define RLE_SIMPLE 0x00
#define RLE_MAX_LENGTH ((1 << 7) - 1)


/*
 * Run Length Encoder
 */
static __always_inline int compress_rle(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem __attribute__ ((unused)),
	const uint16_t source_length)
{
	const uint8_t *anchor = source;
	const uint8_t *source_end = source + source_length;
	unsigned int count;
	uint8_t lastval;
	uint8_t *op = dest + 2;
	const uint8_t *ip = source;
	*((uint16_t *) dest) = source_length;
	while (1) {
		// RLE_SIMPLE
		do {
			lastval = *ip++;
		} while ((ip < source_end) && (lastval != *ip));
		count = ip - anchor - (ip < source_end);
		if (count > 0) {
			while (count > RLE_MAX_LENGTH) {
				*op++ = RLE_SIMPLE | RLE_MAX_LENGTH;
				memcpy(op, anchor, RLE_MAX_LENGTH + 1);
				anchor += RLE_MAX_LENGTH + 1;
				op += RLE_MAX_LENGTH + 1;
				count -= RLE_MAX_LENGTH + 1;
			}
			if (count > 0) {
				*op++ = RLE_SIMPLE | (count - 1);
				memcpy(op, anchor, count);
				anchor += count;
				op += count;
			}
		}
		if (ip == source_end)
			return op - dest;
		// RLE_REPEAT
		do {
			lastval = *ip++;
		} while ((ip < source_end) && (lastval == *ip));
		count = ip - anchor;
		if (count > 0) {
			anchor += count;
			while (count > RLE_MAX_LENGTH) {
				*op++ = RLE_REPEAT | RLE_MAX_LENGTH;
				*op++ = lastval;
				count -= RLE_MAX_LENGTH + 1;
			}
			if (count > 0) {
				*op++ = RLE_REPEAT | (count - 1);
				*op++ = lastval;
			}
		}
		if (ip == source_end)
			return op - dest;
	}
}

static __always_inline int decompress_rle(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem __attribute__ ((unused)))
{
	const uint16_t source_length = *((uint16_t *) source);
	const uint8_t *dest_end = dest + source_length;
	unsigned int length;
	const uint8_t *ip = source + 2;
	uint8_t *op = dest;

	do {
		if ((*ip & RLE_REPEAT) == RLE_REPEAT) {
			length = *ip++ - RLE_REPEAT + 1;
			memset(op, *ip++, length);
			op += length;
		} else {
			length = *ip++ - RLE_SIMPLE + 1;
			memcpy(op, ip, length);
			ip += length;
			op += length;
		}
	} while (op < dest_end);
	return op - dest;
}

static int init_rle(void)
{
	return 0;
}

static void exit_rle(void)
{
}

static struct zbewalgo_alg alg_rle = {
	.name = "rle",
	.flags = ZBEWALGO_ALG_FLAG_COMPRESS,
	.wrkmem_size = 0,
	.init = init_rle,
	.exit = exit_rle,
	.compress = compress_rle,
	.decompress = decompress_rle
};
#endif
