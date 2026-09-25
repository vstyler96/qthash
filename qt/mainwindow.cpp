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

#include "mainwindow.hpp"
#include "glib_qt.hpp"
#include "checkfile.hpp"

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QAction>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

static const char *const kMatchQss =
	"background-color: rgba(0,170,0,0.35);";
static const char *const kNoMatchQss =
	"background-color: rgba(200,0,0,0.30);";
static const QString kAppVersion = QStringLiteral(QTHASH_VERSION);
static const QBrush kRowMatch(QColor(0, 170, 0, 90));
static const QBrush kRowNoMatch(QColor(200, 0, 0, 80));

#include <glib.h>

#include <algorithm>
#include <functional>

// --- hashcore callbacks -> the window (main thread) ---
namespace {
MainWindow *self(void *u) { return static_cast<MainWindow *>(u); }

void on_text_digest(void *u, enum hash_func_e id, const char *d)
{ self(u)->setTextDigest(id, d); }
void on_text_finish(void *) {}
void on_file_progress(void *u, double f, const char *t)
{ self(u)->onFileProgress(f, t); }
void on_file_digest(void *u, enum hash_func_e id, const char *d)
{ self(u)->setFileDigest(id, d); }
void on_file_finish(void *u) { self(u)->onFileFinish(); }
void on_file_stop(void *u) { self(u)->onFileStop(); }
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle(tr("QtHash"));
	resize(620, 680);
	setAcceptDrops(true);

	const hashcore_callbacks cb = {
		on_file_progress, on_file_digest, on_file_finish, on_file_stop,
		on_text_digest, on_text_finish, this,
	};
	hashcore_init(&cb);
	pump_ = new GlibQtPump(this);

	auto *central = new QWidget(this);
	auto *outer = new QVBoxLayout(central);

	// View selector.
	auto *viewRow = new QHBoxLayout;
	viewRow->addWidget(new QLabel(tr("View:")));
	fileRadio_ = new QRadioButton(tr("File"));
	textRadio_ = new QRadioButton(tr("Text"));
	listRadio_ = new QRadioButton(tr("File List"));
	fileRadio_->setChecked(true);
	viewRow->addWidget(fileRadio_);
	viewRow->addWidget(textRadio_);
	viewRow->addWidget(listRadio_);
	viewRow->addStretch();
	outer->addLayout(viewRow);

	// Input area: File / Text / List pages.
	inputStack_ = new QStackedWidget;

	auto *filePage = new QWidget;
	auto *fileRowL = new QHBoxLayout(filePage);
	fileRowL->setContentsMargins(0, 0, 0, 0);
	fileRowL->addWidget(new QLabel(tr("File:")));
	filePathEdit_ = new QLineEdit;
	filePathEdit_->setReadOnly(true);
	filePathEdit_->setPlaceholderText(tr("Drop a file here or Browse…"));
	fileRowL->addWidget(filePathEdit_);
	browseBtn_ = new QPushButton(tr("Browse…"));
	fileRowL->addWidget(browseBtn_);
	inputStack_->addWidget(filePage);  // 0

	auto *textPage = new QWidget;
	auto *textRowL = new QHBoxLayout(textPage);
	textRowL->setContentsMargins(0, 0, 0, 0);
	textRowL->addWidget(new QLabel(tr("Text:")));
	textInput_ = new QLineEdit;
	textRowL->addWidget(textInput_);
	inputStack_->addWidget(textPage);  // 1

	auto *listPage = new QWidget;
	auto *listRowL = new QHBoxLayout(listPage);
	listRowL->setContentsMargins(0, 0, 0, 0);
	addBtn_ = new QPushButton(tr("Add…"));
	removeBtn_ = new QPushButton(tr("Remove"));
	clearBtn_ = new QPushButton(tr("Clear"));
	listRowL->addWidget(addBtn_);
	listRowL->addWidget(removeBtn_);
	listRowL->addWidget(clearBtn_);
	listRowL->addStretch();
	inputStack_->addWidget(listPage);  // 2

	outer->addWidget(inputStack_);

	// Run controls (File / List views).
	progress_ = new QProgressBar;
	progress_->setRange(0, 100);
	outer->addWidget(progress_);

