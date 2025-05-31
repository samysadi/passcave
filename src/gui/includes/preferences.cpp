#include "preferences.h"
#include "config.h"

#include <QApplication>
#include <QFile>
#include <QSettings>

QSettings* Preferences::qSettings = NULL;

Preferences::Preferences() {
	//
}

QSettings& Preferences::getQSettings() {
	if (qSettings == NULL)
		qSettings = new QSettings(QString::fromStdString(passcave_ORGANIZATION), QString::fromStdString(passcave_APPLICATION_NAME));
	return *qSettings;
}

void Preferences::addRecentFile(std::string s) {
	std::vector<std::string> v = getRecentFiles();
	for(auto it = v.begin(); it != v.end(); ++it) {
		if (s.compare(it->c_str()) == 0) {
			v.erase(it);
			break;
		}
	}
	v.insert(v.begin(), s);
	setRecentFiles(v);
}

void Preferences::clearRecentFiles() {
	std::vector<std::string> v;
	setRecentFiles(v);
}

void Preferences::removeRecentFile(std::string s) {
	std::vector<std::string> v = getRecentFiles();
	for(auto it = v.begin(); it != v.end(); ++it) {
		if (s.compare(it->c_str()) == 0) {
			v.erase(it);
			break;
		}
	}
	setRecentFiles(v);
}

bool Preferences::isAutoCloseOnMinimizeToTray() {
	return getQSettings().value("AutoCloseOnMinimizeToTray", QVariant(false)).toBool();
}

bool Preferences::isAutoOpen() {
	return getQSettings().value("AutoOpen", QVariant(false)).toBool();
}

bool Preferences::isAutoOpenOnRestoreFromTray() {
	return getQSettings().value("AutoOpenOnRestoreFromTray", QVariant(false)).toBool();
}

bool Preferences::isAutoSave() {
	return getQSettings().value("AutoSave", QVariant(false)).toBool();
}

bool Preferences::isAutoSaveOnMinimizeToTray() {
	return getQSettings().value("AutoSaveOnMinimizeToTray", QVariant(true)).toBool();
}

bool Preferences::isConfirmBeforeExit() {
	return getQSettings().value("ConfirmBeforeExit", QVariant(false)).toBool();
}

bool Preferences::isConfirmBeforeSpontaneousExit() {
	return getQSettings().value("ConfirmBeforeSpontaneousExit", QVariant(true)).toBool();
}

bool Preferences::isDefaultOpenModeReadOnly() {
	return getQSettings().value("DefaultOpenModeReadOnly", QVariant(false)).toBool();
}

bool Preferences::isMinimizeToTrayOnSpontaneousExit() {
	return getQSettings().value("MinimizeToTrayOnSpontaneousExit", QVariant(false)).toBool();
}

bool Preferences::isObscurePasswords() {
	return getQSettings().value("ObscurePasswords", QVariant(true)).toBool();
}

bool Preferences::isSaveWindowDisposition() {
	return getQSettings().value("SaveWindowDisposition", QVariant(false)).toBool();
}

bool Preferences::isShowToolBar() {
	return getQSettings().value("ShowToolBar", QVariant(true)).toBool();
}

int Preferences::getDisplayDensity() {
	return getQSettings().value("DisplayDensity",
#ifdef Q_OS_ANDROID
								QVariant(2)
#else
								QVariant(0)
#endif
								).toInt();
}

int Preferences::getMaximumNumberOfRecentFiles() {
	return getQSettings().value("MaximumNumberOfRecentFiles", QVariant(10)).toInt();
}

std::string Preferences::getMostRecentFile() {
	auto v = getRecentFiles();
	if (v.empty())
		return std::string();
	else
		return v[0];
}

std::vector<std::string> Preferences::getRecentFiles() {
	QStringList l = getQSettings().value("RecentFiles", QVariant(QStringList())).toStringList();
	std::vector<std::string> v;
	v.reserve(l.size());
	for (QString s: l)
		v.push_back(s.toStdString());
	return v;
}

void Preferences::reset() {
	getQSettings().clear();
}

void Preferences::setAutoCloseOnMinimizeToTray(bool v) {
	getQSettings().setValue("AutoCloseOnMinimizeToTray", QVariant(v));
}

void Preferences::setAutoOpen(bool v) {
	getQSettings().setValue("AutoOpen", QVariant(v));
}

void Preferences::setAutoOpenOnRestoreFromTray(bool v) {
	getQSettings().setValue("AutoOpenOnRestoreFromTray", QVariant(v));
}

void Preferences::setAutoSave(bool v) {
	getQSettings().setValue("AutoSave", QVariant(v));
}

void Preferences::setAutoSaveOnMinimizeToTray(bool v) {
	getQSettings().setValue("AutoSaveOnMinimizeToTray", QVariant(v));
}

void Preferences::setConfirmBeforeExit(bool v) {
	getQSettings().setValue("ConfirmBeforeExit", QVariant(v));
}

void Preferences::setConfirmBeforeSpontaneousExit(bool v) {
	getQSettings().setValue("ConfirmBeforeSpontaneousExit", QVariant(v));
}

void Preferences::setDefaultOpenModeReadOnly(bool v) {
	getQSettings().setValue("DefaultOpenModeReadOnly", QVariant(v));
}

void Preferences::setMaximumNumberOfRecentFiles(int v) {
	getQSettings().setValue("MaximumNumberOfRecentFiles", QVariant(v));
}

void Preferences::setMinimizeToTrayOnSpontaneousExit(bool v) {
	getQSettings().setValue("MinimizeToTrayOnSpontaneousExit", QVariant(v));
}

void Preferences::setObscurePasswords(bool v) {
	getQSettings().setValue("ObscurePasswords", QVariant(v));
}

void Preferences::setSaveWindowDisposition(bool v) {
	getQSettings().setValue("SaveWindowDisposition", QVariant(v));
}

void Preferences::setShowToolBar(bool v) {
	getQSettings().setValue("ShowToolBar", QVariant(v));
}

void Preferences::setDisplayDensity(int v) {
	getQSettings().setValue("DisplayDensity", QVariant(v));
}

void Preferences::setRecentFiles(std::vector<std::string> v) {
	int const max = getMaximumNumberOfRecentFiles();
	int i = 0;
	QStringList l;
	for (std::string s: v) {
		l.push_back(QString::fromStdString(s));
		i++;
		if (i == max)
			break;
	}

	getQSettings().setValue("RecentFiles", QVariant(l));
}

QTranslator* translator = NULL;

QString Preferences::getLanguage() {
	QString r = getQSettings().value("Language", QString(QLocale::languageToString(QLocale::system().language()))).toString().toLower();
	if (r.isEmpty() || r.contains('/') || r.contains('.'))
		r = "english";
	r = r + ".qm";
	QFile f(":/langs/" + r);
	if (!f.exists())
		return QString();
	return r;
}

void Preferences::setLanguage(QString language) {
	int p = language.indexOf('.');
	if (p >= 0)
		language = language.left(p);
	getQSettings().setValue("Language", QVariant(language.toLower()));
	updateLanguage();
}

void Preferences::updateLanguage() {
	if (translator != NULL) {
		QApplication::instance()->removeTranslator(translator);
		delete translator;
		translator = NULL;
	}
	QString language = getLanguage();
	if (!language.isEmpty()) {
		translator = new QTranslator(QApplication::instance());
		translator->load(":/langs/" + language);
		QApplication::instance()->installTranslator(translator);
	}
}



