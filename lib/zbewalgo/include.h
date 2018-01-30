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

#ifndef ZBEWALGO_INCLUDE_H
#define ZBEWALGO_INCLUDE_H

#define ZBEWALGO_ALG_MAX_NAME 128
#define ZBEWALGO_ALG_FLAG_COMPRESS 1
#define ZBEWALGO_ALG_FLAG_TRANSFORM 2
#define ZBEWALGO_COMBINATION_MAX_IDS 7
#define ZBEWALGO_MAX_BASE_ALGORITHMS 16
#define ZBEWALGO_COMBINATION_MAX_ACTIVE 256

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define DIV_BY_8_ROUND_UP(val) ((val >> 3) + ((val & 0x7) > 0))

struct zbewalgo_alg {
	char name[ZBEWALGO_ALG_MAX_NAME];
	/* flag wheether this algorithm compresses the data or
	 * transforms the data only
	 */
	uint8_t flags;
	/* the wrkmem required for this algorithm */
	uint32_t wrkmem_size;
	int (*init)(void);
	void (*exit)(void);
	/* the compression function must store the size of
	 * input/output data itself
	 */
	int (*compress)(
		const uint8_t * const source,
		uint8_t * const dest,
		uint8_t * const wrkmem,
		const uint16_t source_length);
	int (*decompress)(
		const uint8_t * const source,
		uint8_t * const dest,
		uint8_t * const wrkmem);
	uint8_t id;
};

/*
 * to gain speed the compression starts with the algorithm wich was good for
 * the last compressed page.
 */
struct zbewalgo_combination {
	uint8_t count;
	uint8_t ids[ZBEWALGO_COMBINATION_MAX_IDS];
};

struct zbewalgo_main_data {
	/*
	 * best_id contains the id of the best combination for the last page
	 */
	uint8_t best_id;

	/*
	 * if zero search again for best_id - must be unsigned - underflow of
	 * values is intended
	 */
	uint8_t counter_search;

	/*
	 * a bit larger than original compressed size to be still accepted
	 * immediately
	 */
	uint16_t best_id_accepted_size;
};

/*
 * each cpu has its own independent compression history to avoid locks
 */
static struct zbewalgo_main_data __percpu *zbewalgo_main_data_ptr;

/*
 * all available algorithms
 */
static struct zbewalgo_alg
	zbewalgo_base_algorithms[ZBEWALGO_MAX_BASE_ALGORITHMS];
static uint8_t zbewalgo_base_algorithms_count;

/*
 * all currently available combination sequences of algorithms
 */
static struct zbewalgo_combination
	zbewalgo_combinations[ZBEWALGO_COMBINATION_MAX_ACTIVE];
static uint8_t zbewalgo_combinations_count;

/*
 * maximum required wrkmem for compression and decompression for each instance
 * of the algorithm
 */
static uint32_t zbewalgo_wrkmem_size;

/*
 * compression aborts automatically if the compressed data is too large.
 */
static unsigned long zbewalgo_max_output_size;

/*
 * compression can be aborted if the data is smaller than this theshold to
 * speed up the algorithm.
 */
static unsigned long zbewalgo_early_abort_size;

/*
 * If more than the specified number of chars are to be transformed,
 * it is unlikely that the compression will achieve high ratios.
 * As a consequence the Burrows-Wheeler Transformation will abort if the number
 * of different bytes exeeds this value.
 */
static unsigned long zbewalgo_bwt_max_alphabet = 90;

/*
 * add an combination to the current active ones. All combinations are the
 * same for all instances of this algorithm
 */
int zbewalgo_add_combination(
	const char * const string,
	const int string_length);

/*
 * replace ALL combinations with the one specified.
 */
int zbewalgo_set_combination(
	const char * const string,
	const int string_length);

#endif
