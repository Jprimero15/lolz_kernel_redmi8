/*
 * Cryptographic API.
 *
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

#include <crypto/internal/scompress.h>
#include <linux/crypto.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include "linux/zbewalgo.h"


struct zbewalgo_ctx {
	void *zbewalgo_comp_mem;
};

static void *zbewalgo_alloc_ctx(struct crypto_scomp *tfm)
{
	void *ctx;

	ctx = vmalloc(zbewalgo_get_wrkmem_size());
	if (!ctx)
		return ERR_PTR(-ENOMEM);
	return ctx;
}

static int zbewalgo_init(struct crypto_tfm *tfm)
{
	struct zbewalgo_ctx *ctx = crypto_tfm_ctx(tfm);

	ctx->zbewalgo_comp_mem = zbewalgo_alloc_ctx(NULL);
	if (IS_ERR(ctx->zbewalgo_comp_mem))
		return -ENOMEM;
	return 0;
}

static void zbewalgo_free_ctx(struct crypto_scomp *tfm, void *ctx)
{
	vfree(ctx);
}

static void zbewalgo_exit(struct crypto_tfm *tfm)
{
	struct zbewalgo_ctx *ctx = crypto_tfm_ctx(tfm);

	zbewalgo_free_ctx(NULL, ctx->zbewalgo_comp_mem);
}

static int __zbewalgo_compress_crypto(
	const u8 *src,
	unsigned int slen,
	u8 *dst,
	unsigned int *dlen,
	void *ctx)
{
	int out_len;
	if(slen > 4096)
		return -EINVAL;
	out_len = zbewalgo_compress(src, dst, ctx, slen);
	if (!out_len)
		return -EINVAL;
	*dlen = out_len;
	return 0;
}

static int zbewalgo_scompress(
	struct crypto_scomp *tfm,
	const u8 *src,
	unsigned int slen,
	u8 *dst,
	unsigned int *dlen,
	void *ctx)
{
	return __zbewalgo_compress_crypto(src, slen, dst, dlen, ctx);
}

static int zbewalgo_compress_crypto(
	struct crypto_tfm *tfm,
	const u8 *src,
	unsigned int slen,
	u8 *dst,
	unsigned int *dlen)
{
	struct zbewalgo_ctx *ctx = crypto_tfm_ctx(tfm);

	return __zbewalgo_compress_crypto(
		src,
		slen,
		dst,
		dlen,
		ctx->zbewalgo_comp_mem);
}

static int __zbewalgo_decompress_crypto(
	const u8 *src,
	unsigned int slen,
	u8 *dst,
	unsigned int *dlen,
	void *ctx)
{
	int out_len;

	out_len = zbewalgo_decompress(src, dst, ctx);
	if (out_len < 0)
		return -EINVAL;
	return 0;
}

static int zbewalgo_sdecompress(
	struct crypto_scomp *tfm,
	const u8 *src,
	unsigned int slen,
	u8 *dst,
	unsigned int *dlen,
	void *ctx)
{
	return __zbewalgo_decompress_crypto(src, slen, dst, dlen, ctx);
}

static int zbewalgo_decompress_crypto(
	struct crypto_tfm *tfm,
	const u8 *src,
	unsigned int slen,
	u8 *dst,
	unsigned int *dlen)
{
	struct zbewalgo_ctx *ctx = crypto_tfm_ctx(tfm);

	return __zbewalgo_decompress_crypto(
		src,
		slen,
		dst,
		dlen,
		ctx->zbewalgo_comp_mem);
}

static struct crypto_alg crypto_alg_zbewalgo = {
	.cra_name = "zbewalgo",
	.cra_flags = CRYPTO_ALG_TYPE_COMPRESS,
	.cra_ctxsize = sizeof(struct zbewalgo_ctx),
	.cra_module = THIS_MODULE,
	.cra_init = zbewalgo_init,
	.cra_exit = zbewalgo_exit,
	.cra_u = {
		.compress = {
			.coa_compress = zbewalgo_compress_crypto,
			.coa_decompress = zbewalgo_decompress_crypto
		}
	}
};

static struct scomp_alg scomp = {
	.alloc_ctx = zbewalgo_alloc_ctx,
	 .free_ctx = zbewalgo_free_ctx,
	 .compress = zbewalgo_scompress,
	 .decompress = zbewalgo_sdecompress,
	 .base = {
		 .cra_name = "zbewalgo",
		 .cra_driver_name = "zbewalgo-scomp",
		 .cra_module = THIS_MODULE,
	}
};

static int __init zbewalgo_mod_init(void)
{
	int ret;

	ret = crypto_register_alg(&crypto_alg_zbewalgo);
	if (ret)
		return ret;
	ret = crypto_register_scomp(&scomp);
	if (ret) {
		crypto_unregister_alg(&crypto_alg_zbewalgo);
		return ret;
	}
	return ret;
}

static void __exit zbewalgo_mod_fini(void)
{
	crypto_unregister_alg(&crypto_alg_zbewalgo);
	crypto_unregister_scomp(&scomp);
}

module_init(zbewalgo_mod_init);
module_exit(zbewalgo_mod_fini);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ZBeWalgo Compression Algorithm");
MODULE_ALIAS_CRYPTO("zbewalgo");
