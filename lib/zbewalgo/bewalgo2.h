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

#ifndef BEWALGO2_BEWALGO2_H
#define BEWALGO2_BEWALGO2_H

#include "include.h"
#define MAX_LITERALS ((zbewalgo_max_output_size >> 3) - 4)

#define OFFSET_BITS 9
#define OFFSET_SHIFT (16 - OFFSET_BITS)
#define MATCH_BIT_SHIFT 6
#define MATCH_BIT_MASK (1 << MATCH_BIT_SHIFT)
#define LENGTH_BITS 6
#define LENGTH_MASK ((1 << LENGTH_BITS) - 1)
#define LEFT 0
#define RIGHT 1
#define NEITHER 2
#define TREE_NODE_NULL 0xffff


/*
 * insert first node into an empty avl-tree
 * the function returns the index of the node in the preallocated array
 */
static __always_inline int avl_insert_first(
	uint64_t *wrk_literal,
	uint16_t *wrk_tree,
	uint16_t *treep,
	uint64_t target,
	uint16_t *treesize)
{
	uint16_t g_a_tree, g_o_tree2;

	g_a_tree = *treesize;
	g_o_tree2 = g_a_tree << 2;
	*treesize = *treesize + 1;
	wrk_tree[g_o_tree2 + 0] = TREE_NODE_NULL;
	wrk_tree[g_o_tree2 + 1] = TREE_NODE_NULL;
	wrk_tree[g_o_tree2 + 2] = NEITHER;
	wrk_literal[g_a_tree] = target;
	*treep = g_a_tree;
	return g_a_tree;
}

/*
 * insert a node into a non-empty avl-tree
 * for speed, the nodes are saved in preallocated arrays
 * the function returns the index of the node in the preallocated array
 */
