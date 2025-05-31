#pragma once

#include "config.h"
#include "utils.h"
#include "document.h"
#include "gcry.h"

#include "passcave-gui.h"
#include "includes/datamodel.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>

#include <string>
#include <vector>
#include <unordered_map>

using namespace passcave;

class AddNewNodeDialog : public QDialog {
	Q_OBJECT

public:
	AddNewNodeDialog(MainWindow* parent,
				DataModel* dataModel,
				DocumentNodeId nodeId = InvalidDocumentNodeId);
	~AddNewNodeDialog();
	constexpr DataModel* getDataModel() const { return dataModel; }
	MainWindow* getMainWindow() const;
	constexpr DocumentNodeId getNodeId() const { return nodeId; }

protected:
	void closeEvent(QCloseEvent* e);

private:
	bool applied = false;
	DataModel* dataModel;
	SafeDocumentNode* initialNode;
	bool isModified = false;
	SafeDocumentNode* node;
	DocumentNodeId nodeId;
	std::unordered_map<std::string, std::vector<QWidget*>> propertyWidgets;

	QLabel* createQLabel(DocumentPropertyDefinition pDef);
	QLineEdit* createQLineEdit(DocumentPropertyDefinition pDef);
	void initUI();
	void linkQLabel(QLabel* l, QWidget* w);
	QString genPassword(QString chars, int len);
	std::string getPropertyNameFromSender(QObject* sender);
	bool isDataPropertyModified(std::string propertyName);
private slots:
	void onApplyClicked();
	void onCancelClicked();
	void onChange(QWidget* sender = nullptr);
	void onDisplayLongTextClicked();
	void onGenPasswordClicked();
	void onTextArrayAddClicked();
	void onTextArrayRemoveClicked();
};
