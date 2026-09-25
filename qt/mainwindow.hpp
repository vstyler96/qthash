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

#ifndef QTHASH_MAINWINDOW_HPP
#define QTHASH_MAINWINDOW_HPP

#include <QMainWindow>
#include <QString>
#include <QStringList>

#include "hashcore.h"  // enum hash_func_e / digest_format_e, HASH_FUNCS_N

class QLineEdit;
class QCheckBox;
class QComboBox;
class QRadioButton;
class QStackedWidget;
class QPushButton;
class QProgressBar;
class QTreeWidget;
class GlibQtPump;

// Part 3a: shell + function list + Text view.
// Part 3b: File view (open / drag-drop / progress / cancel), async via pump.
// Part 3c: File-list view (multi-file table) + digest-format + HMAC key.
class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

	// Load a file into the File view and hash it (CLI / drop).
	void openPath(const QString &path);
	// Load several files into the File-list view and hash them (CLI).
	void openList(const QStringList &paths);
	// Load a checksum file, hash its referenced files, verify each (CLI / menu).
	void openChecksumFile(const QString &path);

	// hashcore callbacks -- all fire on the main thread.
	void setTextDigest(enum hash_func_e id, const char *digest);
	void setFileDigest(enum hash_func_e id, const char *digest);
	void onFileProgress(double fraction, const char *text);
	void onFileFinish();
	void onFileStop();

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;

private:
	enum class View { File, Text, List };

	void buildMenus();
	void loadSettings();
	void saveSettings();
	void updateCheckHighlight();
	void setView(View view);
	void onFuncsChanged();
	void onFormatChanged();
	void rehashText();
	void startFileHash(const QString &path);
	void startListHash();
	void hashOneFile(const QString &path);
	void refreshTable();
	void addFiles(const QStringList &paths);
	void clearDigests();
	void syncEnabledFromChecks();
	void setBusy(bool busy);

	View view_ = View::File;
	enum digest_format_e format_ = DIGEST_FORMAT_HEX_LOWER;

	QRadioButton *fileRadio_ = nullptr;
	QRadioButton *textRadio_ = nullptr;
	QRadioButton *listRadio_ = nullptr;
	QStackedWidget *inputStack_ = nullptr;
	QStackedWidget *centralStack_ = nullptr;
	QLineEdit *textInput_ = nullptr;
	QLineEdit *filePathEdit_ = nullptr;
	QLineEdit *hmacInput_ = nullptr;
	QLineEdit *checkInput_ = nullptr;
	QComboBox *formatCombo_ = nullptr;
	QPushButton *browseBtn_ = nullptr;
	QPushButton *hashBtn_ = nullptr;
	QPushButton *stopBtn_ = nullptr;
	QPushButton *addBtn_ = nullptr;
	QPushButton *removeBtn_ = nullptr;
	QPushButton *clearBtn_ = nullptr;
	QProgressBar *progress_ = nullptr;
	QTreeWidget *fileTable_ = nullptr;
	QCheckBox *checks_[HASH_FUNCS_N] = {};
	QLineEdit *digests_[HASH_FUNCS_N] = {};
	GlibQtPump *pump_ = nullptr;
	bool busy_ = false;
	QString currentPath_;

	// File-list state.
	struct RowExpected {
		enum hash_func_e id = HASH_FUNC_INVALID;  // verify against this func
		QString digest;                           // expected digest, or empty
	};
	QStringList listPaths_;
	QList<RowExpected> listExpected_;  // parallel to listPaths_
	int listIndex_ = 0;
	int listCol_[HASH_FUNCS_N] = {};  // table column per func id, or -1
};

#endif
