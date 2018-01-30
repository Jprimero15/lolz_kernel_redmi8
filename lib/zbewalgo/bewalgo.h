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

#ifndef ZBEWALGO_BEWALGO_H
#define ZBEWALGO_BEWALGO_H

#include "include.h"

#define BEWALGO_ACCELERATION_DEFAULT 1

#define BEWALGO_MEMORY_USAGE 14

#define BEWALGO_SKIPTRIGGER 6

#define BEWALGO_LENGTH_BITS 8
#define BEWALGO_LENGTH_MAX ((1 << BEWALGO_LENGTH_BITS) - 1)
#define BEWALGO_OFFSET_BITS 16
#define BEWALGO_OFFSET_MAX ((1 << BEWALGO_OFFSET_BITS) - 1)

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define BEWALGO_HASHLOG (BEWALGO_MEMORY_USAGE - 2)
#define BEWALGO_HASH_SIZE_uint32_t (1 << BEWALGO_HASHLOG)
struct bewalgo_compress_internal {
	uint32_t table[BEWALGO_HASH_SIZE_uint32_t];
};

#define bewalgo_copy_helper_src(dest, src, target) \
do { \
	while ((src) < (target) - 3) { \
		(dest)[0] = (src)[0]; \
		(dest)[1] = (src)[1]; \
		(dest)[2] = (src)[2]; \
		(dest)[3] = (src)[3]; \
		(dest) += 4; \
		(src) += 4; \
	} \
	if ((src) < (target) - 1) { \
		(dest)[0] = (src)[0]; \
		(dest)[1] = (src)[1]; \
		(dest) += 2; \
		(src) += 2; \
	} \
	if ((src) < (target)) \
		*(dest)++ = *(src)++; \
} while (0)

static __always_inline int decompress_bewalgo(
	const uint8_t * const source_,
	uint8_t * const dest_,
	uint8_t * const wrkmem __attribute__ ((unused)))
{
	const uint64_t * const source = (const uint64_t *) source_;
	uint64_t * const dest = (uint64_t *) dest_;
	const uint64_t *ip;
	uint64_t *match;
	uint64_t *op;
	const uint64_t *dest_end_ptr;
	const uint8_t *controll_block_ptr;
	const uint64_t *target;
	uint32_t to_read;
	uint32_t avail;
	const uint16_t dest_length = *(uint16_t *) source;
	const uint32_t dest_end = DIV_BY_8_ROUND_UP(dest_length);

	ip = (uint64_t *) (((uint8_t *) source) + 2);
	controll_block_ptr = (uint8_t *) ip;
	match = op = dest;
	dest_end_ptr = dest_end + dest;
	do {
		if (unlikely(dest_end_ptr <= op))
			goto _last_control_block;
		controll_block_ptr = (uint8_t *) ip;
		ip++;
		target = ip + controll_block_ptr[0];
		bewalgo_copy_helper_src(op, ip, target);
		match = op - ((uint16_t *) controll_block_ptr)[1];
		target = match + controll_block_ptr[1];
		bewalgo_copy_helper_src(op, match, target);
		target = ip + controll_block_ptr[4];
		bewalgo_copy_helper_src(op, ip, target);
		match = op - ((uint16_t *) controll_block_ptr)[3];
		target = match + controll_block_ptr[5];
		bewalgo_copy_helper_src(op, match, target);
	} while (1);
_last_control_block:
	to_read = controll_block_ptr[0];
	avail = dest_end_ptr - op;
	target = ip + MIN(to_read, avail);
	bewalgo_copy_helper_src(op, ip, target);
	match = op - ((uint16_t *) controll_block_ptr)[1];
	to_read = controll_block_ptr[1];
	avail = dest_end_ptr - op;
	target = match + MIN(to_read, avail);
	bewalgo_copy_helper_src(op, match, target);
	to_read = controll_block_ptr[4];
	avail = dest_end_ptr - op;
	target = ip + MIN(to_read, avail);
	bewalgo_copy_helper_src(op, ip, target);
	match = op - ((uint16_t *) controll_block_ptr)[3];
	to_read = controll_block_ptr[5];
	avail = dest_end_ptr - op;
	target = match + MIN(to_read, avail);
	bewalgo_copy_helper_src(op, match, target);
	return (op - dest) << 3;
}

static __always_inline uint32_t bewalgo_compress_hash(uint64_t sequence)
{
	return (uint32_t) (
		((sequence >> 24) * 11400714785074694791ULL)
		 >> (64 - BEWALGO_HASHLOG));
}

