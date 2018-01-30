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

#ifndef ZBEWALGO_HUFFMAN_H
#define ZBEWALGO_HUFFMAN_H

#include "include.h"

/*
 * compression using the bare huffman encoding. Optimized for speed and small
 * input buffers
 */
static __always_inline int compress_huffman(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem,
	const uint16_t source_length)
{
	const uint8_t * const source_end = source + source_length;
	const uint8_t * const dest_end = dest + zbewalgo_max_output_size;
	short * const nodes_index = (short *) (wrkmem);
	uint16_t * const leaf_parent_index = (uint16_t *) (wrkmem + 1536);
	uint16_t * const nodes_weight = (uint16_t *) (wrkmem + 2048);
	uint16_t * const frequency = (uint16_t *) (wrkmem + 3072);
	uint16_t * const code_lengths = (uint16_t *) (wrkmem + 3584);
	uint32_t * const code_words = (uint32_t *) (wrkmem + 4096);
	short i;
	uint16_t node_index;
	int i2;
	short max_codeword_length;
	uint32_t weight2;
	short num_nodes;
	uint32_t bits_in_buffer;
	uint32_t bits_in_stack;
	uint16_t free_index;
	const uint8_t *ip;
	uint8_t *op;
	uint32_t stack;
#ifdef __LITTLE_ENDIAN
	uint8_t * const stack_ptr = (uint8_t *) &stack;
#endif
	memset(dest, 0, zbewalgo_max_output_size);
	memset(wrkmem, 0, 5120);
	op = dest;
	ip = source;
	*(uint16_t *) dest = source_length;
	do {
		frequency[*ip++]++;
	} while (ip < source_end);
	ip = source;
	num_nodes = 0;
	for (i = 0; i < 256; i++) {
		if (frequency[i] > 0) {
			i2 = num_nodes++;
			while ((i2 > 0) && (nodes_weight[i2] > frequency[i])) {
				nodes_weight[i2 + 1] = nodes_weight[i2];
				nodes_index[i2 + 1] = nodes_index[i2];
				leaf_parent_index[nodes_index[i2]]++;
				i2--;
			}
			i2++;
			nodes_index[i2] = -(i + 1);
			leaf_parent_index[-(i + 1)] = i2;
			nodes_weight[i2] = frequency[i];
		}
	}
	dest[2] = num_nodes;
	op = dest + 3;
	for (i = 1; i <= num_nodes; ++i) {
		*op++ = -(nodes_index[i] + 1);
		*(uint16_t *) op = nodes_weight[i];
		op += 2;
	}
	free_index = 2;
	while (free_index <= num_nodes) {
		weight2 = nodes_weight[free_index - 1]
			+ nodes_weight[free_index];
		i2 = num_nodes++;
		while ((i2 > 0) && (nodes_weight[i2] > weight2)) {
			nodes_weight[i2 + 1] = nodes_weight[i2];
			nodes_index[i2 + 1] = nodes_index[i2];
			leaf_parent_index[nodes_index[i2]]++;
			i2--;
		}
		i2++;
		nodes_index[i2] = free_index >> 1;
		leaf_parent_index[free_index >> 1] = i2;
		nodes_weight[i2] = weight2;
		free_index += 2;
	}
	if (num_nodes > 400)
		return -1;
	for (i = 0; i < 256; i++) {
		if (frequency[i] == 0)
			continue;
		bits_in_stack = 0;
		stack = 0;
		node_index = leaf_parent_index[-(i + 1)];
		while (node_index < num_nodes) {
			stack |= (node_index & 0x1) << bits_in_stack;
			bits_in_stack++;
			node_index = leaf_parent_index[(node_index + 1) >> 1];
		}
		code_lengths[i] = bits_in_stack;
		code_words[i] = stack;
	}
	max_codeword_length = 0;
	node_index = leaf_parent_index[nodes_index[1]];
	while (node_index < num_nodes) {
		node_index = leaf_parent_index[(node_index + 1) >> 1];
		max_codeword_length++;
	}
	if (max_codeword_length > 24)
		return -1;
	bits_in_buffer = 0;
	do {
		bits_in_stack = code_lengths[*ip];
		stack = code_words[*ip];
		ip++;
		bits_in_buffer += bits_in_stack;
		stack = stack << (32 - bits_in_buffer);
#ifdef __LITTLE_ENDIAN
		op[0] |= stack_ptr[3];
		op[1] |= stack_ptr[2];
		op[2] |= stack_ptr[1];
		op[3] |= stack_ptr[0];
#else
		*(uint32_t *) op |= stack;
#endif
		op += bits_in_buffer >> 3;
		bits_in_buffer = bits_in_buffer & 0x7;
		if (unlikely(op >= dest_end))
			return -1;
	} while (likely(ip < source_end));
	return op - dest + (bits_in_buffer > 0);
}

/*
 * reverses the huffman compression
 */
static __always_inline int decompress_huffman(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem)
{
	const uint32_t dest_length = *(uint16_t *) source;
	const uint8_t * const dest_end = dest + dest_length;
	short * const nodes_index = (short *) (wrkmem);
	uint16_t * const nodes_weight = (uint16_t *) (wrkmem + 1024);
	uint32_t i;
	int node_index;
	uint32_t i2;
	uint32_t weight2;
	uint32_t num_nodes;
	uint32_t current_bit;
	uint32_t free_index;
	const uint8_t *ip;
	uint8_t *op;

	memset(wrkmem, 0, 2048);
	num_nodes = source[2];
	ip = source + 3;
	op = dest;
	for (i = 1; i <= num_nodes; ++i) {
		nodes_index[i] = -(*ip++ + 1);
		nodes_weight[i] = *(uint16_t *) ip;
		ip += 2;
	}
	free_index = 2;
	while (free_index <= num_nodes) {
		weight2 = nodes_weight[free_index - 1]
			+ nodes_weight[free_index];
		i2 = num_nodes++;
		while (i2 > 0 && nodes_weight[i2] > weight2) {
			nodes_weight[i2 + 1] = nodes_weight[i2];
			nodes_index[i2 + 1] = nodes_index[i2];
			i2--;
		}
		i2++;
		nodes_index[i2] = free_index >> 1;
		nodes_weight[i2] = weight2;
		free_index += 2;
	}
	current_bit = 0;
	do {
		node_index = nodes_index[num_nodes];
		while (node_index > 0) {
			ip += current_bit == 8;
			current_bit = ((current_bit) & 0x7) + 1;
			node_index = nodes_index[(node_index << 1)
				- ((*ip >> (8 - current_bit)) & 0x1)];
		}
		*op++ = -(node_index + 1);
	} while (op < dest_end);
	return dest_length;
}

static int init_huffman(void)
{
	return 0;
}

static void exit_huffman(void)
{
}

static struct zbewalgo_alg alg_huffman = {
	.name = "huffman",
	.flags = ZBEWALGO_ALG_FLAG_COMPRESS,
	.wrkmem_size = 5120,
	.init = init_huffman,
	.exit = exit_huffman,
	.compress = compress_huffman,
	.decompress = decompress_huffman
};

#endif
