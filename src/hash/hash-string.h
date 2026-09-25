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

#ifndef QTHASH_HASH_HASH_STRING_H
#define QTHASH_HASH_HASH_STRING_H

#include "hash-func.h"

void qthash_hash_string(struct hash_func_s *funcs, const char *str,
	enum digest_format_e format, const uint8_t *hmac_key, size_t key_size);

void qthash_hash_string_digest_cb(enum hash_func_e id, const char *digest);
void qthash_hash_string_finish_cb(void);

#endif
