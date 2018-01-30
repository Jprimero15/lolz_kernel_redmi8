/*
 * Copyright (c) 2018 Benjamin Warnke <4bwarnke@informatik.uni-hamburg.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef ZBEWALGO_INCLUDE_H
#define ZBEWALGO_INCLUDE_H

/*
 * zbewalgo_get_wrkmem_size must be used to determine the size which is
 * required for allocating the working memory for the compression and
 * decompression algorithm
 */
int zbewalgo_get_wrkmem_size(void);

/*
 * this function compresses the data given by 'source' into the
 * preallocated memory given by 'dest'.
 * The maximum allowed source_length is 4096 Bytes. If larger values are
 * given, the resulting data might be corrupt.
 * If the data is not compressible the function returns -1. Otherwise the
 * size of the compressed data is returned.
 * wrkmem must point to a memory region of at least the size returned by
 * 'zbewalgo_get_wrkmem_size'.
 */
int zbewalgo_compress(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem,
	const uint16_t source_length);

/*
 * this function decompresses data which was already successfully compressed with
 * 'zbewalgo_compress'.
 * the function decompresses the data given by 'source' into the preallocated
 * buffer 'dest'.
 * wrkmem must point to a memory region of at least the size returned by
 * 'zbewalgo_get_wrkmem_size'.
 * the wrkmem for compression and decompression dont need to be the same
 */
int zbewalgo_decompress(
	const uint8_t * const source,
	uint8_t * const dest,
	uint8_t * const wrkmem);

#endif
