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

// Headless checks for the two non-trivial units the Qt port adds:
// the hashcore text path and the checksum-file parser. No Qt event loop or GUI.

#include "hashcore.h"
#include "checkfile.hpp"

#include <QTemporaryFile>
#include <QTextStream>

#include <cstdio>
#include <map>
#include <string>

#define CHECK(c) do { if (!(c)) { \
	fprintf(stderr, "FAIL: %s (%s:%d)\n", #c, __FILE__, __LINE__); \
	return 1; } } while (0)

static std::map<int, std::string> g_text;
static void t_digest(void *, enum hash_func_e id, const char *d)
{ g_text[id] = d; }
static void t_finish(void *) {}

static int test_hashcore_text()
{
	for (int id = 0; id < hashcore_funcs_count(); id++)
		hashcore_func_set_enabled((hash_func_e)id, false);
	hashcore_func_set_enabled(HASH_FUNC_MD5, true);
	hashcore_func_set_enabled(HASH_FUNC_SHA1, true);
	hashcore_func_set_enabled(HASH_FUNC_SHA256, true);

	g_text.clear();
	hashcore_hash_text("abc", DIGEST_FORMAT_HEX_LOWER, nullptr, 0);

	// Known FIPS test vectors for "abc".
	if (hashcore_func_supported(HASH_FUNC_MD5))
		CHECK(g_text[HASH_FUNC_MD5] ==
			"900150983cd24fb0d6963f7d28e17f72");
	if (hashcore_func_supported(HASH_FUNC_SHA1))
		CHECK(g_text[HASH_FUNC_SHA1] ==
			"a9993e364706816aba3e25717850c26c9cd0d89d");
	if (hashcore_func_supported(HASH_FUNC_SHA256))
		CHECK(g_text[HASH_FUNC_SHA256] ==
			"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	return 0;
}

static int test_checkfile()
{
	QTemporaryFile f;
	CHECK(f.open());
	{
		QTextStream out(&f);
		out << "ad8a58aaa1216679adfd94556d79ab6416e229e042a66d9a984f1d6a569a8608"
			   "  a.txt\n";                                 // GNU -> SHA256
		out << "SHA1 (b.txt) = "
			   "34e829d1c403f5533b4831bf732e44dc8324f70a\n"; // BSD -> SHA1
		out << "; a comment\n";                             // skipped
		out << "crc.txt 12345678\n";                        // SFV -> CRC32
	}
	f.flush();

	const QList<ChecksumEntry> e = parseChecksumFile(f.fileName());
	CHECK(e.size() == 3);
	CHECK(e[0].id == HASH_FUNC_SHA256);
	CHECK(e[0].path.endsWith("a.txt"));
	CHECK(e[1].id == HASH_FUNC_SHA1);
	CHECK(e[1].path.endsWith("b.txt"));
	CHECK(e[2].id == HASH_FUNC_CRC32);
	CHECK(e[2].digest == "12345678");
	return 0;
}

int main()
{
	const hashcore_callbacks cb = {
		nullptr, nullptr, nullptr, nullptr, t_digest, t_finish, nullptr,
	};
	hashcore_init(&cb);

	int rc = test_hashcore_text();
	if (!rc)
		rc = test_checkfile();

	hashcore_deinit();
	if (!rc)
		printf("all qthash tests passed\n");
	return rc;
}
