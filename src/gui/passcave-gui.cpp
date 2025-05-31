#include "passcave-gui.h"
#include "ui_passcave-gui.h"

#include "config.h"
#include "utils.h"
#include "document.h"
#include "utils2fa.h"

#include "includes/runguard.h"
#include "includes/preferences.h"
#include "includes/datamodel.h"

#include "forms/preferencesdialog.h"
#include "forms/propertiesdialog.h"
#include "forms/passworddialog.h"
#include "forms/aboutdialog.h"
#include "forms/longtextdialog.h"
#include "forms/addnewnodedialog.h"
#include "forms/loadingdialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFuture>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QTimer>
#include <QTranslator>
#include <QUrl>

#include <fstream>
#include <limits>

// There was an issue introduced in QT 5.13
// See https://bugreports.qt.io/browse/QTBUG-76850
#define FIX_SORT_QT_BUG

using namespace passcave;

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);

	Preferences::updateLanguage();
	a.setApplicationName(QString::fromStdString(passcave_APPLICATION_NAME));

	RunGuard guard("passcave-gui");
	if (!guard.tryToRun()) {
		QMessageBox::warning(nullptr, QObject::tr("Error"), QObject::tr("%1 is already running!").arg(passcave_APPLICATION_NAME));
		return 0;
	}

	MainWindow w;
	w.show();

	return a.exec();
}

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
	ui->setupUi(this);
	isWindowJustOpened = true;
	initUI();
	openNewFile();
}

MainWindow::~MainWindow() {
	trayIcon->hide();
	delete trayActionShowHide;
	delete trayIconMenu;
	delete trayIcon;
	delete ui;
	delete tableHeaderContextMenu;

	this->openedFileInfo.secureErase();
	secureErase(this->openedFileClearPass);
}

bool MainWindow::closeFile(bool exiting, bool spontaneousClose) {
	if (!confirmClose(exiting, spontaneousClose))
		return false;
	return closeFileImmediately();
}

bool MainWindow::closeFileImmediately() {
	ui->searchContainer->setVisible(false);
	this->setFilename("");
	this->openedFileInfo.secureErase();
	secureErase(this->openedFileClearPass);
	this->openedFileClearPass = std::string();
	this->setModified(false);
	return true;
}

bool MainWindow::confirmClose(bool exiting, bool spontaneousClose) {
	if (!hasUnsavedModifications()) {
		if (exiting && (Preferences::isConfirmBeforeExit() || (spontaneousClose && Preferences::isConfirmBeforeSpontaneousExit()))) {
			QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Confirmation"), tr("Are you sure you want to exit?"),
																	  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
			if (reply == QMessageBox::Yes)
				return true;
			else
				return false;
		} else
			return true;
	}

	if (Preferences::isAutoSave()) {
		saveOrSaveAs();
	} else {
		QString m1 = tr("There are unsaved modifications.");
		QString m2 = hasOpenFile() ? (
						exiting ? tr("Save modifications to \"%1\" before exiting?").arg(QString::fromStdString(openedFile)) :
								  tr("Save modifications to \"%1\" before closing file?").arg(QString::fromStdString(openedFile))
					) : (
						 exiting ? tr("Save modifications before exiting?") :
								   tr("Save modifications before closing current file?")
					);

		QMessageBox box(QMessageBox::Question, tr("Confirmation"), m1, QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
		box.setDefaultButton(QMessageBox::Cancel);
		box.setInformativeText(m2);
		int reply = box.exec();
		if (reply == QMessageBox::Discard)
			return true;
		else if (reply == QMessageBox::Cancel)
			return false;
		else
			saveOrSaveAs();
	}

	if (hasUnsavedModifications()) {
		QString m1 = hasOpenFile() ?
					tr("An error occured while saving modifications to \"%1\".").arg(QString::fromStdString(openedFile)) :
					tr("An error occured while saving modifications.");
		QString m2 = exiting ?
					tr("Do you still want to exit?") :
					tr("Do you still want to close current file?");

		QMessageBox box(QMessageBox::Question, tr("Confirmation"), m1, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
		box.setDefaultButton(QMessageBox::Cancel);
		box.setInformativeText(m2);
		int reply = box.exec();
		if (reply == QMessageBox::Yes)
			return true;
		else
			return false;
	}
	return true;
}

bool MainWindow::confirmReplace(std::string filename) {
	QString m1 = tr("The file \"%1\" already exists.").arg(QString::fromStdString(filename));
	QString m2 = tr("Do you want to replace it?");

	QMessageBox box(QMessageBox::Question, tr("Confirmation"), m1, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
	box.setInformativeText(m2);
	box.setDefaultButton(QMessageBox::Cancel);
	int reply = box.exec();
	if (reply == QMessageBox::Yes)
		return true;
	else
		return false;
}

QString MainWindow::get2FAInformation() const {
    if (ui->tableView->selectionModel()->selectedRows().size() != 1)
        return QString();

    QAction* a = dynamic_cast<QAction*>(QObject::sender());

    DataModel* d = static_cast<DataModel*>(ui->tableView->model());
    DocumentNodeId nodeId = d->getNodeIdFromRowIndex(ui->tableView->selectionModel()->selectedRows().first().row());

    return QString::fromStdString(d->getDocument()->getProperty(nodeId, DefaultDocumentPropertyDefinitions::OTPAUTH.name)).trimmed();
}

bool MainWindow::hasOpenFile() {
	return !openedFile.empty();
}

bool MainWindow::hasUnsavedModifications() {
	return isModified;
}

void MainWindow::loadDocumentToUi(Document* document) {
	DataModel* dataModel = new DataModel(document);
	QItemSelectionModel* m = ui->tableView->selectionModel();
	QAbstractItemModel* a = ui->tableView->model();
	ui->tableView->setModel(dataModel);
	delete m;
	delete a;

#ifdef FIX_SORT_QT_BUG
	static_cast<DataModel*>(ui->tableView->model())->sort(0, Qt::SortOrder::AscendingOrder);
#endif
	ui->tableView->sortByColumn(0, Qt::SortOrder::AscendingOrder);
	ui->tableView->horizontalHeader()->setSortIndicator(0, Qt::SortOrder::AscendingOrder);
	ui->tableView->selectRow(0);

	connect(dataModel, SIGNAL(dataChanged(QModelIndex const&, QModelIndex const&, QVector<int> const&)),
			this, SLOT(onDataChanged(QModelIndex const&, QModelIndex const&, QVector<int> const&)));

	connect(dataModel, SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)), this, SLOT(onLayoutAboutToBeChanged()));
	connect(dataModel, SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)), this, SLOT(onLayoutChanged()));

	connect(ui->tableView->selectionModel(),  SIGNAL(selectionChanged(QItemSelection const&, QItemSelection const&)),
			this, SLOT(onSelectionChanged(QItemSelection const&, QItemSelection const&)));

	updateColumnsChanged();

}

