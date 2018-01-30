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

#include <linux/init.h>
#include <linux/module.h>

#include "include.h"

#include "BWT.h"
#include "JBE.h"
#include "JBE2.h"
#include "MTF.h"
#include "RLE.h"
#include "bewalgo.h"
#include "bewalgo2.h"
#include "bitshuffle.h"
#include "huffman.h"
#include "settings.h"

static atomic64_t zbewalgo_stat_combination[256];
static atomic64_t zbewalgo_stat_count[256];

/*
 * returns the required size of wrkmem for compression and decompression
 */
__always_inline int zbewalgo_get_wrkmem_size(void)
{
	return zbewalgo_wrkmem_size;
}
EXPORT_SYMBOL(zbewalgo_get_wrkmem_size);

/*
 * this function adds a combination to the set of enabled combinations or
 * if 'flag' is set, replaces all combinations with the new supplied one.
 * this function is called from the sysfs context, and therefore accepts
 * a string as input.
 */
static __always_inline int add_set_combination(
	const char * const string,
	const int string_length,
	int flag)
{
	/* points behind the string for fast looping over the entire string */
	const char * const string_end = string + string_length;
	/* the string up to 'anchor' is parsed */
	const char *anchor = string;
	const char *pos = string;
	struct zbewalgo_combination combination;
	uint8_t i;

	memset(&combination, 0, sizeof(struct zbewalgo_combination));
	/* loop over entire string */
	while ((pos < string_end) && (*pos != 0)) {
		while ((pos < string_end) && (*pos != 0) && (*pos != '-'))
			pos++;
		if (pos - anchor <= 0) {
			/* skipp leading or consecutive '-' chars */
			pos++;
			anchor = pos;
			continue;
		}
		/* find the algorithm by name in the list of all algorithms */
		for (i = 0; i < zbewalgo_base_algorithms_count; i++) {
			if (((unsigned int) (pos - anchor)
				== strlen(zbewalgo_base_algorithms[i].name))
				&& (memcmp(anchor,
					zbewalgo_base_algorithms[i].name,
					pos - anchor)
					 == 0)) {
				/* found algorithm */
				combination.ids[combination.count++] =
					zbewalgo_base_algorithms[i].id;
				break;
			}
		}
		/*
		 * abort parsing if maximum of algorithms reached or
		 * if string is parsed completely
		 */
		if ((combination.count >= ZBEWALGO_COMBINATION_MAX_IDS)
			|| (*pos != '-'))
			goto _finalize;
		if (i == zbewalgo_base_algorithms_count)
			/* misstyped arguments */
			return -1;
		pos++;
		anchor = pos;
	}
_finalize:
	if (combination.count) {
		/* if combination has at least 1 algorithm */
		if (flag == 1)
			zbewalgo_combinations_count = 0;
		/* don't store the same combination twice */
		for (i = 0; i < zbewalgo_combinations_count; i++)
			if (memcmp(
				&zbewalgo_combinations[i],
				&combination,
				sizeof(struct zbewalgo_combination)) == 0) {
				return 0;
			}
		/* store the new combination */
		memcpy(
			&zbewalgo_combinations[zbewalgo_combinations_count],
			&combination,
			sizeof(struct zbewalgo_combination));
		zbewalgo_combinations_count++;
		return 0;
	}
	return -1; /* empty algorithm is not allowed */
}

int zbewalgo_add_combination(
	const char * const string,
	const int string_length)
{
	return add_set_combination(string, string_length, 0);
}
EXPORT_SYMBOL(zbewalgo_add_combination);

int zbewalgo_set_combination(
	const char * const string,
	const int string_length)
{
	return add_set_combination(string, string_length, 1);
}
EXPORT_SYMBOL(zbewalgo_set_combination);

