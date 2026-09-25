/*
 *   Copyright (C) 2007-2016 Tristan Heaven <tristan@tristanheaven.net>
 *
 *   This file is part of QtHash.
 *
 *   QtHash is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   QtHash is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with QtHash. If not, see <https://gnu.org/licenses/gpl-2.0.txt>.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <glib.h>

#include "hmac.h"
#include "hash-func.h"
#include "hash-lib.h"
#include "digest.h"

void qthash_hmac_start(struct hash_func_s *func, const uint8_t *key,
	const size_t key_size)
{
	g_assert(func);
	g_assert(func->hmac_supported);
	g_assert(func->block_size > 0);
	g_assert(key);

	func->hmac_data = g_new(struct hash_func_s, 1);

	uint8_t padded_key[func->block_size];
	memset(padded_key, 0, func->block_size);

	if (key_size > func->block_size) {
		qthash_hash_func_init(func->hmac_data, func->id);
		func->hmac_data->enabled = true;

		qthash_hash_lib_start(func->hmac_data, NULL, 0);
		qthash_hash_lib_update(func->hmac_data, key, key_size);
		qthash_hash_lib_finish(func->hmac_data);

		size_t newsize = func->hmac_data->digest_size;
		g_assert((newsize > 0) && (newsize <= func->block_size));
		memcpy(padded_key, func->hmac_data->digest->bin, newsize);

		qthash_hash_func_deinit(func->hmac_data);
	} else
		memcpy(padded_key, key, key_size);

	uint8_t pad[func->block_size];

	for (int i = 0; i < func->block_size; i++)
		pad[i] = 0x36 ^ padded_key[i];
	qthash_hash_lib_update(func, pad, func->block_size);

	qthash_hash_func_init(func->hmac_data, func->id);
	func->hmac_data->enabled = true;
	qthash_hash_lib_start(func->hmac_data, NULL, 0);

	for (int i = 0; i < func->block_size; i++)
		pad[i] = 0x5c ^ padded_key[i];
	qthash_hash_lib_update(func->hmac_data, pad, func->block_size);
}

static void qthash_hmac_deinit(struct hash_func_s *func)
{
	qthash_hash_func_deinit(func->hmac_data);
	g_free(func->hmac_data);
	func->hmac_data = NULL;
}

void qthash_hmac_stop(struct hash_func_s *func)
{
	g_assert(func);
	g_assert(func->hmac_supported);
	g_assert(func->block_size > 0);
	g_assert(func->hmac_data);

	qthash_hmac_deinit(func);
}

void qthash_hmac_finish(struct hash_func_s *func)
{
	g_assert(func);
	g_assert(func->hmac_supported);
	g_assert(func->block_size > 0);
	g_assert(func->hmac_data);

	qthash_hash_lib_update(func->hmac_data, func->digest->bin, func->digest_size);
	qthash_hash_lib_finish(func->hmac_data);

	uint8_t *digest = g_memdup(func->hmac_data->digest->bin, func->hmac_data->digest_size);
	qthash_hash_func_set_digest(func, digest, func->hmac_data->digest_size);

	qthash_hmac_deinit(func);
}