bool MainWindow::openFile(std::string filename) {
	if (!fileExists(filename)) {
		QString m1 = tr("Cannot open file.");
		QString m2 = tr("The file \"%1\" does not exist!").arg(QString::fromStdString(filename));

		QMessageBox box(QMessageBox::Warning, tr("Error"), m1, QMessageBox::Ok, this);
		box.setInformativeText(m2);
		box.exec();
		Preferences::removeRecentFile(filename);
		updateUI();
		return false;
	}

	std::string passwd;
	if (QFileInfo(QString::fromStdString(filename)).suffix().toLower().compare("xml") != 0) {
		bool ok;
		passwd = requestPassword(tr("Enter password to open %1").arg(QFileInfo(QString::fromStdString(filename)).fileName()), &ok);
		if (!ok)
			return false;
	}
	bool r = openFile(filename, passwd);
	if (!r) {
		QString m1 = tr("An error occured while opening file \"%1\".").arg(QString::fromStdString(filename));
		QString m2 = tr("The file might be corrupted, or you have entered a bad password.\n"
						"Do you want to retry?");

		QMessageBox box(QMessageBox::Question, tr("Question"), m1, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
		box.setDefaultButton(QMessageBox::Yes);
		box.setInformativeText(m2);
		int reply = box.exec();
		if (reply == QMessageBox::Yes)
			return openFile(filename);
		else
			return false;
	}
	return true;
}

bool MainWindow::openFile(std::string filename, std::string clearPassword) {
	gcrypt_FileInfo fileInfo;
	std::vector<char> decrypted;
	if (clearPassword.empty()) {
		std::ifstream inFile(filename, std::ios::in | std::ios::binary | std::ios::ate);
		if (inFile) {
			std::ifstream::pos_type size = inFile.tellg();
			if (size < 0) {
				inFile.close();
				return false;
			}
			inFile.seekg(0, std::ios::beg);
			decrypted.resize(static_cast<size_t>(size));
			if (!inFile.read(decrypted.data(), size)) {
				inFile.close();
				return false;
			}
			inFile.close();
		} else
			return false;
	} else {
		LoadingDialog l(this, tr("Opening file ..."));
		QFutureWatcher<void> watcher;
		connect(&watcher, SIGNAL(finished()), &l, SLOT(close()));
		QFuture<std::vector<char>> future = QtConcurrent::run(gcrypt_decryptFromFile,
													filename,
													clearPassword,
													&fileInfo);
		watcher.setFuture(future);
		l.exec();

		decrypted = future.result();
		if (decrypted.empty()) {
			fileInfo.secureErase();
			secureErase(clearPassword);
			return false;
		}
	}
	Document* d = nullptr;
	try {
		d = new Document(decrypted);
	} catch(DocumentException const&) {
		if (d != nullptr)
			delete d;
		secureErase(clearPassword);
		return false;
	}
	closeFileImmediately();
	this->setModified(clearPassword.empty()); // set modified if we have opened an XML document
	this->setFilename(clearPassword.empty() ? "" : filename);
	if (!clearPassword.empty()) {
		this->openedFileInfo = fileInfo;
		this->openedFileClearPass = clearPassword;
	} else {
		this->openedFileClearPass = "";
	}
	Preferences::addRecentFile(filename);
	loadDocumentToUi(d);
	updateUI();
	secureErase(clearPassword);
	return true;
}

void MainWindow::openMostRecentFile() {
	std::string f = Preferences::getMostRecentFile();
	if (f.empty())
		return;

	if (!confirmClose())
		return;

	openFile(f);
}

void MainWindow::openNewFile() {
	if (!closeFile())
		return;

	setModified(false);
	setFilename("");

	loadDocumentToUi(newEmptyDocument());
	updateUI();
}

std::string MainWindow::requestPassword(QString label, bool* ok) {
	PasswordDialog d(this, label, true);
	d.exec();

	if (ok != nullptr)
		*ok = d.isApplied();

	if (d.isApplied())
		return d.getPassword();
	else
		return "";
}

void MainWindow::saveAs(bool asACopy) {
	QFileDialog* d = createFileDialog(QFileDialog::AcceptSave);
	d->setWindowTitle(tr("Save file as"));
	if (d->exec()) {
		QStringList l = d->selectedFiles();
		if (l.size() == 1) {
			QString filename = l[0];
			QFileInfo fi(filename);
			if (!d->selectedNameFilter().contains("(*)")) {
				if (d->selectedNameFilter().contains("xml", Qt::CaseInsensitive)) {
					if (fi.suffix().compare("xml", Qt::CaseInsensitive) != 0)
						filename = filename + ".xml";
				} else if (d->selectedNameFilter().contains(QString::fromStdString(GCRY_PASSCAVE_EXT), Qt::CaseInsensitive)) {
					if (fi.suffix().compare(QString::fromStdString(GCRY_PASSCAVE_EXT), Qt::CaseInsensitive) != 0)
						filename = filename + "." + QString::fromStdString(GCRY_PASSCAVE_EXT);
				}
			}

			if (fileExists(filename.toStdString()) && !confirmReplace(filename.toStdString())) {
				delete d;
				return;
			}
			gcrypt_FileInfo info;
			if (filename.endsWith(".xml", Qt::CaseInsensitive)) {
				saveFile(filename.toStdString(), "", info, asACopy);
			} else {
				PasswordDialog p(this, tr("Enter encrypting details to save file"), false);
				p.exec();
				if (p.isApplied() == false) {
					delete d;
					return;
				}
				info.gcry_cipher_algo = p.getCipher();
				info.gcry_cipher_mode = p.getCipherMode();
				info.gcry_md_algo = p.getMessageDigest();
				info.gcry_md_iterations = p.getMessageDigestIterations();
				info.gcry_md_iterations_auto = p.isMessageDigestIterationsAuto();
				saveFile(filename.toStdString(), p.getPassword(), info, asACopy);
				info.secureErase();
			}
		}
	}
	delete d;
}

void MainWindow::saveCopyAs() {
	saveAs(true);
}

bool MainWindow::saveFile(std::string filename, std::string clearPassword, gcrypt_FileInfo fileInfo, bool asACopy) {
	std::string ext = filename.size() > 4 ? simple_lowercase(filename.substr(filename.size() - 4)) : "";

	if (clearPassword.empty()) {
		if (ext.compare(".xml") != 0) {
			filename = filename + ".xml";
			if (fileExists(filename) && !confirmReplace(filename))
				return false;
		}

		std::vector<char> v = static_cast<DataModel*>(ui->tableView->model())->getDocument()->buildXml();

		std::ofstream outFile(filename, std::ios::out | std::ios::binary | std::ios::trunc);
		if (outFile) {
			if (!outFile.write(v.data(), v.size())) {
				outFile.close();
				secureErase(v);
				displaySaveError(filename);
				return false;
			}
			outFile.close();
		} else {
			secureErase(v);
			displaySaveError(filename);
			return false;
		}
		secureErase(v);
	} else {
		if (ext.compare("." + GCRY_PASSCAVE_EXT) != 0) {
			filename = filename + "." + GCRY_PASSCAVE_EXT;
			if (fileExists(filename) && !confirmReplace(filename))
				return false;
		}

		std::vector<char> v = static_cast<DataModel*>(ui->tableView->model())->getDocument()->buildXml();

		LoadingDialog l(this, tr("Saving file ..."));
		QFutureWatcher<void> watcher;
		connect(&watcher, SIGNAL(finished()), &l, SLOT(close()));
		QFuture<bool> future = QtConcurrent::run(gcrypt_encryptToFile,
													filename,
													clearPassword,
													&fileInfo,
													v.data(), v.size());
		watcher.setFuture(future);
		l.exec();

		bool r = future.result();
		if (!r) {
			secureErase(v);
			displaySaveError(filename);
			return false;
		}
		secureErase(v);
	}

	Preferences::addRecentFile(filename);

	if (asACopy || clearPassword.empty()) {
		if (hasOpenFile())
			Preferences::addRecentFile(this->openedFile);
		fileInfo.secureErase();
		secureErase(clearPassword);
		updateUI();
		return true;
	}

	this->setModified(clearPassword.empty()); // set modified if we have opened an XML document
	this->setFilename(filename);
	if (!clearPassword.empty()) {
		this->openedFileInfo = fileInfo;
		this->openedFileClearPass = clearPassword;
	} else {
		this->openedFileClearPass = "";
	}
	fileInfo.secureErase();
	secureErase(clearPassword);
	updateUI();
	return true;
}

void MainWindow::saveOrSaveAs() {
	if (hasOpenFile() && !this->openedFileClearPass.empty())
		saveFile(this->openedFile, this->openedFileClearPass, this->openedFileInfo);
	else
		saveAs();
}

void MainWindow::updateColumnsChanged() {
	DataModel* dataModel = static_cast<DataModel*>(ui->tableView->model());

	ui->menuDisplayColumns->clear();
	tableHeaderContextMenu->clear();

	ui->menuSort->clear();

	for (int i = 0; i< dataModel->columnCount(); i++) {
		QString h = dataModel->headerData(i, Qt::Orientation::Horizontal).toString();

		{
			QAction* act = new QAction(this);
			act->setText(h);
			act->setCheckable(true);
			act->setChecked(!dataModel->isColumnHidden(i));
			if (act->isChecked())
				ui->tableView->showColumn(i);
			else
				ui->tableView->hideColumn(i);
			act->setData(QVariant(i));
			connect(act, SIGNAL(triggered()), this, SLOT(onDisplayColumnClick()));

			ui->menuDisplayColumns->addAction(act);
			tableHeaderContextMenu->addAction(act);
		}

		{
			QAction* act = new QAction(this);
			act->setText(h);
			act->setData(QVariant(i));
			if (i <= 9)
				act->setShortcut(QKeySequence(Qt::CTRL + (Qt::Key_0 + i)));
			connect(act, SIGNAL(triggered(bool)), this, SLOT(onSortMenuClick()));

			ui->menuSort->addAction(act);
		}
	}

	ui->comboBox_search->blockSignals(true);
	ui->comboBox_search->clear();
	ui->comboBox_search->addItem(tr("All columns"));
	ui->comboBox_search->setCurrentIndex(0);
	for (DocumentPropertyDefinition const& pDef: dataModel->getDocument()->getPropertyDefinitions())
		ui->comboBox_search->addItem(QString::fromStdString(DataModel::formatHeader(pDef.name)));
	ui->comboBox_search->blockSignals(false);
	updateFilter();

	updateColumnsWidth();

	updateCopyMenu();
}

void MainWindow::updateColumnsWidth() {
	if (ui->tableView->model()->columnCount() == 0)
		return;
	int width = this->width();

	ui->tableView->horizontalHeader()->setStretchLastSection(false);

	ui->tableView->resizeColumnsToContents();

	std::vector<int> indexes;

	for (int i = 0; i < ui->tableView->model()->columnCount(); i++)
		if (!ui->tableView->isColumnHidden(i))
			indexes.push_back(i);

	int lastColumn = indexes[indexes.size() - 1];

	std::sort(indexes.begin(), indexes.end(), [this](int const& i1, int const& i2) {
		return this->ui->tableView->columnWidth(i1) < this->ui->tableView->columnWidth(i2);
	});

	int cnt = indexes.size();
	for (int const& i: indexes) {
		int maxW = width / cnt-- - 1;
		int w = ui->tableView->columnWidth(i);
		if (w > maxW)
			w = maxW;
		ui->tableView->setColumnWidth(i, w);
		width-=w;
	}
	width = width / indexes.size() - 1;
	if (width > 0)
		for (int const& i: indexes) {
			int w = ui->tableView->columnWidth(i);
			ui->tableView->setColumnWidth(i, w + width);
		}
	ui->tableView->setColumnWidth(lastColumn, 1);
	ui->tableView->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::updateFilter() {
	DataModel* d = static_cast<DataModel*>(ui->tableView->model());
	QStringList columns;
	if (!ui->searchContainer->isVisible()) {
		d->filterData("", columns, false);
		updateUI();
		return;
	}
	if (ui->comboBox_search->currentIndex() != 0)
		columns.push_back(ui->comboBox_search->currentText());
	d->filterData(ui->lineEdit_search->text(), columns, ui->checkBox_search->isChecked());
	updateUI();
	updateColumnsWidth();
}

void MainWindow::changeEvent(QEvent* e) {
	switch (e->type()) {
	case QEvent::LanguageChange:
		this->ui->retranslateUi(this);
		break;
	case QEvent::WindowStateChange:
		if (this->windowState() & Qt::WindowMinimized) {
			QTimer::singleShot(0, this, SLOT(onShowHideClick()));
		}
		break;
	default:
		break;
	}

	QMainWindow::changeEvent(e);
}

void MainWindow::closeEvent(QCloseEvent* e) {
	if (e->spontaneous()) {
		if (Preferences::isMinimizeToTrayOnSpontaneousExit()) {
			e->ignore();
			QTimer::singleShot(0, this, SLOT(onShowHideClick()));
			return;
		}
	}

	if (!closeFile(true, e->spontaneous())) {
		e->ignore();
		return;
	}

	if (Preferences::isSaveWindowDisposition()) {
		Preferences::getQSettings().setValue("WindowGeometry", saveGeometry());
		Preferences::getQSettings().setValue("WindowState", saveState());
	}

	QMainWindow::closeEvent(e);
}

bool MainWindow::event(QEvent* e) {
	bool const r = QMainWindow::event(e);

	if (this->isWindowJustOpened && e->type() == QEvent::Paint) {
		this->isWindowJustOpened = false;
		QTimer::singleShot(0, this, SLOT(onOpenMostRecentFile()));
	}
	return r;
}

void MainWindow::hideEvent(QHideEvent* event) {
	QMainWindow::hideEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
	updateColumnsWidth();

	QMainWindow::resizeEvent(event);
}

void MainWindow::showEvent(QShowEvent* event) {
	QMainWindow::showEvent(event);
}

QFileDialog* MainWindow::createFileDialog(QFileDialog::AcceptMode mode) {
	QString dir = this->openedFile.empty() ? QApplication::instance()->applicationDirPath() : (QFileInfo(QString::fromStdString(this->openedFile)).absoluteDir().absolutePath());
	QFileDialog* d = new QFileDialog(this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	//d->setFixedSize(d.size());
	d->setOption(QFileDialog::DontConfirmOverwrite, true);
	d->setAcceptMode(mode);
	d->setDefaultSuffix(QString::fromStdString(GCRY_PASSCAVE_EXT));
	d->setDirectory(dir);
	d->setFileMode(mode == QFileDialog::AcceptOpen ? QFileDialog::ExistingFile : QFileDialog::AnyFile);

	QStringList hist;
	for (std::string s: Preferences::getRecentFiles())
		hist.push_back(QString::fromStdString(s));
	d->setHistory(hist);

	QStringList filters;
	filters << tr("%2 Encrypted Files (*.%1)")
					.arg(QString::fromStdString(GCRY_PASSCAVE_EXT))
					.arg(QString::fromStdString(passcave_APPLICATION_NAME))
			<< tr("XML Files (*.xml)")
			<< tr("Other encrypted files (*)");
	d->setNameFilters(filters);

	d->setDefaultSuffix("");

	d->setViewMode(QFileDialog::Detail);
	return d;
}

void MainWindow::createTrayIcon() {
	trayIcon = new QSystemTrayIcon(QIcon(":/icon32.png"), this);
	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onTrayIconClick(QSystemTrayIcon::ActivationReason)));

	trayActionShowHide = new QAction(tr("Hide"), trayIcon);
	connect(trayActionShowHide, SIGNAL(triggered()), this, SLOT(onShowHideClick()));

	trayActionClose = new QAction(tr("Close Opened File"), trayIcon);
	connect(trayActionClose, SIGNAL(triggered()), this, SLOT(on_actionClose_triggered()));

	trayIconMenu = new QMenu(this);
	trayIconMenu->addAction(trayActionShowHide);
	trayIconMenu->addSeparator();
	trayIconMenu->addAction(trayActionClose);
	trayIconMenu->addAction(ui->actionExit);

	trayIcon->setContextMenu(trayIconMenu);

	trayIcon->show();
}

void MainWindow::createViewContextMenu() {
	viewContextMenu = new QMenu;
    viewContextMenu->addAction(ui->actionAddNode);
    viewContextMenu->addAction(ui->actionEditNode);
	viewContextMenu->addSeparator();

    viewContextMenu->addAction(ui->actionGenerateAndCopy2FA);

	viewContextCopyMenu = new QMenu(this);
	viewContextCopyMenu->setTitle(tr("Copy"));
	viewContextMenu->addMenu(viewContextCopyMenu);

	viewContextGotoMenu = new QMenu(this);
	viewContextGotoMenu->setTitle(tr("Go to"));
	viewContextMenu->addMenu(viewContextGotoMenu);
}

void MainWindow::displaySaveError(std::string filename) {
	QString m1 = tr("Error when saving file.");
	QString m2 = tr("The file \"%1\" has not been saved!").arg(QString::fromStdString(filename));

	QMessageBox box(QMessageBox::Warning, tr("Error"), m1, QMessageBox::Ok, this);
	box.setInformativeText(m2);
	box.exec();
}

std::vector<int> MainWindow::getSelectedIndexes() {
	std::vector<int> rows;
	for (QModelIndex const& i: ui->tableView->selectionModel()->selectedIndexes())
		rows.push_back(i.row());
	std::sort(rows.begin(), rows.end() );
	rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
	return rows;
}

void MainWindow::initUI() {
	if (Preferences::isSaveWindowDisposition()) {
		restoreGeometry(Preferences::getQSettings().value("WindowGeometry").toByteArray());
		restoreState(Preferences::getQSettings().value("WindowState").toByteArray());
	}

	createTrayIcon();

	QActionGroup* group = new QActionGroup(this);

	ui->actionDensityHigh->setActionGroup(group);
	ui->actionDensityMedium->setActionGroup(group);
	ui->actionDensityLow->setActionGroup(group);
	ui->actionDensityVeryLow->setActionGroup(group);

	ui->tableView->setAlternatingRowColors(true);
	ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableView->setGridStyle(Qt::PenStyle::SolidLine);
	ui->tableView->setSortingEnabled(true);
	ui->tableView->verticalHeader()->hide();
	ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	ui->tableView->horizontalHeader()->setHighlightSections(false);
	ui->tableView->setWordWrap(false);

	ui->noDataLabel->setText(
#ifdef Q_OS_ANDROID
				tr("This file is empty.\nAdd a new node using the toolbar.")
#else
				tr("This file is empty.\nAdd a new node using the toolbar or the menu item: Edit -> Add Node.")
#endif
	);

	ui->allDataFilteredOutLabel->setText(
				tr("There are no results matching the search criteria.")
	);

	ui->searchContainer->setVisible(false);

	connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex const&)), this, SLOT(onTableViewItemDoubleClicked(QModelIndex const&)));

	tableHeaderContextMenu = new QMenu(tr("Display Columns"), this);
	ui->tableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);     //set contextmenu
	connect(ui->tableView->horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint const&)),
			this, SLOT(onTableViewHeaderCustomContextMenu(QPoint const&)));

	createViewContextMenu();
	ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint const&)),
			this, SLOT(onTableViewCustomContextMenu(QPoint const&)));
}