	auto *btnRow = new QHBoxLayout;
	hashBtn_ = new QPushButton(tr("Hash"));
	stopBtn_ = new QPushButton(tr("Stop"));
	btnRow->addWidget(hashBtn_);
	btnRow->addWidget(stopBtn_);
	btnRow->addStretch();
	outer->addLayout(btnRow);

	// Central output: function list (page 0) or file table (page 1).
	centralStack_ = new QStackedWidget;

	auto *scroll = new QScrollArea;
	scroll->setWidgetResizable(true);
	auto *grid = new QWidget;
	auto *g = new QGridLayout(grid);
	const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	int row = 0;
	for (int id = 0; id < hashcore_funcs_count(); id++) {
		if (!hashcore_func_supported((hash_func_e)id))
			continue;
		auto *check = new QCheckBox(hashcore_func_name((hash_func_e)id));
		auto *digest = new QLineEdit;
		digest->setReadOnly(true);
		digest->setFont(mono);
		if (HASH_FUNC_IS_DEFAULT(id))
			check->setChecked(true);
		g->addWidget(check, row, 0);
		g->addWidget(digest, row, 1);
		checks_[id] = check;
		digests_[id] = digest;
		connect(check, &QCheckBox::toggled, this, [this] { onFuncsChanged(); });
		row++;
	}
	scroll->setWidget(grid);
	centralStack_->addWidget(scroll);  // 0

	fileTable_ = new QTreeWidget;
	fileTable_->setRootIsDecorated(false);
	fileTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
	fileTable_->setColumnCount(1);
	fileTable_->setHeaderLabels({tr("File")});
	centralStack_->addWidget(fileTable_);  // 1

	outer->addWidget(centralStack_, 1);

	// Bottom: digest format + HMAC key (shared across views).
	auto *bottom = new QHBoxLayout;
	bottom->addWidget(new QLabel(tr("Format:")));
	formatCombo_ = new QComboBox;
	formatCombo_->addItem(tr("Hex (lowercase)"), DIGEST_FORMAT_HEX_LOWER);
	formatCombo_->addItem(tr("Hex (uppercase)"), DIGEST_FORMAT_HEX_UPPER);
	formatCombo_->addItem(tr("Base64"), DIGEST_FORMAT_BASE64);
	bottom->addWidget(formatCombo_);
	bottom->addSpacing(12);
	bottom->addWidget(new QLabel(tr("HMAC key:")));
	hmacInput_ = new QLineEdit;
	hmacInput_->setPlaceholderText(tr("(optional)"));
	bottom->addWidget(hmacInput_);
	outer->addLayout(bottom);

	auto *checkRow = new QHBoxLayout;
	checkRow->addWidget(new QLabel(tr("Check:")));
	checkInput_ = new QLineEdit;
	checkInput_->setPlaceholderText(tr("Paste a digest to verify"));
	checkRow->addWidget(checkInput_);
	outer->addLayout(checkRow);

	setCentralWidget(central);
	buildMenus();

	// Wiring.
	connect(fileRadio_, &QRadioButton::toggled, this,
		[this](bool on) { if (on) setView(View::File); });
	connect(textRadio_, &QRadioButton::toggled, this,
		[this](bool on) { if (on) setView(View::Text); });
	connect(listRadio_, &QRadioButton::toggled, this,
		[this](bool on) { if (on) setView(View::List); });
	connect(textInput_, &QLineEdit::textChanged, this,
		[this] { if (view_ == View::Text) rehashText(); });
	connect(hmacInput_, &QLineEdit::textChanged, this,
		[this] { onFormatChanged(); });
	connect(formatCombo_, &QComboBox::currentIndexChanged, this, [this] {
		format_ = (digest_format_e)formatCombo_->currentData().toInt();
		onFormatChanged();
	});
	connect(browseBtn_, &QPushButton::clicked, this, [this] {
		const QString p = QFileDialog::getOpenFileName(this, tr("Open File"));
		if (!p.isEmpty())
			startFileHash(p);
	});
	connect(hashBtn_, &QPushButton::clicked, this, [this] {
		if (view_ == View::List)
			startListHash();
		else if (!currentPath_.isEmpty())
			startFileHash(currentPath_);
	});
	connect(stopBtn_, &QPushButton::clicked, this,
		[this] { if (busy_) hashcore_cancel(); });
	connect(addBtn_, &QPushButton::clicked, this, [this] {
		addFiles(QFileDialog::getOpenFileNames(this, tr("Add Files")));
	});
	connect(removeBtn_, &QPushButton::clicked, this, [this] {
		if (busy_)
			return;
		const auto sel = fileTable_->selectionModel()->selectedRows();
		QList<int> rows;
		for (const auto &idx : sel)
			rows << idx.row();
		std::sort(rows.begin(), rows.end(), std::greater<int>());
		for (int r : rows)
			if (r >= 0 && r < listPaths_.size()) {
				listPaths_.removeAt(r);
				listExpected_.removeAt(r);
			}
		refreshTable();
	});
	connect(clearBtn_, &QPushButton::clicked, this, [this] {
		if (busy_)
			return;
		listPaths_.clear();
		listExpected_.clear();
		refreshTable();
	});
	connect(checkInput_, &QLineEdit::textChanged, this,
		[this] { updateCheckHighlight(); });