static int __init zbewalgo_mod_init(void)
{
	uint8_t i;
	int j;
	int res = 0;

	zbewalgo_early_abort_size = 400;
	/*
	 * this algorithm is intended to be used for zram with zsmalloc.
	 * Therefore zbewalgo_max_output_size equals zsmalloc max size class
	 */
	i = 0;
	zbewalgo_max_output_size = 3264;
	zbewalgo_base_algorithms[i++] = alg_bewalgo;
	zbewalgo_base_algorithms[i++] = alg_bewalgo2;
	zbewalgo_base_algorithms[i++] = alg_bitshuffle;
	zbewalgo_base_algorithms[i++] = alg_bwt;
	zbewalgo_base_algorithms[i++] = alg_jbe;
	zbewalgo_base_algorithms[i++] = alg_jbe2;
	zbewalgo_base_algorithms[i++] = alg_mtf;
	zbewalgo_base_algorithms[i++] = alg_rle;
	zbewalgo_base_algorithms[i++] = alg_huffman;
	zbewalgo_base_algorithms_count = i;
	/*
	 * the wrkmem size is the largest wrkmem required by any callable
	 * algorithm
	 */
	zbewalgo_wrkmem_size = 0;
	for (i = 0; i < zbewalgo_base_algorithms_count; i++) {
		res = zbewalgo_base_algorithms[i].init();
		if (res) {
			if (i > 0)
				zbewalgo_base_algorithms[0].exit();
			i--;
			while (i > 0)
				zbewalgo_base_algorithms[i].exit();
			return res;
		}
		zbewalgo_base_algorithms[i].id = i;
		zbewalgo_wrkmem_size = MAX(
			zbewalgo_wrkmem_size,
			zbewalgo_base_algorithms[i].wrkmem_size);
	}
	/* adding some pages for temporary compression results */
	zbewalgo_wrkmem_size += 4096 * 6;
	zbewalgo_main_data_ptr = alloc_percpu(struct zbewalgo_main_data);
	for_each_possible_cpu(j) {
		memset(
			per_cpu_ptr(zbewalgo_main_data_ptr, j),
			0,
			sizeof(struct zbewalgo_main_data));
	}

	memset(&zbewalgo_stat_combination[0], 0, 256 * sizeof(atomic64_t));
	memset(&zbewalgo_stat_count[0], 0, 256 * sizeof(atomic64_t));
	init_settings();
	return res;
}

static void __exit zbewalgo_mod_fini(void)
{
	uint16_t i;
	uint64_t tmp;

	exit_settings();
	for (i = 0; i < zbewalgo_base_algorithms_count; i++)
		zbewalgo_base_algorithms[i].exit();
	free_percpu(zbewalgo_main_data_ptr);
	/* log statistics via printk for debugging purpose */
	for (i = 0; i < 256; i++) {
		tmp = atomic64_read(&zbewalgo_stat_combination[i]);
		if (tmp > 0)
			printk(KERN_INFO "%s %4d -> %llu combination\n",
				__func__, i, tmp);
	}
	for (i = 0; i < 256; i++) {
		tmp = atomic64_read(&zbewalgo_stat_count[i]);
		if (tmp > 0)
			printk(KERN_INFO "%s %4d -> %llu counter\n",
				__func__, i, tmp);
	}
}

#define compress(i, j, s, d, w, l) \
	(zbewalgo_base_algorithms[zbewalgo_combinations[i].ids[j]] \
		.compress(s, d, w, l))