Document* MainWindow::newEmptyDocument() {
	Document* document = new Document();

	document->addDefaultPropertyDefinitions();

	return document;
}

void MainWindow::selectNodeIds(std::vector<DocumentNodeId> v) {
	DataModel* d = static_cast<DataModel*>(ui->tableView->model());
	QItemSelection sel;

	std::vector<int> rows;

	for (DocumentNodeId const& n: v) {
		int row = d->getRowIndexFromNodeId(n);
		if (row < 0)
			continue;
		sel.append(QItemSelectionRange(d->getModelIndex(row, 0), d->getModelIndex(row, d->columnCount()-1)));
		rows.push_back(row);
	}
	ui->tableView->selectionModel()->select(sel, QItemSelectionModel::Clear | QItemSelectionModel::Select | QItemSelectionModel::Rows);
	if (!rows.empty()) {
		std::sort(rows.begin(), rows.end());

		auto r = ui->tableView->rect();
		size_t i = 0;
		int top = ui->tableView->indexAt(r.topLeft()).row();
		int bottom = ui->tableView->indexAt(r.bottomLeft()).row() - 1;
		if (bottom < top)
			bottom = std::numeric_limits<int>::max();

		if (rows[rows.size() - 1] - rows[0] < bottom - top &&
				rows[0] > top &&
				rows[rows.size() - 1] >= bottom)
				i = rows.size() - 1;

		ui->tableView->scrollTo(d->getModelIndex(rows[i], 0));
	}
}

