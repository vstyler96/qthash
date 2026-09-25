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

#include <QApplication>

#include "mainwindow.hpp"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	QApplication::setApplicationName(QStringLiteral("QtHash"));
	QApplication::setApplicationVersion(QStringLiteral(QTHASH_VERSION));
	QApplication::setOrganizationName(QStringLiteral("QtHash"));
	QApplication::setOrganizationDomain(QStringLiteral("kingbeencent.dev"));
	// Binds the window to dev.kingbeencent.qthash.desktop so Wayland/Plasma
	// show the right name and taskbar icon (sets the Wayland app_id / WM class).
	QApplication::setDesktopFileName(QStringLiteral("dev.kingbeencent.qthash"));

	MainWindow window;
	window.show();

	const QStringList args = app.arguments();
	if (args.size() >= 3 && args.at(1) == QLatin1String("--check"))
		window.openChecksumFile(args.at(2));
	else if (args.size() == 2)
		window.openPath(args.at(1));
	else if (args.size() > 2)
		window.openList(args.mid(1));

	return app.exec();
}
