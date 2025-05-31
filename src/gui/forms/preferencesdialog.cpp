#include "preferencesdialog.h"

#include "config.h"

#include "includes/preferences.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(QWidget* parent) :
	QDialog(parent) {

	initUI();
	
}

PreferencesDialog::~PreferencesDialog() {
	
}

bool PreferencesDialog::needsReopen() {
	return this->reopen;
}

int const FORM_WIDTH = 500;
int const SPACING = 4;

#define ADD_BOOLEAN(label, T) addBoolean(label, &Preferences::is ## T, &Preferences::set ## T)
#define ADD_INT(label, T, min, max) addInt(label, &Preferences::get ## T, &Preferences::set ## T, min, max)

void PreferencesDialog::addBoolean(QString label, bool (*getter)(), PreferencesDialogBoolSetter* setter) {
	QCheckBox* w = new QCheckBox(label, this);
	this->currentContainer->layout()->addWidget(w);
	w->setChecked((*getter)());

	boolSetters[w] = setter;

	connect(w, SIGNAL(toggled(bool)), this, SLOT(onBooleanChange()));
}

void PreferencesDialog::addInt(QString label, int (*getter)(), PreferencesDialogIntSetter* setter, int min, int max) {
	QWidget* wLayout = new QWidget(this);
	QHBoxLayout* layout = new QHBoxLayout(wLayout);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(SPACING);
	wLayout->setLayout(layout);
	this->currentContainer->layout()->addWidget(wLayout);

	{
		QLabel* w = new QLabel(label, this);
		w->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
		w->setWordWrap(true);
		layout->addWidget(w);
	}

	{
		QSpinBox* w = new QSpinBox(this);
		w->setMinimum(min);
		w->setMaximum(max);
		w->setValue((*getter)());
		layout->addWidget(w);

		intSetters[w] = setter;

		connect(w, SIGNAL(valueChanged(int)), this, SLOT(onIntChange()));
	}
}

void PreferencesDialog::addHSpacer() {
	QWidget* widget = new QWidget(this);
	widget->setFixedHeight(1);
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	this->currentContainer->layout()->addWidget(widget);
}

void PreferencesDialog::addLanguageComboBox(QString label) {
	QWidget* wLayout = new QWidget(this);
	QHBoxLayout* layout = new QHBoxLayout(wLayout);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(SPACING);
	wLayout->setLayout(layout);
	this->currentContainer->layout()->addWidget(wLayout);

	{
		QLabel* w = new QLabel(label, this);
		w->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
		w->setWordWrap(true);
		layout->addWidget(w);
	}

	{
		QString currentLanguage = Preferences::getLanguage();

		QComboBox* w = new QComboBox(this);
		w->addItem("Default", QVariant(""));
		w->setCurrentIndex(0);
		for (const QString &f: QDir(":/langs").entryList()) {
			QString ff = f.left(f.indexOf("."));
			ff[0] = ff[0].toUpper();
			w->addItem(ff, f);
			if (f == currentLanguage)
				w->setCurrentIndex(w->count() - 1);
		}
		layout->addWidget(w);

		connect(w, SIGNAL(currentIndexChanged(int)), this, SLOT(onLanguageChange()));
	}
}

void PreferencesDialog::addPage(QString title) {
	QWidget* widget = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(widget);
	this->currentContainer = this;
	layout->setSizeConstraint(QLayout::SetDefaultConstraint);
	layout->setContentsMargins(SPACING, SPACING, SPACING, SPACING);
	layout->setSpacing(SPACING);
	widget->setLayout(layout);

	tabWidget->addTab(widget, title);
	this->currentContainer = widget;
}

void PreferencesDialog::addVSpacer() {
	QWidget* widget = new QWidget(this);
	widget->setFixedWidth(1);
	widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	this->currentContainer->layout()->addWidget(widget);
}

