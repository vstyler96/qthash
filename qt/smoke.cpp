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

// Phase 0-2 proof: drive the C hash engine through the hashcore seam, under
// Qt's event loop, no GTK. Validates the callback contract for both the
// (synchronous) text path and the (async) file path.
//
//   qthash-smoke <path>

#include "glib_qt.hpp"
#include "hashcore.h"

#include <QCoreApplication>
#include <QTimer>
#include <cstdio>
#include <glib.h>

namespace {
GlibQtPump *g_pump = nullptr;

void on_file_progress(void *, double fraction, const char *)
{
	fprintf(stderr, "\r  %.1f%%", 100.0 * fraction);
}

void on_file_digest(void *, enum hash_func_e id, const char *digest)
{
	printf("file  %-10s %s\n", hashcore_func_name(id), digest);
}

void on_file_finish(void *)
{
	fprintf(stderr, "\r        \r");
	g_pump->stop();
	QCoreApplication::quit();
}

void on_file_stop(void *)
{
	fprintf(stderr, "canceled\n");
	g_pump->stop();
	QCoreApplication::exit(1);
}

void on_text_digest(void *, enum hash_func_e id, const char *digest)
{
	printf("text  %-10s %s\n", hashcore_func_name(id), digest);
}

void on_text_finish(void *) {}
} // namespace

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	if (argc != 2) {
		fprintf(stderr, "usage: %s <path>\n", argv[0]);
		return 2;
	}

	char *uri = g_filename_to_uri(
		g_canonicalize_filename(argv[1], nullptr), nullptr, nullptr);
	if (!uri) {
		fprintf(stderr, "bad path: %s\n", argv[1]);
		return 2;
	}

	const hashcore_callbacks cb = {
		on_file_progress, on_file_digest, on_file_finish, on_file_stop,
		on_text_digest, on_text_finish, nullptr,
	};
	hashcore_init(&cb);

	for (int id = 0; id < hashcore_funcs_count(); id++)
		if (hashcore_func_supported((hash_func_e)id) &&
			HASH_FUNC_IS_DEFAULT(id))
			hashcore_func_set_enabled((hash_func_e)id, true);

	g_pump = new GlibQtPump(&app);

	// Text path is synchronous; run it up front.
	hashcore_hash_text("abc", DIGEST_FORMAT_HEX_LOWER, nullptr, 0);

	// File path is async; kick off once the loop (and pump) are running.
	QTimer::singleShot(0, &app, [uri]() {
		g_pump->start();
		hashcore_hash_file(uri, DIGEST_FORMAT_HEX_LOWER, nullptr, 0);
		g_free(uri);
	});

	int rc = app.exec();
	hashcore_deinit();
	return rc;
}