void MainWindow::setFilename(std::string filename) {
	if (filename.empty())
		this->openedFile = "";
	else {
		QFileInfo q(QString::fromStdString(filename));
		this->openedFile = q.absoluteFilePath().toStdString();
	}
	updateWindowTitle();
}

void MainWindow::setModified(bool v) {
	if (v == this->isModified)
		return;

	if (!v)
		static_cast<DataModel*>(ui->tableView->model())->saved();

	this->isModified = v;
	updateWindowTitle();
}

void MainWindow::showEditNode(int rowIndex) {
	AddNewNodeDialog f(this, static_cast<DataModel*>(ui->tableView->model()), static_cast<DataModel*>(ui->tableView->model())->getNodeIdFromRowIndex(rowIndex));
	f.exec();
}

void MainWindow::showLongText(QModelIndex const& index) {
	QString hd = ui->tableView->model()->headerData(index.column(), Qt::Orientation::Horizontal).toString();
	QString val = ui->tableView->model()->data(index, Qt::UserRole).toString();

	LongTextDialog d(this, !Preferences::isDefaultOpenModeReadOnly(), hd, val);
	d.exec();

	if (d.isApplied())
		static_cast<DataModel*>(ui->tableView->model())->setData(index, d.getContents());
}

void MainWindow::updateCopyMenu() {
	ui->menuCopy->clear();
	viewContextCopyMenu->clear();

	DataModel* d = static_cast<DataModel*>(ui->tableView->model());

	int i = 0;

	for (DocumentPropertyDefinition def: d->getDocument()->getPropertyDefinitions()) {
		if (d->isSequenceColumn(def.name))
			continue;
		QString hd = QString::fromStdString(DataModel::formatHeader(def.name));
		QAction* a = new QAction(tr("Copy %1").arg(hd));
		a->setData(QString::fromStdString(def.name));
		if (i <= 9)
			a->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + (Qt::Key_0 + i++)));
		connect(a, SIGNAL(triggered(bool)), this, SLOT(onActionCopyClick()));

		ui->menuCopy->addAction(a);
		viewContextCopyMenu->addAction(a);
	}
}

