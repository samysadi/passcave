#pragma once

#include "gcry.h"
#include "document.h"
#include "utils2fa.h"

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QEvent>
#include <QFileDialog>
#include <QCloseEvent>
#include <QItemSelection>
#include <QWidget>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = 0);
	~MainWindow();
	bool closeFile(bool exiting = false, bool spontaneousClose = false);
	bool closeFileImmediately();
	bool confirmClose(bool exiting = false, bool spontaneousClose = false);
	bool confirmReplace(std::string filename);
    QString get2FAInformation() const;
	bool hasOpenFile();
	bool hasUnsavedModifications();
	void loadDocumentToUi(Document* document);
	bool openFile(std::string filename);
	bool openFile(std::string filename, std::string clearPassword);
	void openMostRecentFile();
	void openNewFile();
	std::string requestPassword(QString label, bool* ok);
	void saveAs(bool asACopy = false);
	void saveCopyAs();
	bool saveFile(std::string filename, std::string clearPassword, passcave::gcrypt_FileInfo fileInfo, bool asACopy = false);
	void saveOrSaveAs();
	void updateColumnsChanged();
	void updateColumnsWidth();
	void updateFilter();
protected:
	void changeEvent(QEvent* e);
	void closeEvent(QCloseEvent* e);
	bool event(QEvent* e);
	void hideEvent(QHideEvent* event);
	void resizeEvent(QResizeEvent* event);
	void showEvent(QShowEvent* event);
private:
	Ui::MainWindow* ui;
	QSystemTrayIcon* trayIcon;
	QMenu* trayIconMenu;
	QAction* trayActionShowHide;
	QAction* trayActionClose;

	QMenu* viewContextMenu;
	QMenu* viewContextCopyMenu;
	QMenu* viewContextGotoMenu;

	QMenu* tableHeaderContextMenu;
	bool isWindowJustOpened;

	std::vector<DocumentNodeId> lastSelectedIndexes;

	bool isModified = false;
	std::string openedFile;
	passcave::gcrypt_FileInfo openedFileInfo;
	std::string openedFileClearPass;

	void addToRecentFiles(std::string filename);
	void clearRecentFiles();
	QFileDialog* createFileDialog(QFileDialog::AcceptMode mode);
	void createTrayIcon();
	void createViewContextMenu();
	void displaySaveError(std::string filename);
	std::vector<int> getSelectedIndexes();
	void initUI();
	Document* newEmptyDocument();
	void selectNodeIds(std::vector<DocumentNodeId> v);
	void setFilename(std::string filename);
	void setModified(bool v);
	void showEditNode(int rowIndex);
	void showLongText(QModelIndex const& index);
	void updateCopyMenu();
	void updateDisplayDensity();
	void updateGotoMenu();
	void updateHasItems();
	void updateUI();
	void updateWindowTitle();
private slots:
	void onActionCopyClick();
	void onActionGotoClick();
	void onDataChanged(QModelIndex const& topLeft, QModelIndex const& bottomRight, QVector<int> const& roles = QVector<int> ());
	void onDisplayColumnClick();
	void onLayoutAboutToBeChanged();
	void onLayoutChanged();
	void onOpenMostRecentFile();
	void onOpenRecentClick();
	void onSelectionChanged(QItemSelection const& a, QItemSelection const& b);
	void onSelectRowRequested();
	void onShowHideClick();
	void onSortMenuClick();
	void onTableViewCustomContextMenu(QPoint const& p);
	void onTableViewHeaderCustomContextMenu(QPoint const& p);
	void onTableViewItemDoubleClicked(QModelIndex const& index);
	void onTrayIconClick(QSystemTrayIcon::ActivationReason v);

	void on_actionAbout_triggered();
	void on_actionAddNode_triggered();
	void on_actionAutosave_triggered();
	void on_actionClearRecent_triggered();
	void on_actionClose_triggered();
	void on_actionDeleteNode_triggered();
	void on_actionDensityHigh_triggered();
	void on_actionDensityLow_triggered();
	void on_actionDensityMedium_triggered();
	void on_actionDensityVeryLow_triggered();
	void on_actionEditNode_triggered();
	void on_actionExit_triggered();
	void on_actionMoveBottom_triggered();
	void on_actionMoveDown_triggered();
	void on_actionMoveTop_triggered();
	void on_actionMoveUp_triggered();
	void on_actionNew_triggered();
	void on_actionObscurePasswords_triggered();
	void on_actionOpen_triggered();
	void on_actionPreferences_triggered();
	void on_actionProperties_triggered();
	void on_actionReadOnly_triggered();
	void on_actionReload_triggered();
	void on_actionSave_triggered();
	void on_actionSaveAs_triggered();
	void on_actionSaveCopy_triggered();
	void on_actionSearch_triggered();
	void on_actionShowToolbar_triggered();
	void on_checkBox_search_toggled(bool checked);
	void on_comboBox_search_currentIndexChanged(int index);
	void on_lineEdit_search_textEdited(QString const& arg1);
	void on_toolButton_clicked();
    void on_actionAddMissingProperties_triggered();
    void on_actionGenerateAndCopy2FA_triggered();
};
