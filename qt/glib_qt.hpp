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

#ifndef QTHASH_GLIB_QT_HPP
#define QTHASH_GLIB_QT_HPP

#include <QObject>

class QTimer;

// Drives GLib's default main context from Qt's event loop so the C hash
// backend's GIO async reads and g_idle/g_timeout sources fire while a
// QCoreApplication/QApplication is running -- no GTK, no GLib main loop.
//
// Pump only while a hash is active: start() before kicking off hashing,
// stop() from the finish/stop callback.
class GlibQtPump : public QObject {
	Q_OBJECT
public:
	explicit GlibQtPump(QObject *parent = nullptr);
	void start();
	void stop();

private:
	void iterate();
	QTimer *timer_;
};

#endif