void MainWindow::updateDisplayDensity() {
	int density = Preferences::getDisplayDensity();

	int h;
	switch (density) {
	case 0:
	default:
		if (!ui->actionDensityHigh->isChecked())
			ui->actionDensityHigh->setChecked(true);
		h = 16;
		break;
	case 1:if (!ui->actionDensityMedium->isChecked())
			ui->actionDensityMedium->setChecked(true);
		h = 22;
		break;
	case 2:if (!ui->actionDensityLow->isChecked())
			ui->actionDensityLow->setChecked(true);
		h = 28;
		break;
	case 3:if (!ui->actionDensityLow->isChecked())
			ui->actionDensityVeryLow->setChecked(true);
		h = 34;
		break;
	}
	ui->toolBar->setIconSize(QSize(h, h));
	ui->tableView->verticalHeader()->setDefaultSectionSize(h);
}

void MainWindow::updateGotoMenu() {
	ui->menuGoto->clear();
	viewContextGotoMenu->clear();

	if (ui->tableView->selectionModel()->selectedRows().count() != 1) {
		ui->menuGoto->setEnabled(false);
		viewContextGotoMenu->setEnabled(false);
		return;
	}

	DataModel* d = static_cast<DataModel*>(ui->tableView->model());

	DocumentNodeId nodeId = d->getNodeIdFromRowIndex(ui->tableView->selectionModel()->selectedRows().at(0).row());

	int i = 0;

	for (DocumentPropertyDefinition def: d->getDocument()->getPropertyDefinitions()) {
		if (d->isSequenceColumn(def.name))
			continue;
		if (def.type != DocumentPropertyType::DPT_URI)
			continue;

		QString uri = QString::fromStdString(d->getDocument()->getProperty(nodeId, def.name));
		if (uri.isEmpty())
			continue;
		if (!uri.contains("://"))
			uri = "http://" + uri;

		QAction* a = new QAction(uri);
		a->setData(uri);
		if (i <= 9)
			a->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::SHIFT + (Qt::Key_0 + i++)));
		connect(a, SIGNAL(triggered(bool)), this, SLOT(onActionGotoClick()));

		ui->menuGoto->addAction(a);
		viewContextGotoMenu->addAction(a);
	}

	ui->menuGoto->setEnabled(i != 0);
	viewContextGotoMenu->setEnabled(i != 0);
}

void MainWindow::updateHasItems() {
	bool hasDisplayedItems = ui->tableView->model()->rowCount() != 0;
	bool hasHiddenItems = static_cast<DataModel*>(ui->tableView->model())->getDocument()->getNodesCount() != 0;

	ui->tableView->setVisible(hasDisplayedItems);

	ui->noDataLabel->setVisible(!hasHiddenItems);
	ui->allDataFilteredOutLabel->setVisible(!hasDisplayedItems && hasHiddenItems);

	if (hasDisplayedItems && ui->tableView->selectionModel()->selectedRows().size() == 0)
		QTimer::singleShot(0, this, SLOT(onSelectRowRequested()));
}

