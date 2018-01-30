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

#ifndef ZBEWALGO_JBE_H
#define ZBEWALGO_JBE_H

#include "include.h"

/*
 * Implementation of the J-Bit encoding as published by
 * 'I Made Agus Dwi Suarjaya' 2012
 */
static __always_inline int compress_jbe(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem __attribute__ ((unused)),
	const uint16_t source_length)
{
	uint64_t source_tmp;
	uint8_t tmp;
	const uint64_t *source_end = (uint64_t *) (source + source_length);
	const uint64_t *ip = (uint64_t *) source;
	uint8_t *dataI = dest + 2;
	uint8_t *dataII = dataI + DIV_BY_8_ROUND_UP(source_length);
	uint8_t * const source_tmp_ptr = (uint8_t *) (&source_tmp);
	*(uint16_t *) dest = source_length;
	do {
		source_tmp = *ip++;
		tmp = source_tmp_ptr[0] > 0;
		*dataII = source_tmp_ptr[0];
		*dataI = tmp << 7;
		dataII += tmp;
		tmp = source_tmp_ptr[1] > 0;
		*dataII = source_tmp_ptr[1];
		*dataI |= tmp << 6;
		dataII += tmp;
		tmp = source_tmp_ptr[2] > 0;
		*dataII = source_tmp_ptr[2];
		*dataI |= tmp << 5;
		dataII += tmp;
		tmp = source_tmp_ptr[3] > 0;
		*dataII = source_tmp_ptr[3];
		*dataI |= tmp << 4;
		dataII += tmp;
		tmp = source_tmp_ptr[4] > 0;
		*dataII = source_tmp_ptr[4];
		*dataI |= tmp << 3;
		dataII += tmp;
		tmp = source_tmp_ptr[5] > 0;
		*dataII = source_tmp_ptr[5];
		*dataI |= tmp << 2;
		dataII += tmp;
		tmp = source_tmp_ptr[6] > 0;
		*dataII = source_tmp_ptr[6];
		*dataI |= tmp << 1;
		dataII += tmp;
		tmp = source_tmp_ptr[7] > 0;
		*dataII = source_tmp_ptr[7];
		*dataI |= tmp;
		dataII += tmp;
		dataI++;
	} while (ip < source_end);
	return dataII - dest;
}

static __always_inline int decompress_jbe(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem __attribute__ ((unused)))
{
	uint64_t dest_tmp;
	const uint16_t dest_length = *(uint16_t *) source;
	const uint8_t *dataI = source + 2;
	const uint8_t *dataII = dataI + DIV_BY_8_ROUND_UP(dest_length);
	const uint64_t *dest_end = (uint64_t *) (dest + dest_length);
	uint8_t * const dest_tmp_ptr = (uint8_t *) (&dest_tmp);
	uint64_t *op = (uint64_t *) dest;

	do {
		dest_tmp_ptr[0] = ((*dataI & 0x80) ? *dataII : 0);
		dataII += (*dataI & 0x80) > 0;
		dest_tmp_ptr[1] = ((*dataI & 0x40) ? *dataII : 0);
		dataII += (*dataI & 0x40) > 0;
		dest_tmp_ptr[2] = ((*dataI & 0x20) ? *dataII : 0);
		dataII += (*dataI & 0x20) > 0;
		dest_tmp_ptr[3] = ((*dataI & 0x10) ? *dataII : 0);
		dataII += (*dataI & 0x10) > 0;
		dest_tmp_ptr[4] = ((*dataI & 0x08) ? *dataII : 0);
		dataII += (*dataI & 0x08) > 0;
		dest_tmp_ptr[5] = ((*dataI & 0x04) ? *dataII : 0);
		dataII += (*dataI & 0x04) > 0;
		dest_tmp_ptr[6] = ((*dataI & 0x02) ? *dataII : 0);
		dataII += (*dataI & 0x02) > 0;
		dest_tmp_ptr[7] = ((*dataI & 0x01) ? *dataII : 0);
		dataII += (*dataI & 0x01) > 0;
		dataI++;
		*op++ = dest_tmp;
	} while (op < dest_end);
	return dest_length;
}

static int init_jbe(void)
{
	return 0;
}

static void exit_jbe(void)
{
}

static struct zbewalgo_alg alg_jbe = {
	.name = "jbe",
	.flags = ZBEWALGO_ALG_FLAG_COMPRESS,
	.wrkmem_size = 0,
	.init = init_jbe,
	.exit = exit_jbe,
	.compress = compress_jbe,
	.decompress = decompress_jbe
};
#endif