	loadSettings();
	setView(View::File);
}

MainWindow::~MainWindow()
{
	saveSettings();
	hashcore_deinit();
}

void MainWindow::buildMenus()
{
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
	QAction *open = fileMenu->addAction(tr("&Open…"));
	open->setShortcut(QKeySequence::Open);
	connect(open, &QAction::triggered, this, [this] {
		const QString p = QFileDialog::getOpenFileName(this, tr("Open File"));
		if (!p.isEmpty()) {
			fileRadio_->setChecked(true);
			startFileHash(p);
		}
	});
	QAction *openCheck = fileMenu->addAction(tr("Open &Checksum File…"));
	connect(openCheck, &QAction::triggered, this, [this] {
		const QString p = QFileDialog::getOpenFileName(this,
			tr("Open Checksum File"), QString(),
			tr("Checksum files (*.md5 *.sha1 *.sha256 *.sha512 *.sfv "
			   "*sum* *SUM*);;All files (*)"));
		if (!p.isEmpty())
			openChecksumFile(p);
	});
	fileMenu->addSeparator();
	QAction *quit = fileMenu->addAction(tr("&Quit"));
	quit->setShortcut(QKeySequence::Quit);
	connect(quit, &QAction::triggered, this, &QWidget::close);

	QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
	QAction *about = helpMenu->addAction(tr("&About"));
	connect(about, &QAction::triggered, this, [this] {
		QMessageBox::about(this, tr("About QtHash"),
			tr("<b>QtHash %1</b><br><br>"
			   "A utility for computing message digests or checksums.<br>"
			   "Qt6 port of GtkHash.<br><br>"
			   "Licensed under the GNU GPL v2 or later.").arg(kAppVersion));
	});
}

void MainWindow::loadSettings()
{
	QSettings s;
	restoreGeometry(s.value(QStringLiteral("geometry")).toByteArray());

	if (s.contains(QStringLiteral("functions"))) {
		const QStringList on =
			s.value(QStringLiteral("functions")).toStringList();
		for (int id = 0; id < HASH_FUNCS_N; id++)
			if (checks_[id])
				checks_[id]->setChecked(
					on.contains(hashcore_func_name((hash_func_e)id)));
	}

	const int fmt = s.value(QStringLiteral("format"), 0).toInt();
	if (fmt >= 0 && fmt < formatCombo_->count())
		formatCombo_->setCurrentIndex(fmt);
}

void MainWindow::saveSettings()
{
	QSettings s;
	s.setValue(QStringLiteral("geometry"), saveGeometry());
	s.setValue(QStringLiteral("format"), formatCombo_->currentIndex());

	QStringList on;
	for (int id = 0; id < HASH_FUNCS_N; id++)
		if (checks_[id] && checks_[id]->isChecked())
			on << hashcore_func_name((hash_func_e)id);
	s.setValue(QStringLiteral("functions"), on);
}

void MainWindow::updateCheckHighlight()
{
	const QString exp = checkInput_->text().trimmed();
	bool anyMatch = false;
	for (int id = 0; id < HASH_FUNCS_N; id++) {
		if (!digests_[id])
			continue;
		const QString d = digests_[id]->text();
		const bool match = !exp.isEmpty() && !d.isEmpty() &&
			d.compare(exp, Qt::CaseInsensitive) == 0;
		digests_[id]->setStyleSheet(match ? kMatchQss : QString());
		anyMatch = anyMatch || match;
	}
	checkInput_->setStyleSheet(exp.isEmpty() ? QString()
		: (anyMatch ? kMatchQss : kNoMatchQss));
}