__always_inline int zbewalgo_compress(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem,
	const uint16_t source_length)
{
	struct zbewalgo_main_data * const main_data =
		get_cpu_ptr(zbewalgo_main_data_ptr);
	uint8_t *dest_best = wrkmem;
	uint8_t *dest1 = dest_best + (4096 << 1);
	uint8_t *dest2 = dest1 + (4096 << 1);
	uint8_t * const wrk = dest2 + (4096 << 1);
	uint8_t *dest_tmp;
	const uint8_t *current_source;
	uint8_t i, j;
	uint16_t dest_best_size = source_length << 1;
	int dest_current_size;
	uint8_t dest_best_id = 0;
	uint8_t i_from = main_data->best_id
		+ (main_data->counter_search-- == 0);
	uint8_t i_to = zbewalgo_combinations_count;
	uint8_t looped = 0;
	uint16_t local_abort_size = MAX(
		main_data->best_id_accepted_size,
		zbewalgo_early_abort_size);
	uint16_t counter = 0;
	struct zbewalgo_combination * const dest_best_combination =
		(struct zbewalgo_combination *) dest;
_begin:
	/*
	 * loop from 0 to zbewalgo_combinations_count starting with the last
	 * successfull combination
	 */
	i = i_from;
	while (i < i_to) {
		current_source	  = source;
		dest_current_size = source_length;
		counter++;
		for (j = 0; j < zbewalgo_combinations[i].count; j++) {
			dest_current_size = compress(i, j,
				current_source,
				dest2,
				wrk,
				dest_current_size);
			if (dest_current_size <= 0)
				goto _next_algorithm;
			current_source = dest2;
			dest_tmp = dest2;
			dest2 = dest1;
			dest1 = dest_tmp;
			if (dest_current_size < dest_best_size) {
	/* if a higher compression ratio is found, update to the best */
				dest_best_id = i;
				dest_best_size = dest_current_size;
				dest_tmp = dest_best;
				dest_best = dest1;
				dest1 = dest_tmp;
				memcpy(
					dest_best_combination,
					&zbewalgo_combinations[i],
					sizeof(struct zbewalgo_combination));
	/* partial combination is allowed, if its compression ratio is high */
				dest_best_combination->count = j + 1;
			}
		}
		if (dest_best_size < local_abort_size)
			goto _early_abort;
_next_algorithm:
		local_abort_size = zbewalgo_early_abort_size;
		i++;
	}
	if (!(looped++) && (i_from > 0)) {
		i_to = MIN(i_from, zbewalgo_combinations_count);
		i_from = 0;
		goto _begin;
	}
	if (dest_best_size > zbewalgo_max_output_size)
		return -1;
_early_abort:
	atomic64_inc(&zbewalgo_stat_combination[dest_best_id]);
	atomic64_inc(&zbewalgo_stat_count[counter]);
	main_data->best_id = dest_best_id;
	main_data->best_id_accepted_size =
		MAX(
			dest_best_size + (dest_best_size >> 3),
			zbewalgo_early_abort_size);
	memcpy(
		dest + sizeof(struct zbewalgo_combination),
		dest_best,
		dest_best_size);
	return sizeof(struct zbewalgo_combination) + dest_best_size;
}
EXPORT_SYMBOL(zbewalgo_compress);

#define decompress(i, s, d, w) \
	(zbewalgo_base_algorithms[combination->ids[i]] \
		.decompress(s, d, w))

__always_inline int zbewalgo_decompress(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem)
{
	uint8_t *dest1 = wrkmem;
	uint8_t *dest2 = dest1 + (4096 << 1);
	uint8_t *wrk = dest2 + (4096 << 1);
	const struct zbewalgo_combination * const combination =
		(struct zbewalgo_combination *) source;
	uint8_t j;
	int res;

	if (combination->count == 1) {
		res = decompress(0,
			(source + sizeof(struct zbewalgo_combination)),
			dest,
			wrk);
		return res;
	}
	res = decompress(combination->count - 1,
		(source + sizeof(struct zbewalgo_combination)),
		dest1,
		wrk);
	for (j = combination->count - 2; j > 1; j -= 2) {
		res = decompress(j, dest1, dest2, wrk);
		res = decompress(j - 1, dest2, dest1, wrk);
	}
	if (j == 1) {
		res = decompress(1, dest1, dest2, wrk);
		res = decompress(0, dest2, dest, wrk);
		return res;
	}
	res = decompress(0, dest1, dest, wrk);
	return res;
}
EXPORT_SYMBOL(zbewalgo_decompress);

module_init(zbewalgo_mod_init);
module_exit(zbewalgo_mod_fini);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ZBeWalgo Compression Algorithm");