static __always_inline int avl_insert(
	uint64_t *wrk_literal,
	uint16_t *wrk_tree,
	uint16_t *treep,
	uint64_t target,
	uint16_t *treesize)
{
	uint16_t *path_top[2];
	uint16_t g_a_tree, g_o_tree2, g_o_next_step;
	uint16_t r_1_arr[10];
	uint16_t path, path2, first[2], second;

	if (unlikely(target == wrk_literal[*treep]))
		return *treep;
	path_top[0] = treep;
	g_o_next_step = target > wrk_literal[*treep];
	g_o_tree2 = *treep << 2;
	path_top[wrk_tree[g_o_tree2 + 2] == NEITHER] = treep;
	treep = &wrk_tree[g_o_tree2 + g_o_next_step];
	if (unlikely(*treep == TREE_NODE_NULL))
		goto _insert_new_node;
_search_node:
	if (target == wrk_literal[*treep])
		return *treep;
	g_o_next_step = target > wrk_literal[*treep];
	g_o_tree2 = *treep << 2;
	path_top[wrk_tree[g_o_tree2 + 2] == NEITHER] = treep;
	treep = &wrk_tree[g_o_tree2 + g_o_next_step];
	if (likely(*treep != TREE_NODE_NULL))
		goto _search_node;
_insert_new_node:
	g_a_tree = *treesize;
	g_o_tree2 = g_a_tree << 2;
	*treesize = *treesize + 1;
	wrk_tree[g_o_tree2 + 0] = TREE_NODE_NULL;
	wrk_tree[g_o_tree2 + 1] = TREE_NODE_NULL;
	wrk_tree[g_o_tree2 + 2] = NEITHER;
	wrk_literal[g_a_tree] = target;
	*treep = g_a_tree;
	path = *path_top[0];
	path2 = path << 2;
	if (wrk_tree[path2 + 2] == NEITHER) {
		while (1) {
			r_1_arr[0] = target > wrk_literal[path];
			wrk_tree[path2 + 2] = r_1_arr[0];
			path = wrk_tree[path2 + r_1_arr[0]];
			if (target == wrk_literal[path])
				return g_a_tree;
			path2 = path << 2;
		}
	}
	first[0] = target > wrk_literal[path];
	if (wrk_tree[path2 + 2] != first[0]) {
		wrk_tree[path2 + 2] = NEITHER;
		r_1_arr[2] = wrk_tree[path2 + first[0]];
		if (target == wrk_literal[r_1_arr[2]])
			return g_a_tree;
		while (1) {
			r_1_arr[0] = target > wrk_literal[r_1_arr[2]];
			r_1_arr[1] = r_1_arr[2] << 2;
			r_1_arr[2] = wrk_tree[r_1_arr[1] + r_1_arr[0]];
			wrk_tree[r_1_arr[1] + 2] = r_1_arr[0];
			if (target == wrk_literal[r_1_arr[2]])
				return g_a_tree;
		}
	}
	second = target > wrk_literal[wrk_tree[path2 + first[0]]];
	first[1] = 1 - first[0];
	if (first[0] == second) {
		r_1_arr[2] = *path_top[0];
		r_1_arr[3] = r_1_arr[2] << 2;
		r_1_arr[4] = wrk_tree[r_1_arr[3] + first[0]];
		r_1_arr[5] = r_1_arr[4] << 2;
		wrk_tree[r_1_arr[3] + first[0]] =
			wrk_tree[r_1_arr[5] + first[1]];
		path = wrk_tree[r_1_arr[5] + first[0]];
		*path_top[0] = r_1_arr[4];
		wrk_tree[r_1_arr[5] + first[1]] = r_1_arr[2];
		wrk_tree[r_1_arr[3] + 2] = NEITHER;
		wrk_tree[r_1_arr[5] + 2] = NEITHER;
		if (target == wrk_literal[path])
			return g_a_tree;
		while (1) {
			r_1_arr[0] = target > wrk_literal[path];
			r_1_arr[1] = path << 2;
			wrk_tree[r_1_arr[1] + 2] = r_1_arr[0];
			path = wrk_tree[r_1_arr[1] + r_1_arr[0]];
			if (target == wrk_literal[path])
				return g_a_tree;
		}
	}
	path = wrk_tree[(wrk_tree[path2 + first[0]] << 2) + second];
	r_1_arr[5] = *path_top[0];
	r_1_arr[1] = r_1_arr[5] << 2;
	r_1_arr[8] = wrk_tree[r_1_arr[1] + first[0]];
	r_1_arr[0] = r_1_arr[8] << 2;
	r_1_arr[6] = wrk_tree[r_1_arr[0] + first[1]];
	r_1_arr[7] = r_1_arr[6] << 2;
	r_1_arr[2] = wrk_tree[r_1_arr[7] + first[1]];
	r_1_arr[3] = wrk_tree[r_1_arr[7] + first[0]];
	*path_top[0] = r_1_arr[6];
	wrk_tree[r_1_arr[7] + first[1]] = r_1_arr[5];
	wrk_tree[r_1_arr[7] + first[0]] = r_1_arr[8];
	wrk_tree[r_1_arr[1] + first[0]] = r_1_arr[2];
	wrk_tree[r_1_arr[0] + first[1]] = r_1_arr[3];
	wrk_tree[r_1_arr[7] + 2] = NEITHER;
	wrk_tree[r_1_arr[1] + 2] = NEITHER;
	wrk_tree[r_1_arr[0] + 2] = NEITHER;
	if (target == wrk_literal[path])
		return g_a_tree;
	r_1_arr[9] = (target > wrk_literal[path]) == first[0];
	wrk_tree[r_1_arr[r_1_arr[9]] + 2] = first[r_1_arr[9]];
	path = r_1_arr[r_1_arr[9] + 2];
	if (target == wrk_literal[path])
		return g_a_tree;
	while (1) {
		path2 = path << 2;
		r_1_arr[4] = target > wrk_literal[path];
		wrk_tree[path2 + 2] = r_1_arr[4];
		path = wrk_tree[path2 + r_1_arr[4]];
		if (target == wrk_literal[path])
			return g_a_tree;
	}
}

/*
 * compress the given data using a binary tree holding all the previously
 * seen 64-bit values
 * returns the length of the compressed data
 */