void PreferencesDialog::initUI() {
	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	this->setModal(true);
	this->setWindowTitle(tr("Preferences"));

	{
		QVBoxLayout* layout = new QVBoxLayout(this);
		this->currentContainer = this;
		layout->setSizeConstraint(QLayout::SetDefaultConstraint);
		layout->setContentsMargins(SPACING, SPACING, SPACING, SPACING);
		layout->setSpacing(SPACING);
		setLayout(layout);

		tabWidget = new QTabWidget(this);
		tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		this->layout()->addWidget(tabWidget);
	}

	addPage(tr("General preferences"));
	addLanguageComboBox(tr("Language"));
	ADD_BOOLEAN(tr("Always ask before closing %1")
					.arg(QString::fromStdString(passcave_APPLICATION_NAME)),
				ConfirmBeforeExit);
	ADD_BOOLEAN(tr("Ask before closing %1 when not closed from the menu")
				.arg(QString::fromStdString(passcave_APPLICATION_NAME)),
				ConfirmBeforeSpontaneousExit);
	ADD_BOOLEAN(tr("Automatically open most recent file when launching %1")
					.arg(QString::fromStdString(passcave_APPLICATION_NAME)),
				AutoOpen);
	ADD_BOOLEAN(tr("Automatically save files before closing them"),
				AutoSave);
	ADD_BOOLEAN(tr("Hide passwords in the main interface"),
				ObscurePasswords);
	ADD_INT(tr("Maximum number of items to keep under the recent files menu"),
			MaximumNumberOfRecentFiles,
			0, 99);
	ADD_BOOLEAN(tr("Open files in read-only mode"),
				DefaultOpenModeReadOnly);
	ADD_BOOLEAN(tr("Save / Restore window disposition when exiting / launching %1")
				.arg(QString::fromStdString(passcave_APPLICATION_NAME)),
				SaveWindowDisposition);
	ADD_BOOLEAN(tr("Show a toolbar under the menu"),
				ShowToolBar);

	addPage(tr("Tray related preferences"));
	ADD_BOOLEAN(tr("Always close open file when minimizing to tray"),
				AutoCloseOnMinimizeToTray);
	ADD_BOOLEAN(tr("Always open most recent file when restoring from tray"),
				AutoOpenOnRestoreFromTray);
	ADD_BOOLEAN(tr("Automatically save current file when minimizing to tray"),
				AutoSaveOnMinimizeToTray);
	ADD_BOOLEAN(tr("Minimize to tray, instead of closing %1, when not using the menu")
				.arg(QString::fromStdString(passcave_APPLICATION_NAME)),
				MinimizeToTrayOnSpontaneousExit);
	addVSpacer();

	{

		QWidget* wLayout2 = new QWidget(this);
		QHBoxLayout* layout2 = new QHBoxLayout(wLayout2);
		wLayout2->setLayout(layout2);
		this->layout()->addWidget(wLayout2);
		this->currentContainer = wLayout2;

		addHSpacer();

		QPushButton* b2 = new QPushButton(tr("Close"), this);
		layout2->addWidget(b2);
		b2->setDefault(true);
		connect(b2, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
	}

	this->setFixedSize(FORM_WIDTH, this->sizeHint().height());
}

void PreferencesDialog::onBooleanChange() {
	QCheckBox* w = dynamic_cast<QCheckBox*>(QObject::sender());
	auto it = boolSetters.find(w);
	if (it == boolSetters.end())
		return;
	PreferencesDialogBoolSetter* setter = it->second;
	(*setter)(w->isChecked());
}

void PreferencesDialog::onCloseClicked() {
	this->close();
}

void PreferencesDialog::onIntChange() {
	QSpinBox* w = dynamic_cast<QSpinBox*>(QObject::sender());
	auto it = intSetters.find(w);
	if (it == intSetters.end())
		return;
	PreferencesDialogIntSetter* setter = it->second;
	(*setter)(w->value());

}

void PreferencesDialog::onLanguageChange() {
	QComboBox* w = dynamic_cast<QComboBox*>(QObject::sender());
	if (w->currentIndex() < 0)
		return;
	Preferences::setLanguage(w->currentData().toString());

	QString m1 = tr("Language changed.");
	QString m2 = tr("However, some changes can be applied only after the application is restarted.");

	QMessageBox box(QMessageBox::Question, tr("Information"), m1, QMessageBox::Ok, this);
	box.setDefaultButton(QMessageBox::Cancel);
	box.setInformativeText(m2);
	box.exec();

	this->reopen = true;
	this->close();
}