void MainWindow::openPath(const QString &path)
{
	fileRadio_->setChecked(true);
	startFileHash(path);
}

void MainWindow::openList(const QStringList &paths)
{
	listRadio_->setChecked(true);
	addFiles(paths);
	startListHash();
}

void MainWindow::openChecksumFile(const QString &path)
{
	if (busy_)
		return;

	const QList<ChecksumEntry> entries = parseChecksumFile(path);
	if (entries.isEmpty()) {
		QMessageBox::warning(this, tr("Checksum File"),
			tr("No recognisable checksums found in:\n%1").arg(path));
		return;
	}

	// Parsed digests are always hex; compute in hex so they compare.
	formatCombo_->setCurrentIndex(0);  // DIGEST_FORMAT_HEX_LOWER

	listPaths_.clear();
	listExpected_.clear();
	for (const ChecksumEntry &e : entries) {
		listPaths_ << e.path;
		listExpected_.append(RowExpected{e.id, e.digest});
		if (HASH_FUNC_IS_VALID(e.id) && checks_[e.id])
			checks_[e.id]->setChecked(true);  // ensure it gets computed
	}

	listRadio_->setChecked(true);  // -> setView(List) -> refreshTable
	startListHash();
}

void MainWindow::setView(View view)
{
	view_ = view;
	inputStack_->setCurrentIndex(
		view == View::File ? 0 : view == View::Text ? 1 : 2);
	centralStack_->setCurrentIndex(view == View::List ? 1 : 0);

	const bool run = (view != View::Text);
	progress_->setVisible(run);
	hashBtn_->setVisible(run);
	stopBtn_->setVisible(run);

	clearDigests();
	if (view == View::List)
		refreshTable();
	else if (view == View::Text)
		rehashText();
}

void MainWindow::onFuncsChanged()
{
	if (busy_)
		return;
	if (view_ == View::Text)
		rehashText();
	else if (view_ == View::List)
		refreshTable();
	else
		clearDigests();  // File view: require an explicit re-Hash
}

void MainWindow::onFormatChanged()
{
	if (busy_)
		return;
	if (view_ == View::Text)
		rehashText();
	else if (view_ == View::List)
		refreshTable();
	else
		clearDigests();
}

void MainWindow::clearDigests()
{
	for (int id = 0; id < HASH_FUNCS_N; id++)
		if (digests_[id])
			digests_[id]->clear();
	updateCheckHighlight();
}

void MainWindow::syncEnabledFromChecks()
{
	for (int id = 0; id < HASH_FUNCS_N; id++)
		if (checks_[id])
			hashcore_func_set_enabled((hash_func_e)id,
				checks_[id]->isChecked());
}

void MainWindow::rehashText()
{
	syncEnabledFromChecks();
	clearDigests();
	if (hashcore_funcs_count_enabled() == 0)
		return;

	const QByteArray text = textInput_->text().toUtf8();
	const QByteArray key = hmacInput_->text().toUtf8();
	hashcore_hash_text(text.constData(), format_,
		key.isEmpty() ? nullptr : (const uint8_t *)key.constData(),
		key.isEmpty() ? 0 : (size_t)key.size());
}

void MainWindow::hashOneFile(const QString &path)
{
	char *uri = g_filename_to_uri(path.toUtf8().constData(), nullptr, nullptr);
	if (!uri)
		return;

	const QByteArray key = hmacInput_->text().toUtf8();
	pump_->start();
	hashcore_hash_file(uri, format_,
		key.isEmpty() ? nullptr : (const uint8_t *)key.constData(),
		key.isEmpty() ? 0 : (size_t)key.size());
	g_free(uri);
}

void MainWindow::startFileHash(const QString &path)
{
	if (busy_)
		return;
	currentPath_ = path;
	filePathEdit_->setText(path);
	syncEnabledFromChecks();
	clearDigests();
	if (hashcore_funcs_count_enabled() == 0)
		return;

	setBusy(true);
	progress_->setValue(0);
	hashOneFile(path);
}

