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

#ifndef QTHASH_HASHCORE_H
#define QTHASH_HASHCORE_H

// Toolkit-agnostic C seam over the QtHash hash engine.
//
// The engine (src/hash) calls up into fixed callback symbols
// (qthash_hash_file_*_cb / qthash_hash_string_*_cb). hashcore.c defines
// those once and forwards to the function pointers the UI registers here, so
// the engine stays untouched and the Qt layer never links against GTK glue.
//
// Every registered callback fires on the thread that runs the Qt/GLib event
// loop (the main thread) -- safe to touch widgets directly.

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "hash/hash-func.h"      // enum hash_func_e, HASH_FUNCS_N
#include "hash/digest-format.h"  // enum digest_format_e

#ifdef __cplusplus
extern "C" {
#endif

struct hashcore_callbacks {
	// File hashing (async). fraction is 0..1; text is a human-readable
	// throughput string or NULL early on -- do not free it.
	void (*file_progress)(void *user, double fraction, const char *text);
	void (*file_digest)(void *user, enum hash_func_e id, const char *digest);
	void (*file_finish)(void *user);  // completed
	void (*file_stop)(void *user);    // canceled

	// Text hashing (synchronous: digests then finish, before hashcore_hash_text
	// returns).
	void (*text_digest)(void *user, enum hash_func_e id, const char *digest);
	void (*text_finish)(void *user);

	void *user;  // opaque, passed back to every callback
};

void hashcore_init(const struct hashcore_callbacks *cb);
void hashcore_deinit(void);

// --- function metadata, for building the UI ---
int         hashcore_funcs_count(void);  // == HASH_FUNCS_N
const char *hashcore_func_name(enum hash_func_e id);
bool        hashcore_func_supported(enum hash_func_e id);
bool        hashcore_func_hmac_supported(enum hash_func_e id);

// --- selection ---
void         hashcore_func_set_enabled(enum hash_func_e id, bool enabled);
bool         hashcore_func_enabled(enum hash_func_e id);
unsigned int hashcore_funcs_count_enabled(void);
// Enable functions by name (e.g. from the CLI); unknown names are ignored.
enum hash_func_e hashcore_func_id_from_name(const char *name);

// --- hashing (hmac_key may be NULL / key_size 0 for plain hashing) ---
void hashcore_hash_file(const char *uri, enum digest_format_e format,
	const uint8_t *hmac_key, size_t key_size);
void hashcore_hash_text(const char *text, enum digest_format_e format,
	const uint8_t *hmac_key, size_t key_size);
void hashcore_cancel(void);

#ifdef __cplusplus
}
#endif

#endif