static __always_inline int compress_bewalgo2(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem,
	const uint16_t source_length)
{
	const uint64_t * const source_begin = (const uint64_t *) source;
	uint64_t *wrk_literal = (uint64_t *) wrkmem;
	uint16_t *wrk_tree = (uint16_t *) (wrkmem + PAGE_SIZE);
	uint16_t *op_match = (uint16_t *) (dest + 4);
	int count;
	uint16_t i;
	uint16_t j;
	uint16_t tree_root = TREE_NODE_NULL;
	uint16_t tree_size = 0;
	const uint64_t *ip = ((const uint64_t *) source)
		+ DIV_BY_8_ROUND_UP(source_length) - 1;

	/*
	 * add first value into the tree
	 */
	i = avl_insert_first(
		wrk_literal,
		wrk_tree,
		&tree_root,
		*ip,
		&tree_size);
	/*
	 * to gain performance abort if the data does not seem to be
	 * compressible
	 */
	if (source_length > 512) {
		/*
		 * verify that at there are at most 5 different values
		 * at the first 10 positions
		 */
		for (i = 2; i < 11; i++)
			avl_insert(
				wrk_literal,
				wrk_tree,
				&tree_root,
				ip[-i],
				&tree_size);
		if (tree_size < 6)
			goto _start;
		/*
		 * verify that there are at most 12 different values
		 * at the first 10 and last 10 positions
		 */
		for (i = 0; i < 10; i++)
			avl_insert(
				wrk_literal,
				wrk_tree,
				&tree_root,
				source_begin[i],
				&tree_size);
		if (tree_size < 13)
			goto _start;
		/*
		 * if the previous conditions do not pass, check some bytes
		 * in the middle for matches.
		 * if not enough matches found abort
		 */
		for (i = 0; i < 10; i++)
			avl_insert(
				wrk_literal,
				wrk_tree,
				&tree_root,
				source_begin[256 + i],
				&tree_size);
		if (tree_size >= 21)
			return -1;
	}
_start:
	i = 0;
_loop1_after_insert:
	count = 0;
	do {
		ip--;
		count++;
	} while (likely(ip > source_begin) && (*ip == wrk_literal[i]));
	if (count == 1) {
		count = 0;
		ip++;
		j = i + count;
		do {
			ip--;
			count++;
			j++;
		} while (likely(ip > source_begin)
			&& (j < tree_size)
			&& (*ip == wrk_literal[j]));
		ip++;
		count--;
		if (likely(ip > source_begin)) {
			do {
				ip--;
				count++;
				j = avl_insert(
					wrk_literal,
					wrk_tree,
					&tree_root,
					*ip,
					&tree_size);
				if (unlikely(tree_size > MAX_LITERALS))
					return -1;
			} while ((j == i + count)
				&& likely(ip > source_begin));
		}
		while (unlikely(count > LENGTH_MASK)) {
			*op_match++ = (i << OFFSET_SHIFT)
				+ MATCH_BIT_MASK
				+ LENGTH_MASK;
			count -= LENGTH_MASK;
			i += LENGTH_MASK;
		}
		*op_match++ = (i << OFFSET_SHIFT) + MATCH_BIT_MASK + count;
		if (unlikely(ip <= source_begin))
			goto _finish;
		i = j;
		goto _loop1_after_insert;
	} else {
		while (unlikely(count > LENGTH_MASK)) {
			*op_match++ = (i << OFFSET_SHIFT) + LENGTH_MASK;
			count -= LENGTH_MASK;
		}
		*op_match++ = (i << OFFSET_SHIFT) + count;
	}
	if (unlikely(ip <= source_begin))
		goto _finish;
	i = avl_insert(wrk_literal, wrk_tree, &tree_root, *ip, &tree_size);
	goto _loop1_after_insert;
_finish:
	j = avl_insert(wrk_literal, wrk_tree, &tree_root, *ip, &tree_size);
	*op_match++ = (j << OFFSET_SHIFT) + 1;
	((uint16_t *) dest)[0] = op_match - ((uint16_t *) dest);
	((uint16_t *) dest)[1] = source_length;
	count = sizeof(uint64_t) * (tree_size);
	memcpy(op_match, wrk_literal, count);
	return ((op_match - ((uint16_t *) dest)) << 1) + count;
}

/*
 * decompress the data and return the uncompressed length
 */
static __always_inline int decompress_bewalgo2(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem __attribute__ ((unused)))
{
	uint64_t *dest_anchor = (uint64_t *) dest;
	const uint16_t *ip_match = ((const uint16_t *) (source + 4));
	const uint16_t *ip_match_end =
		((const uint16_t *) source) + ((const uint16_t *) source)[0];
	const uint64_t *ip_literal = (uint64_t *) ip_match_end;
	uint16_t i;
	uint16_t count;
	uint64_t *op = dest_anchor
		+ DIV_BY_8_ROUND_UP(((uint16_t *) source)[1]) - 1;

	while (ip_match < ip_match_end) {
		i = (*ip_match) >> OFFSET_SHIFT;
		count = (*ip_match) & LENGTH_MASK;
		if ((*ip_match) & MATCH_BIT_MASK)
			while (count-- > 0)
				*op-- = ip_literal[i++];
		else
			while (count-- > 0)
				*op-- = ip_literal[i];
		ip_match++;
	}
	return ((const uint16_t *) source)[1];
}

static int init_bewalgo2(void)
{
	return 0;
}

static void exit_bewalgo2(void)
{
}

static struct zbewalgo_alg alg_bewalgo2 = {
	.name = "bewalgo2",
	.flags = ZBEWALGO_ALG_FLAG_COMPRESS,
	.wrkmem_size = 4096 << 1,
	.init = init_bewalgo2,
	.exit = exit_bewalgo2,
	.compress = compress_bewalgo2,
	.decompress = decompress_bewalgo2
};
#endif