void MainWindow::startListHash()
{
	if (busy_ || listPaths_.isEmpty())
		return;
	syncEnabledFromChecks();
	refreshTable();  // build columns for enabled funcs, clear digest cells
	if (hashcore_funcs_count_enabled() == 0)
		return;

	listIndex_ = 0;
	setBusy(true);
	progress_->setValue(0);
	hashOneFile(listPaths_.at(0));
}

void MainWindow::refreshTable()
{
	QStringList headers;
	headers << tr("File");
	for (int id = 0; id < HASH_FUNCS_N; id++)
		listCol_[id] = -1;
	for (int id = 0; id < hashcore_funcs_count(); id++) {
		if (checks_[id] && checks_[id]->isChecked() &&
			hashcore_func_supported((hash_func_e)id)) {
			listCol_[id] = headers.size();
			headers << hashcore_func_name((hash_func_e)id);
		}
	}

	fileTable_->clear();
	fileTable_->setColumnCount(headers.size());
	fileTable_->setHeaderLabels(headers);
	for (const QString &path : listPaths_) {
		auto *item = new QTreeWidgetItem(fileTable_);
		item->setText(0, QFileInfo(path).fileName());
		item->setToolTip(0, path);
	}
}

void MainWindow::addFiles(const QStringList &paths)
{
	if (busy_ || paths.isEmpty())
		return;
	listPaths_ += paths;
	for (int i = 0; i < paths.size(); i++)
		listExpected_.append(RowExpected{});  // no expected digest
	refreshTable();
}

void MainWindow::onFileProgress(double fraction, const char *text)
{
	progress_->setValue((int)(fraction * 100.0));
	progress_->setFormat(text ? QString::fromUtf8(text)
		: QStringLiteral("%p%"));
}

void MainWindow::setFileDigest(enum hash_func_e id, const char *digest)
{
	if (view_ == View::List) {
		if (listCol_[id] >= 0 && listIndex_ < fileTable_->topLevelItemCount()) {
			QTreeWidgetItem *item = fileTable_->topLevelItem(listIndex_);
			item->setText(listCol_[id], digest);
			if (listIndex_ < listExpected_.size()) {
				const RowExpected &e = listExpected_[listIndex_];
				if (e.id == id && !e.digest.isEmpty()) {
					const bool ok = e.digest.compare(
						QString::fromUtf8(digest), Qt::CaseInsensitive) == 0;
					const QBrush &b = ok ? kRowMatch : kRowNoMatch;
					for (int c = 0; c < fileTable_->columnCount(); c++)
						item->setBackground(c, b);
				}
			}
		}
	} else if (digests_[id]) {
		digests_[id]->setText(digest);
		updateCheckHighlight();
	}
}

void MainWindow::setTextDigest(enum hash_func_e id, const char *digest)
{
	if (digests_[id]) {
		digests_[id]->setText(digest);
		updateCheckHighlight();
	}
}

void MainWindow::onFileFinish()
{
	if (view_ == View::List && ++listIndex_ < listPaths_.size()) {
		progress_->setValue(0);
		hashOneFile(listPaths_.at(listIndex_));
		return;
	}

	pump_->stop();
	progress_->setValue(100);
	progress_->setFormat(QStringLiteral("%p%"));
	setBusy(false);
}

void MainWindow::onFileStop()
{
	pump_->stop();
	progress_->setValue(0);
	progress_->setFormat(QStringLiteral("%p%"));
	setBusy(false);
}

void MainWindow::setBusy(bool busy)
{
	busy_ = busy;
	browseBtn_->setEnabled(!busy);
	hashBtn_->setEnabled(!busy);
	stopBtn_->setEnabled(busy);
	addBtn_->setEnabled(!busy);
	removeBtn_->setEnabled(!busy);
	clearBtn_->setEnabled(!busy);
	fileRadio_->setEnabled(!busy);
	textRadio_->setEnabled(!busy);
	listRadio_->setEnabled(!busy);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	const QMimeData *m = event->mimeData();
	if (!busy_ && m->hasUrls() && m->urls().first().isLocalFile())
		event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
	QStringList paths;
	for (const QUrl &u : event->mimeData()->urls())
		if (u.isLocalFile())
			paths << u.toLocalFile();
	if (paths.isEmpty())
		return;

	if (view_ == View::List) {
		addFiles(paths);
	} else {
		fileRadio_->setChecked(true);
		startFileHash(paths.first());
	}
}
