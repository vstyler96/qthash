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

#include "glib_qt.hpp"

#include <QTimer>
#include <glib.h>

// ponytail: polling pump, ~4ms. Correct (drains every ready GLib source each
// tick) but caps big-file throughput at buffer/interval, since one GIO async
// read completes per tick. Upgrade path when throughput measurably matters:
// g_main_context_prepare/query/check/dispatch driven off
// QAbstractEventDispatcher aboutToBlock/awake with QSocketNotifiers, which
// blocks in Qt and wakes the instant a GLib fd is ready.
static constexpr int kPumpIntervalMs = 4;

GlibQtPump::GlibQtPump(QObject *parent)
	: QObject(parent), timer_(new QTimer(this))
{
	timer_->setInterval(kPumpIntervalMs);
	connect(timer_, &QTimer::timeout, this, &GlibQtPump::iterate);
}

void GlibQtPump::start()
{
	if (!timer_->isActive())
		timer_->start();
}

void GlibQtPump::stop()
{
	timer_->stop();
}

void GlibQtPump::iterate()
{
	GMainContext *ctx = g_main_context_default();
	while (g_main_context_pending(ctx))
		g_main_context_iteration(ctx, FALSE);
}