void MainWindow::updateUI() {
	ui->toolBar->setVisible(Preferences::isShowToolBar());
	ui->actionShowToolbar->setChecked(Preferences::isShowToolBar());

	updateHasItems();

	ui->actionReadOnly->setChecked(Preferences::isDefaultOpenModeReadOnly());
	ui->tableView->setEditTriggers(Preferences::isDefaultOpenModeReadOnly() ? QAbstractItemView::NoEditTriggers : QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

	ui->actionAutosave->setChecked(Preferences::isAutoSave());
	ui->actionObscurePasswords->setChecked(Preferences::isObscurePasswords());

	updateDisplayDensity();

	bool const _readOnly = ui->actionReadOnly->isChecked();

	bool const _hasOpenedFile = hasOpenFile();
	bool const _hasUnsavedModifications = hasUnsavedModifications();

	ui->actionReload->setEnabled(_hasOpenedFile);
	ui->actionSave->setEnabled(_hasUnsavedModifications);
	ui->actionSaveAs->setEnabled(_hasOpenedFile || _hasUnsavedModifications);
	ui->actionSaveCopy->setEnabled(_hasOpenedFile || _hasUnsavedModifications);
	ui->actionProperties->setEnabled(_hasOpenedFile);
	ui->actionClose->setEnabled(_hasOpenedFile || _hasUnsavedModifications);
	trayActionClose->setVisible(_hasOpenedFile || _hasUnsavedModifications);
	ui->actionAutosave->setEnabled(_hasOpenedFile);
	ui->actionSearch->setEnabled(true);

	bool const _hasSelectedItems = ui->tableView->selectionModel()->hasSelection();
	ui->actionAddNode->setEnabled(!_readOnly);
	ui->actionEditNode->setEnabled(!_readOnly && (ui->tableView->selectionModel()->selectedRows().size() == 1));
    ui->actionGenerateAndCopy2FA->setEnabled(!get2FAInformation().isEmpty());
	ui->actionDeleteNode->setEnabled(!_readOnly && _hasSelectedItems);
	ui->actionCreateProperty->setEnabled(!_readOnly);
	ui->actionRemoveProperty->setEnabled(!_readOnly);
	ui->menuMoveNode->setEnabled(!_readOnly && _hasSelectedItems);
	ui->actionMoveUp->setEnabled(!_readOnly && _hasSelectedItems);
	ui->actionMoveDown->setEnabled(!_readOnly && _hasSelectedItems);
	ui->actionMoveTop->setEnabled(!_readOnly && _hasSelectedItems);
	ui->actionMoveBottom->setEnabled(!_readOnly && _hasSelectedItems);
	ui->menuDisplayColumns->setEnabled(!_readOnly);
	tableHeaderContextMenu->setEnabled(!_readOnly);
	ui->menuCopy->setEnabled(ui->tableView->selectionModel()->selectedRows().size() == 1);
	viewContextCopyMenu->setEnabled(ui->tableView->selectionModel()->selectedRows().size() == 1);

	updateGotoMenu();

	std::vector<std::string> recentFiles = Preferences::getRecentFiles();
	ui->menuOpenRecent->setEnabled(!recentFiles.empty());
	ui->actionClearRecent->setVisible(!recentFiles.empty());

	for (auto a: ui->menuOpenRecent->actions())
		if (a != ui->actionClearRecent)
			ui->menuOpenRecent->removeAction(a);

	for (std::string s: recentFiles) {
		QString h = QString::fromStdString(s);

		QAction* act = new QAction(ui->menuOpenRecent);
		act->setText(h);
		act->setData(QVariant(h));
		connect(act, SIGNAL(triggered()), this, SLOT(onOpenRecentClick()));

		ui->menuOpenRecent->insertAction(ui->actionClearRecent, act);
	}
	ui->menuOpenRecent->insertSeparator(ui->actionClearRecent);
}

void MainWindow::updateWindowTitle() {
	QString a;
	if (this->openedFile.empty())
		a = "[New File]";
	else {
		QFileInfo q(QString::fromStdString(this->openedFile));
		a = q.fileName();
	}
	this->setWindowTitle(a + (this->hasUnsavedModifications() ? QString("*") : QString()) +
						 " - " + QString::fromStdString(passcave_APPLICATION_NAME));
}

void MainWindow::onActionCopyClick() {
	if (ui->tableView->selectionModel()->selectedRows().size() != 1)
		return;

	QAction* a = dynamic_cast<QAction*>(QObject::sender());

	DataModel* d = static_cast<DataModel*>(ui->tableView->model());
	DocumentNodeId nodeId = d->getNodeIdFromRowIndex(ui->tableView->selectionModel()->selectedRows().first().row());

	QString val = QString::fromStdString(d->getDocument()->getProperty(nodeId, a->data().toString().toStdString()));
	QApplication::clipboard()->setText(val);
}

void MainWindow::onActionGotoClick() {
	QAction* a = dynamic_cast<QAction*>(QObject::sender());
	QString uri = a->data().toString();

	QDesktopServices::openUrl(QUrl(uri));
}

void MainWindow::onDataChanged(QModelIndex const&, QModelIndex const&, QVector<int> const&) {
	updateHasItems();
	setModified(true);
	//TODO should also update ui (see minor bug in readme)
}

void MainWindow::onDisplayColumnClick() {
	for (auto a: tableHeaderContextMenu->actions()) {
		if (a->isChecked())
			ui->tableView->showColumn(a->data().toInt());
		else
			ui->tableView->hideColumn(a->data().toInt());
		static_cast<DataModel*>(ui->tableView->model())->setColumnHidden(a->data().toInt(), !a->isChecked());
	}
	updateColumnsWidth();
}

void MainWindow::onLayoutAboutToBeChanged() {
	lastSelectedIndexes.clear();
	for (int i: getSelectedIndexes())
		lastSelectedIndexes.push_back(static_cast<DataModel*>(ui->tableView->model())->getNodeIdFromRowIndex(i));
}

void MainWindow::onLayoutChanged() {
	ui->tableView->selectionModel()->clearSelection();

	bool scrolled = false;
	for (DocumentNodeId id: lastSelectedIndexes) {
		int r = static_cast<DataModel*>(ui->tableView->model())->getRowIndexFromNodeId(id);
		QModelIndex index = static_cast<DataModel*>(ui->tableView->model())->getModelIndex(r,0);
		ui->tableView->selectionModel()->select(
					index,
					QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);

		if (!scrolled) {
			ui->tableView->scrollTo(index);
			scrolled = true;
		}
	}

	lastSelectedIndexes.clear();
}

void MainWindow::onOpenMostRecentFile() {
	if (QApplication::arguments().length() > 1) {
		QString s = QApplication::arguments().at(1);
		QFile file(s);
		if (file.exists()) {
			openFile(s.toStdString());
			return;
		}
	}
	if (Preferences::isAutoOpen() && !Preferences::getMostRecentFile().empty()) {
		openMostRecentFile();
	}
}

void MainWindow::onOpenRecentClick() {
	QAction* act = dynamic_cast<QAction*>(sender());

	if (!confirmClose())
		return;

	openFile(act->data().toString().toStdString());
}

void MainWindow::onSelectionChanged(QItemSelection const&, QItemSelection const&) {
	updateUI();
}

void MainWindow::onSelectRowRequested() {
	if (ui->tableView->model()->rowCount() != 0 && ui->tableView->selectionModel()->selectedRows().size() == 0)
		ui->tableView->selectRow(0);
}

void MainWindow::onShowHideClick() {
	if (isVisible()) {
		bool canHide = true;
		if (QApplication::activeModalWidget() != nullptr) {
			canHide = false;
		} else {
			if (hasUnsavedModifications() && Preferences::isAutoSaveOnMinimizeToTray()) {
				saveOrSaveAs();
				if (hasUnsavedModifications())
					canHide = false;
			}
			if (Preferences::isAutoCloseOnMinimizeToTray()) {
				if (!closeFile())
					canHide = false;
			}
		}
		if (!canHide) {
			show();
			activateWindow();
			raise();
			return;
		}
		hide();
		trayActionShowHide->setText(tr("Show"));
	} else {
		if (!hasOpenFile() && !hasUnsavedModifications() && Preferences::isAutoOpenOnRestoreFromTray())
			openMostRecentFile();
		show();
		activateWindow();
		raise();
		trayActionShowHide->setText(tr("Hide"));
	}
}

void MainWindow::onSortMenuClick() {
	QAction* act = dynamic_cast<QAction*>(QObject::sender());
	int const col = act->data().toInt();

	Qt::SortOrder sort =  ui->tableView->horizontalHeader()->sortIndicatorSection() != col ? Qt::SortOrder::AscendingOrder : (
							ui->tableView->horizontalHeader()->sortIndicatorOrder() == Qt::SortOrder::AscendingOrder ?
								Qt::SortOrder::DescendingOrder : Qt::SortOrder::AscendingOrder
						 );

	ui->tableView->sortByColumn(col, sort);
	ui->tableView->horizontalHeader()->setSortIndicator(col, sort);
}

void MainWindow::onTableViewCustomContextMenu(QPoint const& p) {
	viewContextMenu->exec(ui->tableView->viewport()->mapToGlobal(p));
}

void MainWindow::onTableViewHeaderCustomContextMenu(QPoint const& p) {
	tableHeaderContextMenu->exec(ui->tableView->horizontalHeader()->viewport()->mapToGlobal(p));
}

void MainWindow::onTableViewItemDoubleClicked(QModelIndex const& index) {
	DocumentPropertyType type = static_cast<DataModel*>(ui->tableView->model())->headerType(index.column());

	switch (type) {
	case DocumentPropertyType::DPT_LONGTEXT:
		showLongText(index);
		break;
	case DocumentPropertyType::DPT_TEXTARRAY:
		showEditNode(index.row());
		break;
	default:
		if (static_cast<DataModel*>(ui->tableView->model())->isSequenceColumn(index.column()))
			showEditNode(index.row());
	}
}

void MainWindow::onTrayIconClick(QSystemTrayIcon::ActivationReason v) {
	if (v == QSystemTrayIcon::ActivationReason::Trigger)
		onShowHideClick();
}

void MainWindow::on_actionAbout_triggered() {
	AboutDialog aboutDialog(this);
	aboutDialog.exec();
}

void MainWindow::on_actionAddNode_triggered() {
	AddNewNodeDialog f(this, static_cast<DataModel*>(ui->tableView->model()));
	f.exec();

	// If we are filtering data, then we disable the filter to ensure added node is displayed
	if (ui->actionSearch->isChecked()) {
		ui->actionSearch->setChecked(false);
		on_actionSearch_triggered();
	}

	// ensure we select the added node
	selectNodeIds({f.getNodeId()});
}

void MainWindow::on_actionAutosave_triggered() {
	Preferences::setAutoSave(ui->actionAutosave->isChecked());
}

void MainWindow::on_actionClearRecent_triggered() {
	Preferences::clearRecentFiles();
	if (hasOpenFile())
		Preferences::addRecentFile(this->openedFile);
	updateUI();
}

void MainWindow::on_actionClose_triggered() {
	openNewFile();
}

void MainWindow::on_actionDeleteNode_triggered() {
	std::vector<int> indexes = getSelectedIndexes();

	QString m1 = tr("Are you sure you want to delete the (%n) selected nodes?", "", static_cast<int>(indexes.size()));

	QMessageBox box(QMessageBox::Question, tr("Confirmation"), m1, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
	box.setDefaultButton(QMessageBox::Cancel);
	int reply = box.exec();
	if (reply != QMessageBox::Yes)
		return;

	// Which node to select next?
	DocumentNodeId toSelect = InvalidDocumentNodeId;
	if (indexes.size()) {
		int low = *std::min_element(indexes.begin(), indexes.end());
		if (low > 0)
			toSelect = static_cast<DataModel*>(ui->tableView->model())->getNodeIdFromRowIndex(low - 1);
	}

	static_cast<DataModel*>(ui->tableView->model())->deleteIndexes(indexes);

	//TODO: The following temporary fix, disables searching when deleting a node. (Deleting items doesn't work when searching)
	if (ui->actionSearch->isChecked()) {
		ui->actionSearch->setChecked(false);
		on_actionSearch_triggered();
	}

	// Select a node
	if (toSelect != InvalidDocumentNodeId)
		selectNodeIds({toSelect});
}

void MainWindow::on_actionDensityHigh_triggered() {
	Preferences::setDisplayDensity(0);
	updateDisplayDensity();
}

void MainWindow::on_actionDensityLow_triggered() {
	Preferences::setDisplayDensity(2);
	updateDisplayDensity();
}

void MainWindow::on_actionDensityMedium_triggered() {
	Preferences::setDisplayDensity(1);
	updateDisplayDensity();
}

void MainWindow::on_actionDensityVeryLow_triggered() {
	Preferences::setDisplayDensity(3);
	updateDisplayDensity();
}

void MainWindow::on_actionEditNode_triggered() {
	auto v = getSelectedIndexes();
	if (v.size() != 1)
		return;
	showEditNode(v.back());
}

void MainWindow::on_actionExit_triggered() {
	close();
}

void MainWindow::on_actionMoveBottom_triggered() {
#ifdef FIX_SORT_QT_BUG
	if (ui->tableView->horizontalHeader()->sortIndicatorOrder() != Qt::SortOrder::AscendingOrder || ui->tableView->horizontalHeader()->sortIndicatorSection() != 0) {
		QString m1 = tr("Please, sort with id (#) ascending first (FIX_SORT_QT_BUG)");
		QMessageBox box(QMessageBox::Warning, tr("Error"), m1, QMessageBox::Ok, this);
		box.setDefaultButton(QMessageBox::Ok);
		box.exec();
		return;
	}
#endif

	auto v = static_cast<DataModel*>(ui->tableView->model())->moveDataBottom(getSelectedIndexes());
#ifdef FIX_SORT_QT_BUG
	static_cast<DataModel*>(ui->tableView->model())->sort(0, Qt::AscendingOrder);
#endif
	ui->tableView->sortByColumn(0, Qt::AscendingOrder);
	MainWindow::selectNodeIds(v);
}

void MainWindow::on_actionMoveDown_triggered() {
#ifdef FIX_SORT_QT_BUG
	if (ui->tableView->horizontalHeader()->sortIndicatorOrder() != Qt::SortOrder::AscendingOrder || ui->tableView->horizontalHeader()->sortIndicatorSection() != 0) {
		QString m1 = tr("Please, sort with id (#) ascending first (FIX_SORT_QT_BUG)");
		QMessageBox box(QMessageBox::Warning, tr("Error"), m1, QMessageBox::Ok, this);
		box.setDefaultButton(QMessageBox::Ok);
		box.exec();
		return;
	}
#endif

	auto v = static_cast<DataModel*>(ui->tableView->model())->moveData(getSelectedIndexes(), 1);
#ifdef FIX_SORT_QT_BUG
	static_cast<DataModel*>(ui->tableView->model())->sort(0, Qt::AscendingOrder);
#endif
	ui->tableView->sortByColumn(0, Qt::AscendingOrder);
	MainWindow::selectNodeIds(v);
}

void MainWindow::on_actionMoveTop_triggered() {
#ifdef FIX_SORT_QT_BUG
	if (ui->tableView->horizontalHeader()->sortIndicatorOrder() != Qt::SortOrder::AscendingOrder || ui->tableView->horizontalHeader()->sortIndicatorSection() != 0) {
		QString m1 = tr("Please, sort with id (#) ascending first (FIX_SORT_QT_BUG)");
		QMessageBox box(QMessageBox::Warning, tr("Error"), m1, QMessageBox::Ok, this);
		box.setDefaultButton(QMessageBox::Ok);
		box.exec();
		return;
	}
#endif

	auto v = static_cast<DataModel*>(ui->tableView->model())->moveDataTop(getSelectedIndexes());
#ifdef FIX_SORT_QT_BUG
	static_cast<DataModel*>(ui->tableView->model())->sort(0, Qt::AscendingOrder);
#endif
	ui->tableView->sortByColumn(0, Qt::AscendingOrder);
	MainWindow::selectNodeIds(v);
}

void MainWindow::on_actionMoveUp_triggered() {
#ifdef FIX_SORT_QT_BUG
	if (ui->tableView->horizontalHeader()->sortIndicatorOrder() != Qt::SortOrder::AscendingOrder || ui->tableView->horizontalHeader()->sortIndicatorSection() != 0) {
		QString m1 = tr("Please, sort with id (#) ascending first (FIX_SORT_QT_BUG)");
		QMessageBox box(QMessageBox::Warning, tr("Error"), m1, QMessageBox::Ok, this);
		box.setDefaultButton(QMessageBox::Ok);
		box.exec();
		return;
	}
#endif

	auto v = static_cast<DataModel*>(ui->tableView->model())->moveData(getSelectedIndexes(), -1);
#ifdef FIX_SORT_QT_BUG
	static_cast<DataModel*>(ui->tableView->model())->sort(0, Qt::AscendingOrder);
#endif
	ui->tableView->sortByColumn(0, Qt::AscendingOrder);
	MainWindow::selectNodeIds(v);
}

void MainWindow::on_actionNew_triggered() {
	openNewFile();
}

void MainWindow::on_actionObscurePasswords_triggered() {
	Preferences::setObscurePasswords(ui->actionObscurePasswords->isChecked());
	static_cast<DataModel*>(ui->tableView->model())->reset();
}

void MainWindow::on_actionOpen_triggered() {
	QFileDialog* d = createFileDialog(QFileDialog::AcceptOpen);
	d->setWindowTitle(tr("Open file"));
	if (!confirmClose()) {
		delete d;
		return;
	}
	if (d->exec()) {
		QStringList l = d->selectedFiles();
		if (l.size() == 1)
			openFile(l[0].toStdString());
	}
	delete d;
}

void MainWindow::on_actionPreferences_triggered() {
	PreferencesDialog d(this);
	d.exec();
	updateUI();
	if (d.needsReopen())
		QTimer::singleShot(0, this, SLOT(on_actionPreferences_triggered()));
}

void MainWindow::on_actionProperties_triggered() {
	DataModel* d = static_cast<DataModel*>(ui->tableView->model());
	PropertiesDialog p(this,
					   this->openedFile,
					   this->openedFileInfo,
					   d->getDocument()->getNodesCount(),
					   d->getDocument()->getPropertyDefinitionsCount());
	p.exec();
}


void MainWindow::on_actionReadOnly_triggered() {
	Preferences::setDefaultOpenModeReadOnly(ui->actionReadOnly->isChecked());
	updateUI();
}

void MainWindow::on_actionReload_triggered() {
	std::string f = this->openedFile;
	if (f.empty())
		return;

	bool oldModified = this->isModified;
	if (hasUnsavedModifications()) {
		QString m1 = tr("There are unsaved modifications to \"%1\".").arg(QString::fromStdString(this->openedFile));
		QString m2 = tr("If you continue, you will lose those modifications.\n"
						"Are you sure you want to continue?");

		QMessageBox box(QMessageBox::Question, tr("Question"), m1, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
		box.setDefaultButton(QMessageBox::Cancel);
		box.setInformativeText(m2);
		int reply = box.exec();
		if (reply != QMessageBox::Yes)
			return;

		this->isModified = false;
	}

	if (!openFile(this->openedFile, this->openedFileClearPass)) {
		if (!openFile(this->openedFile))
			this->isModified = oldModified;
	}
}

void MainWindow::on_actionSave_triggered() {
	saveOrSaveAs();
}

void MainWindow::on_actionSaveCopy_triggered() {
	saveCopyAs();
}

void MainWindow::on_actionSaveAs_triggered() {
	saveAs();
}

void MainWindow::on_actionSearch_triggered() {
	ui->searchContainer->setVisible(ui->actionSearch->isChecked());
	ui->lineEdit_search->blockSignals(true);
	ui->comboBox_search->blockSignals(true);
	ui->checkBox_search->blockSignals(true);
	if (ui->actionSearch->isChecked())
		ui->lineEdit_search->setFocus();
	else
		ui->lineEdit_search->setText("");
	ui->comboBox_search->setCurrentIndex(0);
	ui->checkBox_search->setChecked(false);
	ui->checkBox_search->blockSignals(false);
	ui->comboBox_search->blockSignals(false);
	ui->lineEdit_search->blockSignals(false);
	updateFilter();
}

void MainWindow::on_actionShowToolbar_triggered() {
	Preferences::setShowToolBar(ui->actionShowToolbar->isChecked());
	updateUI();
}

void MainWindow::on_checkBox_search_toggled(bool) {
	updateFilter();
}

void MainWindow::on_comboBox_search_currentIndexChanged(int) {
	updateFilter();
}

void MainWindow::on_lineEdit_search_textEdited(QString const&) {
	updateFilter();
}

void MainWindow::on_toolButton_clicked() {
	ui->actionSearch->setChecked(false);
	on_actionSearch_triggered();
}

void MainWindow::on_actionAddMissingProperties_triggered() {
    static_cast<DataModel*>(ui->tableView->model())->addMissingDocumentProperties();
}


void MainWindow::on_actionGenerateAndCopy2FA_triggered() {
    auto info = get2FAInformation();
    if (info.isEmpty())
        return;
    QApplication::clipboard()->setText(generateOTP(parse2FAUri(info)));
}

