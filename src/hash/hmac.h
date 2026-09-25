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

#ifndef QTHASH_HASH_HMAC_H
#define QTHASH_HASH_HMAC_H

#ifndef IN_HASH_LIB
	#error "don't use directly"
#endif

#include <stdlib.h>
#include <stdint.h>

#include "hash-func.h"

void qthash_hmac_start(struct hash_func_s *func, const uint8_t *key,
	size_t key_size);
void qthash_hmac_stop(struct hash_func_s *func);
void qthash_hmac_finish(struct hash_func_s *func);

#endif
