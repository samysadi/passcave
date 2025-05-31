#pragma once

#include <QDialog>
#include <QLayout>
#include <QTabWidget>

#include <unordered_map>

typedef void (PreferencesDialogBoolSetter)(bool);
typedef void (PreferencesDialogIntSetter)(int);

class PreferencesDialog : public QDialog {
	Q_OBJECT

public:
	PreferencesDialog(QWidget* parent);
	~PreferencesDialog();
	bool needsReopen();

private:
	bool reopen = false;
	QTabWidget* tabWidget;
	QWidget* currentContainer;
	std::unordered_map<QWidget*, PreferencesDialogBoolSetter*> boolSetters;
	void addBoolean(QString label, bool (*getter)(), PreferencesDialogBoolSetter* setter);
	std::unordered_map<QWidget*, PreferencesDialogIntSetter*> intSetters;
	void addInt(QString label, int (*getter)(), PreferencesDialogIntSetter* setter, int min, int max);
	void addHSpacer();
	void addLanguageComboBox(QString label);
	void addPage(QString title);
	void addVSpacer();
	void initUI();

private slots:
	void onBooleanChange();
	void onCloseClicked();
	void onIntChange();
	void onLanguageChange();
};
