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

#include "checkfile.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

// Ported from src/check.c's three GRegex patterns.
static const QRegularExpression &bsdRe()
{
	static const QRegularExpression re(
		QStringLiteral("^(?<FUNCTION>[A-Z0-9-]{3,16}) \\((?<FILENAME>.+)\\) = "
			"(?<DIGEST>[0-9a-fA-F]{8,})$"));
	return re;
}
static const QRegularExpression &gnuRe()
{
	static const QRegularExpression re(
		QStringLiteral("^(?<DIGEST>[0-9a-fA-F]{8,}) [* ]?(?<FILENAME>.+)$"));
	return re;
}
static const QRegularExpression &sfvRe()
{
	static const QRegularExpression re(
		QStringLiteral("^(?<FILENAME>[^;].*?[^ ]*)[ ]+(?<DIGEST>[0-9a-fA-F]{8})$"));
	return re;
}

// GNU/SFV lines don't name the function; the digest's hex length identifies the
// canonical one, which covers real-world *sum / .sfv files.
static enum hash_func_e idFromDigestLen(int hexlen)
{
	switch (hexlen) {
		case 8:   return HASH_FUNC_CRC32;
		case 32:  return HASH_FUNC_MD5;
		case 40:  return HASH_FUNC_SHA1;
		case 56:  return HASH_FUNC_SHA224;
		case 64:  return HASH_FUNC_SHA256;
		case 96:  return HASH_FUNC_SHA384;
		case 128: return HASH_FUNC_SHA512;
		default:  return HASH_FUNC_INVALID;
	}
}

static QString resolve(const QDir &dir, const QString &filename)
{
	return QDir::isAbsolutePath(filename) ? filename
		: QDir::cleanPath(dir.absoluteFilePath(filename));
}

QList<ChecksumEntry> parseChecksumFile(const QString &filePath)
{
	QList<ChecksumEntry> out;

	QFile f(filePath);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return out;

	const QDir dir = QFileInfo(filePath).absoluteDir();
	QTextStream in(&f);

	QString line;
	while (in.readLineInto(&line)) {
		if (line.isEmpty() || line.startsWith('#') || line.startsWith(';'))
			continue;

		QRegularExpressionMatch m = bsdRe().match(line);
		if (m.hasMatch()) {
			out.append({resolve(dir, m.captured("FILENAME")),
				m.captured("DIGEST"),
				hashcore_func_id_from_name(
					m.captured("FUNCTION").toUtf8().constData())});
			continue;
		}
		m = gnuRe().match(line);
		if (m.hasMatch()) {
			const QString digest = m.captured("DIGEST");
			out.append({resolve(dir, m.captured("FILENAME")), digest,
				idFromDigestLen(digest.size())});
			continue;
		}
		m = sfvRe().match(line);
		if (m.hasMatch()) {
			out.append({resolve(dir, m.captured("FILENAME")),
				m.captured("DIGEST"), HASH_FUNC_CRC32});
		}
	}

	return out;
}
