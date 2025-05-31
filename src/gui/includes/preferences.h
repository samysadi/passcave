#pragma once

#include <QSettings>
#include <QTranslator>

#include <string>
#include <vector>

class Preferences {
private:
	Preferences();
	static QSettings* qSettings;
public:
	static QSettings& getQSettings();

	static void addRecentFile(std::string s);
	static void clearRecentFiles();
	static void removeRecentFile(std::string s);

	static bool isAutoCloseOnMinimizeToTray();
	static bool isAutoOpen();
	static bool isAutoOpenOnRestoreFromTray();
	static bool isAutoSave();
	static bool isAutoSaveOnMinimizeToTray();
	static bool isConfirmBeforeExit();
	static bool isConfirmBeforeSpontaneousExit();
	static bool isDefaultOpenModeReadOnly();
	static bool isMinimizeToTrayOnSpontaneousExit();
	static bool isObscurePasswords();
	static bool isSaveWindowDisposition();
	static bool isShowToolBar();

	static int getDisplayDensity();
	static int getMaximumNumberOfRecentFiles();
	static std::string getMostRecentFile();
	static std::vector<std::string> getRecentFiles();

	static void reset();

	static void setAutoCloseOnMinimizeToTray(bool v);
	static void setAutoOpen(bool v);
	static void setAutoOpenOnRestoreFromTray(bool v);
	static void setAutoSave(bool v);
	static void setAutoSaveOnMinimizeToTray(bool v);
	static void setConfirmBeforeExit(bool v);
	static void setConfirmBeforeSpontaneousExit(bool v);
	static void setDefaultOpenModeReadOnly(bool v);
	static void setMaximumNumberOfRecentFiles(int v);
	static void setMinimizeToTrayOnSpontaneousExit(bool v);
	static void setObscurePasswords(bool v);
	static void setSaveWindowDisposition(bool v);
	static void setShowToolBar(bool v);

	static void setDisplayDensity(int v);
	static void setRecentFiles(std::vector<std::string> v);

	static QString getLanguage();
	static void setLanguage(QString language);
	static void updateLanguage();
};
