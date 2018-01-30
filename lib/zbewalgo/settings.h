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

#ifndef ZBEWALGO_SETTINGS_H
#define ZBEWALGO_SETTINGS_H

#include "include.h"

#define add_combination_compile_time(name) \
	zbewalgo_add_combination(name, sizeof(name))

static ssize_t zbewalgo_combinations_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	ssize_t res = 0;
	ssize_t tmp;
	uint8_t i, j;
	struct zbewalgo_combination *com;

	res = tmp = scnprintf(buf, PAGE_SIZE - res, "combinations={\n");
	buf += tmp;
	for (i = 0; i < zbewalgo_combinations_count; i++) {
		com = &zbewalgo_combinations[i];
		res += tmp = scnprintf(
			buf,
			PAGE_SIZE - res,
			"\tcombination[%d]=",
			i);
		buf += tmp;
		for (j = 0; j < com->count - 1; j++) {
			res += tmp = scnprintf(buf, PAGE_SIZE - res, "%s-",
				zbewalgo_base_algorithms[com->ids[j]].name);
			buf += tmp;
		}
		res += tmp = scnprintf(buf, PAGE_SIZE - res, "%s\n",
			zbewalgo_base_algorithms[com->ids[j]].name);
		buf += tmp;
	}
	res += tmp = scnprintf(buf, PAGE_SIZE - res, "}\n");
	return res;
}

static ssize_t zbewalgo_combinations_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	ssize_t res;

	count--;
	if (count < 5)
		return -1;
	if (memcmp(buf, "add ", 4) == 0) {
		res = zbewalgo_add_combination(buf + 4, count - 4);
		return res < 0 ? res : count+1;
	}
	if (memcmp(buf, "set ", 4) == 0) {
		res = zbewalgo_set_combination(buf + 4, count - 4);
		return res < 0 ? res : count+1;
	}
	if (memcmp(buf, "reset", 5) == 0) {
		zbewalgo_combinations_count = 0;
		add_combination_compile_time(
			"bewalgo2-bitshuffle-jbe-rle");
		add_combination_compile_time(
			"bwt-mtf-bewalgo-huffman");
		add_combination_compile_time(
			"bitshuffle-bewalgo2-mtf-bewalgo-jbe");
		add_combination_compile_time(
			"bitshuffle-rle-bitshuffle-rle");
		return count;
	}
	return -1;
}

static ssize_t zbewalgo_max_output_size_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu", zbewalgo_max_output_size);
}

static ssize_t zbewalgo_max_output_size_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	char localbuf[10];
	ssize_t res;

	memcpy(&localbuf[0], buf, count < 10 ? count : 10);
	localbuf[count < 9 ? count : 9] = 0;
	res = kstrtoul(localbuf, 10, &zbewalgo_max_output_size);
	return res < 0 ? res : count;
}

static ssize_t zbewalgo_early_abort_size_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu", zbewalgo_early_abort_size);
}

static ssize_t zbewalgo_early_abort_size_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	char localbuf[10];
	ssize_t res;

	memcpy(&localbuf[0], buf, count < 10 ? count : 10);
	localbuf[count < 9 ? count : 9] = 0;
	res = kstrtoul(localbuf, 10, &zbewalgo_early_abort_size);
	return res < 0 ? res : count;
}

static ssize_t zbewalgo_bwt_max_alphabet_show(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu", zbewalgo_bwt_max_alphabet);
}

static ssize_t zbewalgo_bwt_max_alphabet_store(
	struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	char localbuf[10];
	ssize_t res;

	memcpy(&localbuf[0], buf, count < 10 ? count : 10);
	localbuf[count < 9 ? count : 9] = 0;
	res = kstrtoul(localbuf, 10, &zbewalgo_bwt_max_alphabet);
	return res < 0 ? res : count;
}

static struct kobj_attribute zbewalgo_combinations_attribute = __ATTR(
	combinations,
	0664,
	zbewalgo_combinations_show,
	zbewalgo_combinations_store);
static struct kobj_attribute zbewalgo_max_output_size_attribute = __ATTR(
	max_output_size,
	0664,
	zbewalgo_max_output_size_show,
	zbewalgo_max_output_size_store);
static struct kobj_attribute zbewalgo_early_abort_size_attribute = __ATTR(
	early_abort_size,
	0664,
	zbewalgo_early_abort_size_show,
	zbewalgo_early_abort_size_store);
static struct kobj_attribute zbewalgo_bwt_max_alphabet_attribute = __ATTR(
	bwt_max_alphabet,
	0664,
	zbewalgo_bwt_max_alphabet_show,
	zbewalgo_bwt_max_alphabet_store);
static struct attribute *attrs[] = {
	&zbewalgo_combinations_attribute.attr,
	&zbewalgo_max_output_size_attribute.attr,
	&zbewalgo_early_abort_size_attribute.attr,
	&zbewalgo_bwt_max_alphabet_attribute.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *zbewalgo_kobj;

static int init_settings(void)
{
	int resval;

	zbewalgo_kobj = kobject_create_and_add("zbewalgo", kernel_kobj);
	if (!zbewalgo_kobj)
		return -ENOMEM;
	resval = sysfs_create_group(zbewalgo_kobj, &attr_group);
	if (resval) {
		kobject_put(zbewalgo_kobj);
		zbewalgo_combinations_store(
			zbewalgo_kobj,
			&zbewalgo_combinations_attribute,
			"reset",
			sizeof("reset"));
	}
	return resval;
}

static void exit_settings(void)
{
	kobject_put(zbewalgo_kobj);
}
#endif
