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

#ifndef QTHASH_CHECKFILE_HPP
#define QTHASH_CHECKFILE_HPP

#include <QList>
#include <QString>

#include "hashcore.h"  // enum hash_func_e

// One line of a checksum file: the referenced file, its expected digest, and
// the hash function to verify it with (inferred from the BSD tag, or the
// digest length for GNU/SFV lines).
struct ChecksumEntry {
	QString path;             // resolved (relative paths are joined to dir)
	QString digest;           // expected digest, verbatim
	enum hash_func_e id;      // function to verify with, or HASH_FUNC_INVALID
};

// Parse a BSD / GNU coreutils / SFV checksum file. Returns one entry per
// recognised line (empty list if the file can't be read or nothing matched).
QList<ChecksumEntry> parseChecksumFile(const QString &filePath);

#endif
