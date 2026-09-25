/*
 *   Copyright (C) 2026 QtHash contributors
 *
 *   This file is part of QtHash.
 *
 *   QtHash is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <glib.h>

#include "hashcore.h"
#include "hash/hash-func.h"
#include "hash/hash-string.h"
#include "hash/hash-file.h"
#include "util/util.h"

// This module is the bridge between the hash engine and the Qt layer:
// it owns the funcs array + the file-hash handle, and turns the engine's
// fixed up-calls into registered function-pointer callbacks.

static struct hash_func_s funcs[HASH_FUNCS_N];
static struct hash_file_s *hfile = NULL;
static struct hashcore_callbacks cbs;

void hashcore_init(const struct hashcore_callbacks *cb)
{
	g_assert(cb);
	cbs = *cb;

	qthash_hash_func_init_all(funcs);
	hfile = qthash_hash_file_new(funcs);
}

void hashcore_deinit(void)
{
	qthash_hash_file_free(hfile);
	hfile = NULL;
	qthash_hash_func_deinit_all(funcs);
}

int hashcore_funcs_count(void)
{
	return HASH_FUNCS_N;
}

const char *hashcore_func_name(enum hash_func_e id)
{
	g_assert(HASH_FUNC_IS_VALID(id));
	return funcs[id].name;
}

bool hashcore_func_supported(enum hash_func_e id)
{
	g_assert(HASH_FUNC_IS_VALID(id));
	return funcs[id].supported;
}

bool hashcore_func_hmac_supported(enum hash_func_e id)
{
	g_assert(HASH_FUNC_IS_VALID(id));
	return funcs[id].hmac_supported;
}

void hashcore_func_set_enabled(enum hash_func_e id, bool enabled)
{
	g_assert(HASH_FUNC_IS_VALID(id));
	funcs[id].enabled = enabled && funcs[id].supported;
}

bool hashcore_func_enabled(enum hash_func_e id)
{
	g_assert(HASH_FUNC_IS_VALID(id));
	return funcs[id].enabled;
}

unsigned int hashcore_funcs_count_enabled(void)
{
	unsigned int n = 0;
	for (int i = 0; i < HASH_FUNCS_N; i++)
		if (funcs[i].enabled)
			n++;
	return n;
}

enum hash_func_e hashcore_func_id_from_name(const char *name)
{
	return qthash_hash_func_get_id_from_name(name);
}

void hashcore_hash_file(const char *uri, enum digest_format_e format,
	const uint8_t *hmac_key, size_t key_size)
{
	g_assert(hashcore_funcs_count_enabled());

	// The engine holds the uri for the whole async op; hand it an owned copy,
	// passed as cb_data too, freed once in the finish/stop up-call.
	char *uri_dup = g_strdup(uri);
	qthash_hash_file(hfile, uri_dup, format, hmac_key, key_size, uri_dup);
}

void hashcore_hash_text(const char *text, enum digest_format_e format,
	const uint8_t *hmac_key, size_t key_size)
{
	if (!hashcore_funcs_count_enabled())
		return;

	qthash_hash_string(funcs, text, format, hmac_key, key_size);
}

void hashcore_cancel(void)
{
	qthash_hash_file_cancel(hfile);
}

//
// Up-calls from the engine -> registered callbacks
//

void qthash_hash_file_report_cb(G_GNUC_UNUSED void *data,
	const goffset file_size, const goffset total_read, GTimer *timer)
{
	if (!cbs.file_progress)
		return;

	const double fraction = file_size ?
		(double)total_read / (double)file_size : 0.0;

	const double elapsed = g_timer_elapsed(timer, NULL);
	if (elapsed <= 1) {
		cbs.file_progress(cbs.user, fraction, NULL);
		return;
	}

	char *text = qthash_format_progress(file_size, total_read, elapsed);
	cbs.file_progress(cbs.user, fraction, text);
	g_free(text);
}

void qthash_hash_file_digest_cb(const enum hash_func_e id, const char *digest,
	G_GNUC_UNUSED void *data)
{
	if (cbs.file_digest)
		cbs.file_digest(cbs.user, id, digest);
}

void qthash_hash_file_finish_cb(void *uri)
{
	g_free(uri);
	if (cbs.file_finish)
		cbs.file_finish(cbs.user);
}

void qthash_hash_file_stop_cb(void *uri)
{
	g_free(uri);
	if (cbs.file_stop)
		cbs.file_stop(cbs.user);
}

void qthash_hash_string_digest_cb(const enum hash_func_e id,
	const char *digest)
{
	if (cbs.text_digest)
		cbs.text_digest(cbs.user, id, digest);
}

void qthash_hash_string_finish_cb(void)
{
	if (cbs.text_finish)
		cbs.text_finish(cbs.user);
}