static __always_inline int compress_bewalgo_(
	struct bewalgo_compress_internal *wrkmem,
	const uint64_t * const source,
	uint64_t * const dest,
	const uint16_t source_length,
	uint8_t acceleration)
{
	const int acceleration_start =
		(acceleration < 4 ? 32 >> acceleration : 4);
	const uint64_t * const dest_end_ptr =
		dest + zbewalgo_max_output_size;
	const uint32_t source_end =
		(source_length >> 3) + ((source_length & 7) > 0);
	const uint64_t * const source_end_ptr = source + source_end;
	const uint64_t * const source_end_ptr_1 = source_end_ptr - 1;
	const uint64_t *match = source;
	const uint64_t *anchor = source;
	const uint64_t *ip = source;
	uint64_t *op = (uint64_t *) (((uint8_t *) dest) + 2);
	uint8_t *op_control = NULL;
	uint32_t op_control_available = 0;
	const uint64_t *target;
	int length;
	uint16_t offset;
	uint32_t h;
	int j;
	int tmp_literal_length;
	int match_nodd;
	int match_nzero_nodd;
	*(uint16_t *) dest = source_length;
	memset(wrkmem, 0, sizeof(struct bewalgo_compress_internal));
	h = bewalgo_compress_hash(*ip);
	wrkmem->table[h] = 0;
	do {
		j = acceleration_start;
		length = source_end_ptr_1 - ip;
		j = j < length ? j : length;
		for (length = 1; length <= j; length++) {
			ip++;
			h = bewalgo_compress_hash(*ip);
			match = source + wrkmem->table[h];
			wrkmem->table[h] = ip - source;
			if (*(uint64_t *) match == *(uint64_t *) ip)
				goto _find_match_left;
		}
		length = acceleration_start
			+ (acceleration << BEWALGO_SKIPTRIGGER);
		ip++;
		do {
			if (unlikely(ip >= source_end_ptr))
				goto _encode_last_literal;
			h = bewalgo_compress_hash(*ip);
			match = source + wrkmem->table[h];
			wrkmem->table[h] = ip - source;
			if (*(uint64_t *) match == *(uint64_t *) ip)
				goto _find_match_left;
			ip += (length++ >> BEWALGO_SKIPTRIGGER);
		} while (1);
_find_match_left:
		while ((match != source) && (match[-1] == ip[-1])) {
			ip--;
			match--;
			if (ip == anchor)
				goto _find_match_right;
		}
		length = ip - anchor;
		tmp_literal_length = length
			- (op_control_available ? BEWALGO_LENGTH_MAX : 0);
		if (unlikely(op
			 + ((tmp_literal_length / (BEWALGO_LENGTH_MAX * 2)))
			 + ((tmp_literal_length % (BEWALGO_LENGTH_MAX * 2)) > 0)
			 + length > dest_end_ptr)) {
			goto _error;
		}
		while (length > BEWALGO_LENGTH_MAX) {
			if (op_control_available == 0) {
				op_control = (uint8_t *) op;
				*op++ = 0;
			}
			op_control_available = !op_control_available;
			*op_control = BEWALGO_LENGTH_MAX;
			op_control += 4;
			target = anchor + BEWALGO_LENGTH_MAX;
			bewalgo_copy_helper_src(op, anchor, target);
			length -= BEWALGO_LENGTH_MAX;
		}
		if (likely(length > 0)) {
			if (op_control_available == 0) {
				op_control = (uint8_t *) op;
				*op++ = 0;
			}
			op_control_available = !op_control_available;
			*op_control = length;
			op_control += 4;
			bewalgo_copy_helper_src(op, anchor, ip);
		}
_find_match_right:
		do {
			ip++;
			match++;
		} while ((ip < source_end_ptr) && (*match == *ip));
		length = ip - anchor;
		offset = ip - match;
		anchor = ip;
		if (length > BEWALGO_LENGTH_MAX) {
			uint32_t control_match_value =
				(BEWALGO_LENGTH_MAX << 8) | (offset << 16);
			size_t match_length_div_255 = length / 255;
			size_t match_length_mod_255 = length % 255;
			uint32_t match_zero = match_length_mod_255 == 0;
			uint32_t match_nzero = !match_zero;
			int control_blocks_needed = match_length_div_255
				+ match_nzero
				- op_control_available;
			if (unlikely((op
					+ (control_blocks_needed >> 1)
					+ (control_blocks_needed & 1)
					) > dest_end_ptr))
				goto _error;
			op_control = op_control_available > 0
				? op_control
				: (uint8_t *) op;
			*((uint32_t *) op_control) = control_match_value;
			match_length_div_255 -= op_control_available > 0;
			match_nodd = !(match_length_div_255 & 1);
			match_nzero_nodd = match_nzero && match_nodd;
			if (match_length_div_255 > 0) {
				uint64_t control_match_value_long =
					control_match_value
					| (((uint64_t) control_match_value)
						<< 32);
				target = op
					+ (match_length_div_255 >> 1)
					+ (match_length_div_255 & 1);
				do {
					*op++ = control_match_value_long;
				} while (op < target);
			}
			op_control = ((uint8_t *) op) - 4;
			*(uint32_t *) (op_control
				+ (match_nzero_nodd << 3)) = 0;
			*(uint32_t *) (op_control
				+ (match_nzero_nodd << 2)) = 0;
			*(op_control + (match_nzero_nodd << 2) + 1) =
				(match_zero & match_nodd)
				 ? BEWALGO_LENGTH_MAX
				 : match_length_mod_255;
			*(uint16_t *) (op_control
				+ (match_nzero_nodd << 2) + 2) = offset;
			op_control += match_nzero_nodd << 3;
			op += match_nzero_nodd;
			op_control_available =
				(match_length_div_255 & 1) == match_zero;
		} else {
			if (unlikely((op_control_available == 0)
				&& (op >= dest_end_ptr)
				&& (op_control[-3] != 0)))
				goto _error;
			if (op_control[-3] != 0) {
				if (op_control_available == 0) {
					op_control = (uint8_t *) op;
					*op++ = 0;
				}
				op_control_available = !op_control_available;
				op_control += 4;
			}
			op_control[-3] = length;
			((uint16_t *) op_control)[-1] = offset;
		}
		if (unlikely(ip == source_end_ptr))
			goto _finish;
		h = bewalgo_compress_hash(*ip);
		match = source + wrkmem->table[h];
		wrkmem->table[h] = ip - source;
		if (*(uint64_t *) match == *(uint64_t *) ip)
			goto _find_match_right;
	} while (1);
_encode_last_literal:
	length = source_end_ptr - anchor;
	tmp_literal_length = length
		- (op_control_available ? BEWALGO_LENGTH_MAX : 0);
	if (op
		+ ((tmp_literal_length / (BEWALGO_LENGTH_MAX * 2)))
		+ ((tmp_literal_length % (BEWALGO_LENGTH_MAX * 2)) > 0)
		+ length > dest_end_ptr)
		goto _error;
	while (length > BEWALGO_LENGTH_MAX) {
		if (op_control_available == 0) {
			op_control = (uint8_t *) op;
			*op++ = 0;
		}
		op_control_available = !op_control_available;
		*op_control = BEWALGO_LENGTH_MAX;
		op_control += 4;
		target = anchor + BEWALGO_LENGTH_MAX;
		bewalgo_copy_helper_src(op, anchor, target);
		length -= BEWALGO_LENGTH_MAX;
	}
	if (length > 0) {
		if (op_control_available == 0) {
			op_control = (uint8_t *) op;
			*op++ = 0;
		}
		op_control_available = !op_control_available;
		*op_control = length;
		op_control += 4;
		bewalgo_copy_helper_src(op, anchor, source_end_ptr);
	}
_finish:
	return ((uint8_t *) op) - ((uint8_t *) dest);
_error:
	return -1;
}

static __always_inline int compress_bewalgo(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem,
	const uint16_t source_length)
{
	return compress_bewalgo_(
		(struct bewalgo_compress_internal *) wrkmem,
		(const uint64_t *) source,
		(uint64_t *) dest,
		source_length, 1);
}

static int init_bewalgo(void)
{
	return 0;
}

static void exit_bewalgo(void)
{
}

static struct zbewalgo_alg alg_bewalgo = {
	.name = "bewalgo",
	.flags = ZBEWALGO_ALG_FLAG_COMPRESS,
	.wrkmem_size = sizeof(struct bewalgo_compress_internal),
	.init = init_bewalgo,
	.exit = exit_bewalgo,
	.compress = compress_bewalgo,
	.decompress = decompress_bewalgo
};

#endif
